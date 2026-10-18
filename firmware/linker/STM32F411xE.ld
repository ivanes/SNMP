ENTRY(Reset_Handler) /* старт после сброса — Reset_Handler */

MEMORY /* регионы памяти STM32F411xE */
{ /* начало MEMORY */
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K /* 512 КБ флеш */
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K /* 128 КБ ОЗУ */
} /* конец MEMORY */

_estack = ORIGIN(RAM) + LENGTH(RAM); /* начальный указатель стека — конец RAM */
_Min_Heap_Size = 0x200; /* резерв кучи newlib, проверка линкера */
_Min_Stack_Size = 0x400; /* резерв стека MSP, проверка линкера */

SECTIONS /* раскладка выходных секций */
{ /* начало SECTIONS */
    .isr_vector : ALIGN(4) /* таблица векторов с адреса 0x08000000 */
    { /* начало .isr_vector */
        KEEP(*(.isr_vector)) /* не выбрасывать таблицу при --gc-sections */
    } > FLASH /* лежит во флеш */

    .text : ALIGN(4) /* код и константы */
    { /* начало .text */
        *(.text*) /* весь код */
        *(.rodata*) /* строки и константы */
        KEEP(*(.init)) /* конструкторы */
        KEEP(*(.fini)) /* деструкторы */
        . = ALIGN(4); /* выровнять */
        _etext = .; /* конец кода во флеш */
    } > FLASH /* секция во флеш */

    .ARM.exidx : { *(.ARM.exidx*) } > FLASH /* таблица размотки исключений ARM */

    _sidata = LOADADDR(.data); /* адрес инициализаторов .data во флеш */

    .data : ALIGN(4) /* инициализированные переменные в RAM */
    { /* начало .data */
        _sdata = .; /* старт .data в RAM */
        *(.data*) /* сами переменные */
        . = ALIGN(4); /* выровнять */
        _edata = .; /* конец .data в RAM */
    } > RAM AT > FLASH /* жить в RAM, грузиться из флеш */

    .bss (NOLOAD) : ALIGN(4) /* ноль, не хранить во флеш */
    { /* начало .bss */
        _sbss = .; /* старт BSS */
        __bss_start__ = .; /* синоним для newlib */
        *(.bss*) /* неинициализированные переменные */
        *(COMMON) /* общие символы */
        . = ALIGN(4); /* выровнять */
        _ebss = .; /* конец BSS */
        __bss_end__ = .; /* синоним для newlib */
    } > RAM /* только в ОЗУ */

    ._user_heap_stack (NOLOAD) : ALIGN(8) /* проверка, что heap+stack влезают */
    { /* начало резерва */
        . = . + _Min_Heap_Size; /* место под кучу */
        . = . + _Min_Stack_Size; /* место под стек */
    } > RAM /* в ОЗУ */

    /DISCARD/ : /* выбросить лишнее */
    { /* начало DISCARD */
        *(.comment) /* комментарии компилятора */
        *(.note*) /* ELF-заметки */
    } /* конец DISCARD */
} /* конец SECTIONS */
