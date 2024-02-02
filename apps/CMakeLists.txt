if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_subdirectory(mqtt-pub)
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_subdirectory(leitor-cli)
    add_subdirectory(medidor-simulado-cli)
endif()


# add other folder apps here
# ...
# ...
