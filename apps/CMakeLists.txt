if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_subdirectory(leitor-cli)
endif()

# add other folder apps here
# ...
# ...