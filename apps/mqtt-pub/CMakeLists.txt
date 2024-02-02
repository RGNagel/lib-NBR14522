set(SOURCE_SERIAL_POLICY ${CMAKE_SOURCE_DIR}/src/serial/serial_policy_unix.cpp)

set(APP mqtt-pub)

add_executable(${APP} 
  main.cpp 
  ${SOURCE_SERIAL_POLICY} 
  ./MQTT-C/src/mqtt.c 
  ./MQTT-C/src/mqtt_pal.c
)
target_include_directories(${APP} PUBLIC ${CMAKE_SOURCE_DIR}/include ./MQTT-C/include)
target_link_libraries(${APP} PRIVATE ${LIBRARY_NAME})
target_set_warnings(${APP} ENABLE ALL ALL DISABLE Annoying) # Set warnings (if needed).
target_enable_lto(${APP} optimized)  # enable link-time-optimization if available for non-debug configurations

target_compile_options(${APP} PRIVATE -ggdb)


set_target_properties(
    ${APP}
      PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED NO
        CXX_EXTENSIONS NO
)
