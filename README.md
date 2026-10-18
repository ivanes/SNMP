# SNMP-F411

Прошивка SNMPv2c-агента для **STM32F411** на **FreeRTOS**. Репозиторий больше не заготовка под Arduino: ядро — Cortex-M4, планировщик FreeRTOS и переносимый SNMP-агент.

Целевые платы:

- NUCLEO-F411RE (по умолчанию) — LED `PA5`, USART2 `PA2/PA3` через ST-LINK VCP
- WeAct Black Pill STM32F411CE — LED `PC13`, USART1 `PA9/PA10`

У MCU нет Ethernet MAC, поэтому SNMP ходит по UART в коротких кадрах. Тот же агент собирается и гоняется на хосте без железа.

## Состав

| Путь | Назначение |
| --- | --- |
| `lib/snmp` | SNMPv2c GET / GETNEXT / SET + кадры `A5 5A` |
| `firmware` | startup, тактирование 100 МГц, UART, задачи FreeRTOS |
| `tests` | хостовые тесты агента и кадрирования |
| `third_party` | FreeRTOS-Kernel V11.2.0 и CMSIS для STM32F411 |
| `tools/snmp_uart.py` | запрос к плате через serial |

## Задачи FreeRTOS

- `led` — мигает светодиодом, пока его не зафиксирует SNMP SET
- `snmp` — принимает кадр, разбирает PDU, отвечает
- таймерный сервис FreeRTOS

Тактирование: HSI 16 МГц → PLL 100 МГц, APB1 = 50 МГц, APB2 = 100 МГц.

## MIB

| OID | Объект | Доступ |
| --- | --- | --- |
| `1.3.6.1.2.1.1.1.0` | `sysDescr` | read |
| `1.3.6.1.2.1.1.2.0` | `sysObjectID` | read |
| `1.3.6.1.2.1.1.3.0` | `sysUpTime` | read |
| `1.3.6.1.2.1.1.4.0` | `sysContact` | read |
| `1.3.6.1.2.1.1.5.0` | `sysName` | read |
| `1.3.6.1.2.1.1.6.0` | `sysLocation` | read |
| `1.3.6.1.2.1.1.7.0` | `sysServices` | read |
| `1.3.6.1.4.1.99999.1.1.0` | `ledState` (0/1) | read/write |

Community: `public` (чтение), `private` (запись).

## Сборка

Нужны CMake ≥ 3.20, `gcc` и `arm-none-eabi-gcc`.

```bash
make test                 # хостовые тесты SNMP
make firmware             # Nucleo: build/fw/firmware/snmp_f411.{elf,bin,hex}
make firmware-blackpill   # Black Pill
```

Прошивка Nucleo:

```bash
st-flash write build/fw/firmware/snmp_f411.bin 0x08000000
```

UART: 115200 8N1. Кадр: `A5 5A | len_hi len_lo | SNMP BER | xor-checksum`.

```bash
python3 tools/snmp_uart.py /dev/ttyACM0
python3 tools/snmp_uart.py /dev/ttyACM0 --community private --set-led 1
```

## Лицензии

Код приложения — GNU GPL v2 (`LICENSE`). FreeRTOS — MIT, CMSIS Core — Apache-2.0, CMSIS Device — BSD-3-Clause. Версии и файлы перечислены в `third_party/NOTICE`.
