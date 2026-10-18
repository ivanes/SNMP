#!/usr/bin/env python3
# Минимальные помощники SNMPv2c BER для UART-клиента.

from __future__ import annotations  # аннотации как строки не нужны, синтаксис 3.10+


def _encode_length(n: int) -> bytes:  # закодировать длину BER
    if n < 0x80:  # короткая форма, один байт
        return bytes((n,))  # само значение длины
    if n <= 0xFF:  # длинная форма, 1 байт длины
        return bytes((0x81, n))  # 0x81 и сам байт
    return bytes((0x82, (n >> 8) & 0xFF, n & 0xFF))  # 0x82 и 16-битная длина


def _tlv(tag: int, body: bytes) -> bytes:  # собрать TLV: тег, длина, содержимое
    return bytes((tag,)) + _encode_length(len(body)) + body  # склеить три части


def _encode_int(value: int, tag: int = 0x02) -> bytes:  # INTEGER или TimeTicks
    if value == 0:  # ноль кодируется одним байтом 0x00
        return _tlv(tag, b"\x00")  # тег и один нулевой байт
    raw = value.to_bytes(4, "big", signed=True)  # 4 байта big-endian со знаком
    start = 0  # индекс первого значащего байта
    while start < 3:  # убрать лишние знаковые расширения
        if raw[start] == 0x00 and raw[start + 1] < 0x80:  # лишний 0x00 перед положительным
            start += 1  # сдвинуть начало
            continue  # проверить снова
        if raw[start] == 0xFF and raw[start + 1] >= 0x80:  # лишний 0xFF перед отрицательным
            start += 1  # сдвинуть начало
            continue  # проверить снова
        break  # минимальная кодировка найдена
    return _tlv(tag, raw[start:])  # отдать укороченное число


def _encode_oid(text: str) -> bytes:  # закодировать OID из текста 1.3.6...
    arcs = [int(part) for part in text.split(".")]  # разобрать дуги
    body = bytearray((40 * arcs[0] + arcs[1],))  # первые две дуги в одном байте
    for arc in arcs[2:]:  # остальные дуги
        tmp = [arc & 0x7F]  # младшие 7 бит
        arc >>= 7  # сдвиг на следующую группу
        while arc:  # пока есть старшие биты
            tmp.append((arc & 0x7F) | 0x80)  # продолжение с битом 0x80
            arc >>= 7  # следующая группа
        body.extend(reversed(tmp))  # старшие группы идут первыми
    return _tlv(0x06, bytes(body))  # тег OBJECT IDENTIFIER


def build_request(pdu_tag: int, request_id: int, community: str, oid: str, set_led: int | None) -> bytes:  # собрать запрос
    if set_led is None:  # GET или GETNEXT
        value = _tlv(0x05, b"")  # значение NULL
    else:  # SET светодиода
        value = _encode_int(set_led)  # INTEGER 0 или 1
    varbind = _tlv(0x30, _encode_oid(oid) + value)  # одна пара OID+значение
    vb_list = _tlv(0x30, varbind)  # список varbind
    pdu = _encode_int(request_id) + _encode_int(0) + _encode_int(0) + vb_list  # id, error-status, error-index, varbinds
    message = _encode_int(1) + _tlv(0x04, community.encode("ascii")) + _tlv(pdu_tag, pdu)  # v2c + community + PDU
    return _tlv(0x30, message)  # внешний SEQUENCE


def _read_len(data: bytes, i: int) -> tuple[int, int]:  # прочитать длину BER
    first = data[i]  # первый байт длины
    i += 1  # сдвинуться дальше
    if first < 0x80:  # короткая форма
        return first, i  # длина и новая позиция
    n = first & 0x7F  # сколько байт занимает длина
    value = 0  # собираемое число
    for _ in range(n):  # прочитать n байт
        value = (value << 8) | data[i]  # нарастить длину
        i += 1  # следующий байт
    return value, i  # длина и позиция


def _read_tlv(data: bytes, i: int) -> tuple[int, bytes, int]:  # прочитать один TLV
    tag = data[i]  # тег
    length, i = _read_len(data, i + 1)  # длина после тега
    return tag, data[i : i + length], i + length  # тег, тело, позиция после TLV


def parse_response(data: bytes) -> dict[str, object]:  # разобрать GetResponse в словарь
    tag, body, _ = _read_tlv(data, 0)  # внешний SEQUENCE
    if tag != 0x30:  # это не SEQUENCE
        raise ValueError("not a sequence")  # битый пакет
    i = 0  # позиция внутри сообщения
    _, version, i = _read_tlv(body, i)  # версия
    _, community, i = _read_tlv(body, i)  # community
    pdu_tag, pdu, i = _read_tlv(body, i)  # PDU
    j = 0  # позиция внутри PDU
    _, req, j = _read_tlv(pdu, j)  # request-id
    _, err, j = _read_tlv(pdu, j)  # error-status
    _, idx, j = _read_tlv(pdu, j)  # error-index
    _, vbl, j = _read_tlv(pdu, j)  # список varbind
    _, vb, _ = _read_tlv(vbl, 0)  # первый varbind
    k = 0  # позиция внутри varbind
    _, oid, k = _read_tlv(vb, k)  # OID
    vtag, value, _ = _read_tlv(vb, k)  # значение
    return {  # удобный словарь для печати
        "version": int.from_bytes(version, "big", signed=True),  # версия SNMP
        "community": community.decode("ascii", "replace"),  # community строкой
        "pdu": pdu_tag,  # тег PDU
        "request_id": int.from_bytes(req, "big", signed=True),  # request-id
        "error_status": int.from_bytes(err, "big", signed=True),  # error-status
        "error_index": int.from_bytes(idx, "big", signed=True),  # error-index
        "oid": oid.hex(),  # OID в hex
        "value_tag": vtag,  # тег значения
        "value": value.hex(),  # значение в hex
    }  # конец словаря
