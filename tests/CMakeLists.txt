add_executable(test_snmp
    test_snmp.c
)

target_link_libraries(test_snmp PRIVATE snmp)
target_compile_options(test_snmp PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME snmp_agent COMMAND test_snmp)
