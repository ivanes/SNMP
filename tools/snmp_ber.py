"""Minimal SNMPv2c BER helpers for the UART host tool."""

from __future__ import annotations


def _encode_length(n: int) -> bytes:
    if n < 0x80:
        return bytes((n,))
    if n <= 0xFF:
        return bytes((0x81, n))
    return bytes((0x82, (n >> 8) & 0xFF, n & 0xFF))


def _tlv(tag: int, body: bytes) -> bytes:
    return bytes((tag,)) + _encode_length(len(body)) + body


def _encode_int(value: int, tag: int = 0x02) -> bytes:
    if value == 0:
        return _tlv(tag, b"\x00")
    raw = value.to_bytes(4, "big", signed=True)
    start = 0
    while start < 3:
        if raw[start] == 0x00 and raw[start + 1] < 0x80:
            start += 1
            continue
        if raw[start] == 0xFF and raw[start + 1] >= 0x80:
            start += 1
            continue
        break
    return _tlv(tag, raw[start:])


def _encode_oid(text: str) -> bytes:
    arcs = [int(part) for part in text.split(".")]
    body = bytearray((40 * arcs[0] + arcs[1],))
    for arc in arcs[2:]:
        tmp = [arc & 0x7F]
        arc >>= 7
        while arc:
            tmp.append((arc & 0x7F) | 0x80)
            arc >>= 7
        body.extend(reversed(tmp))
    return _tlv(0x06, bytes(body))


def build_request(pdu_tag: int, request_id: int, community: str, oid: str, set_led: int | None) -> bytes:
    if set_led is None:
        value = _tlv(0x05, b"")
    else:
        value = _encode_int(set_led)
    varbind = _tlv(0x30, _encode_oid(oid) + value)
    vb_list = _tlv(0x30, varbind)
    pdu = _encode_int(request_id) + _encode_int(0) + _encode_int(0) + vb_list
    message = _encode_int(1) + _tlv(0x04, community.encode("ascii")) + _tlv(pdu_tag, pdu)
    return _tlv(0x30, message)


def _read_len(data: bytes, i: int) -> tuple[int, int]:
    first = data[i]
    i += 1
    if first < 0x80:
        return first, i
    n = first & 0x7F
    value = 0
    for _ in range(n):
        value = (value << 8) | data[i]
        i += 1
    return value, i


def _read_tlv(data: bytes, i: int) -> tuple[int, bytes, int]:
    tag = data[i]
    length, i = _read_len(data, i + 1)
    return tag, data[i : i + length], i + length


def parse_response(data: bytes) -> dict[str, object]:
    tag, body, _ = _read_tlv(data, 0)
    if tag != 0x30:
        raise ValueError("not a sequence")
    i = 0
    _, version, i = _read_tlv(body, i)
    _, community, i = _read_tlv(body, i)
    pdu_tag, pdu, i = _read_tlv(body, i)
    j = 0
    _, req, j = _read_tlv(pdu, j)
    _, err, j = _read_tlv(pdu, j)
    _, idx, j = _read_tlv(pdu, j)
    _, vbl, j = _read_tlv(pdu, j)
    _, vb, _ = _read_tlv(vbl, 0)
    k = 0
    _, oid, k = _read_tlv(vb, k)
    vtag, value, _ = _read_tlv(vb, k)
    return {
        "version": int.from_bytes(version, "big", signed=True),
        "community": community.decode("ascii", "replace"),
        "pdu": pdu_tag,
        "request_id": int.from_bytes(req, "big", signed=True),
        "error_status": int.from_bytes(err, "big", signed=True),
        "error_index": int.from_bytes(idx, "big", signed=True),
        "oid": oid.hex(),
        "value_tag": vtag,
        "value": value.hex(),
    }
