#!/usr/bin/env python3
"""Send an SNMPv2c request over the A5 5A framed UART link."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Allow running without installing a package.
sys.path.insert(0, str(Path(__file__).resolve().parent))

SYNC = bytes((0xA5, 0x5A))


def encode_frame(payload: bytes) -> bytes:
    if not payload or len(payload) > 1024:
        raise ValueError("payload size")
    length = len(payload)
    cksum = ((length >> 8) & 0xFF) ^ (length & 0xFF)
    for byte in payload:
        cksum ^= byte
    return SYNC + bytes(((length >> 8) & 0xFF, length & 0xFF)) + payload + bytes((cksum,))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="serial device, e.g. /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--community", default="public")
    parser.add_argument("--oid", default="1.3.6.1.2.1.1.1.0")
    parser.add_argument("--set-led", type=int, choices=(0, 1))
    args = parser.parse_args()

    try:
        import serial  # type: ignore
    except ImportError:
        print("pyserial is required: pip install pyserial", file=sys.stderr)
        return 2

    # Local import after path setup would be the C helper; build the PDU via a tiny
    # subprocess against the host test binary is unnecessary — encode BER here.
    from snmp_ber import build_request, parse_response

    pdu = 0xA3 if args.set_led is not None else 0xA0
    oid = args.oid if args.set_led is None else "1.3.6.1.4.1.99999.1.1.0"
    payload = build_request(pdu, 1, args.community, oid, args.set_led)
    frame = encode_frame(payload)

    with serial.Serial(args.port, args.baud, timeout=2) as ser:
        ser.reset_input_buffer()
        ser.write(frame)
        header = ser.read(4)
        if len(header) < 4 or header[0] != 0xA5 or header[1] != 0x5A:
            print("no framed response", file=sys.stderr)
            return 1
        length = (header[2] << 8) | header[3]
        body = ser.read(length + 1)
        if len(body) != length + 1:
            print("short response", file=sys.stderr)
            return 1
        cksum = header[2] ^ header[3]
        for byte in body[:-1]:
            cksum ^= byte
        if cksum != body[-1]:
            print("bad checksum", file=sys.stderr)
            return 1
        parsed = parse_response(body[:-1])
        print(parsed)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
