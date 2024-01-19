set(SOURCE_SERIAL_POLICY ${CMAKE_SOURCE_DIR}/src/serial/serial_policy_unix.cpp)

set(RELAY tcp-client-to-serial)

add_executable(${RELAY} main.cpp ${SOURCE_SERIAL_POLICY})
target_include_directories(${RELAY} PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(${RELAY} PRIVATE ${LIBRARY_NAME})
target_set_warnings(${RELAY} ENABLE ALL ALL DISABLE Annoying) # Set warnings (if needed).
target_enable_lto(${RELAY} optimized)  # enable link-time-optimization if available for non-debug configurations

set_target_properties(
    ${RELAY}
      PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED NO
        CXX_EXTENSIONS NO
)
