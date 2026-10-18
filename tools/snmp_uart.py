#!/usr/bin/env python3
# Отправить SNMPv2c-запрос по кадрам A5 5A через последовательный порт.

from __future__ import annotations  # синтаксис аннотаций 3.10+

import argparse  # разбор аргументов командной строки
import sys  # stderr и код выхода
from pathlib import Path  # путь к каталогу скрипта

sys.path.insert(0, str(Path(__file__).resolve().parent))  # найти соседний snmp_ber.py

SYNC = bytes((0xA5, 0x5A))  # два синхробайта кадра


def encode_frame(payload: bytes) -> bytes:  # упаковать BER в кадр UART
    if not payload or len(payload) > 1024:  # пусто или слишком длинно
        raise ValueError("payload size")  # отказаться
    length = len(payload)  # длина нагрузки
    cksum = ((length >> 8) & 0xFF) ^ (length & 0xFF)  # XOR старшего и младшего байта длины
    for byte in payload:  # пройти нагрузку
        cksum ^= byte  # накопить контрольную сумму
    return SYNC + bytes(((length >> 8) & 0xFF, length & 0xFF)) + payload + bytes((cksum,))  # полный кадр


def main() -> int:  # точка входа CLI
    parser = argparse.ArgumentParser(description="SNMP по UART")  # описание утилиты
    parser.add_argument("port", help="последовательный порт, например /dev/ttyACM0")  # устройство
    parser.add_argument("--baud", type=int, default=115200)  # скорость, как на плате
    parser.add_argument("--community", default="public")  # community
    parser.add_argument("--oid", default="1.3.6.1.2.1.1.1.0")  # OID по умолчанию — sysDescr
    parser.add_argument("--set-led", type=int, choices=(0, 1))  # если задан — SET ledState
    args = parser.parse_args()  # разобрать argv

    try:  # pyserial может быть не установлен
        import serial  # type: ignore  # библиотека COM-порта
    except ImportError:  # модуля нет
        print("нужен pyserial: pip install pyserial", file=sys.stderr)  # подсказка
        return 2  # код «нет зависимости»

    from snmp_ber import build_request, parse_response  # сборка и разбор BER

    pdu = 0xA3 if args.set_led is not None else 0xA0  # SET или GET
    oid = args.oid if args.set_led is None else "1.3.6.1.4.1.99999.1.1.0"  # для SET всегда ledState
    payload = build_request(pdu, 1, args.community, oid, args.set_led)  # BER-запрос
    frame = encode_frame(payload)  # обернуть в кадр

    with serial.Serial(args.port, args.baud, timeout=2) as ser:  # открыть порт на 2 секунды таймаута
        ser.reset_input_buffer()  # выбросить старый мусор
        ser.write(frame)  # отправить запрос
        header = ser.read(4)  # A5 5A len_hi len_lo
        if len(header) < 4 or header[0] != 0xA5 or header[1] != 0x5A:  # нет кадра
            print("нет кадрового ответа", file=sys.stderr)  # ошибка
            return 1  # неуспех
        length = (header[2] << 8) | header[3]  # длина нагрузки
        body = ser.read(length + 1)  # нагрузка плюс контрольная сумма
        if len(body) != length + 1:  # плата ответила не полностью
            print("короткий ответ", file=sys.stderr)  # ошибка
            return 1  # неуспех
        cksum = header[2] ^ header[3]  # начать сумму с байтов длины
        for byte in body[:-1]:  # все байты нагрузки
            cksum ^= byte  # накопить XOR
        if cksum != body[-1]:  # сумма не совпала
            print("плохая контрольная сумма", file=sys.stderr)  # ошибка
            return 1  # неуспех
        parsed = parse_response(body[:-1])  # разобрать BER без байта суммы
        print(parsed)  # показать поля ответа
    return 0  # успех


if __name__ == "__main__":  # запуск как скрипт
    raise SystemExit(main())  # код возврата main()
