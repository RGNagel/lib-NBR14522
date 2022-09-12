if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(SOURCE_SERIAL_POLICY ${CMAKE_SOURCE_DIR}/src/serial/serial_policy_unix.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(SOURCE_SERIAL_POLICY ${CMAKE_SOURCE_DIR}/src/serial/serial_policy_win.cpp)
endif()

set(LEITOR leitor-cli)

add_executable(${LEITOR} leitor-cli.cpp ${SOURCE_SERIAL_POLICY})
target_include_directories(${LEITOR} PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(${LEITOR} PRIVATE ${LIBRARY_NAME})
target_set_warnings(${LEITOR} ENABLE ALL ALL DISABLE Annoying) # Set warnings (if needed).
target_enable_lto(${LEITOR} optimized)  # enable link-time-optimization if available for non-debug configurations

set_target_properties(
    ${LEITOR}
      PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED NO
        CXX_EXTENSIONS NO
)
