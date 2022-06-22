cmake_minimum_required(VERSION 3.14)

# List all files containing tests. (Change as needed)
set(TESTFILES
    main.cpp
    ring_buffer.cpp
    CRC.cpp
    leitor.cpp
    # leitor_com_medidor_simulado.cpp
    # simulador/medidor.cpp
    # simulador/gerador_de_respostas.cpp
    BCD.cpp
    timer.cpp
)

set(TEST_MAIN testes-unitarios)
set(TEST_RUNNER_PARAMS "")  # Any arguments to feed the test runner (change as needed).

add_executable(${TEST_MAIN} ${TESTFILES})

target_link_libraries(${TEST_MAIN} PRIVATE ${LIBRARY_NAME} doctest)
set_target_properties(${TEST_MAIN} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
target_set_warnings(${TEST_MAIN} ENABLE ALL ALL DISABLE Annoying) # Set warnings (if needed).

set_target_properties(${TEST_MAIN} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED NO
    CXX_EXTENSIONS NO
)

add_test(
    # Use some per-module/project prefix so that it is easier to run only tests for this module
    NAME ${LIBRARY_NAME}.${TEST_MAIN}
    COMMAND ${TEST_MAIN} ${TEST_RUNNER_PARAMS})

# Adds a 'coverage' target.
include(CodeCoverage)
