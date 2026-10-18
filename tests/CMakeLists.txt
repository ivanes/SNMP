add_executable(test_snmp # хостовый тест-раннер
    test_snmp.c # все проверки агента и кадров
) # конец исходников теста

target_link_libraries(test_snmp PRIVATE snmp) # линковать библиотеку SNMP
target_compile_options(test_snmp PRIVATE -Wall -Wextra -Wpedantic) # строгие предупреждения

add_test(NAME snmp_agent COMMAND test_snmp) # зарегистрировать тест в CTest
