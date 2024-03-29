# This file specifies how the project should be built, using CMake.
# If you are unfamiliar with CMake, don't worry about all the details.
# The sections you might want to edit are marked as such, and
# the comments should hopefully make most of it clear.
#
# For many purposes, you may not need to change anything about this file.

cmake_minimum_required(VERSION 3.14)

# Set project name, version and laguages here. (change as needed)
# Version numbers are available by including "exampleConfig.h" in
# the source. See exampleConfig.h.in for some more details.
project(LIB_NBR14522 VERSION 1.2.3.4 LANGUAGES CXX)

# message(STATUS "CMAKE_SYSTEM_NAME: " ${CMAKE_SYSTEM_NAME})

# Options: Things you can set via commandline options to cmake (e.g. -DENABLE_LTO=[ON|OFF])
option(ENABLE_WARNINGS_SETTINGS "Allow target_set_warnings to add flags and defines.
                                 Set this to OFF if you want to provide your own warning parameters." ON)
option(ENABLE_LTO "Enable link time optimization" ON)
option(ENABLE_DOCTESTS "Include tests in the library. Setting this to OFF will remove all doctest related code.
                        Tests in tests/*.cpp will still be enabled." ON)

# Include stuff. No change needed.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
include(ConfigSafeGuards)
include(Colors)
include(CTest)
include(Doctest)
include(Documentation)
include(LTO)
include(Misc)
include(Warnings)

# Check for LTO support.
find_lto(CXX)

# --------------------------------------------------------------------------------
#                         Locate files (change as needed).
# --------------------------------------------------------------------------------

set(SOURCES_LIBRARY
    src/CRC.cpp
)

set(LIBRARY_NAME leitor-lib)  

# Compile all sources into a library.
add_library(${LIBRARY_NAME} OBJECT ${SOURCES_LIBRARY})

# Lib needs its header files, and users of the library must also see these (PUBLIC). (No change needed)
target_include_directories(${LIBRARY_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

# There's also (probably) doctests within the library, so we need to see this as well.
# target_link_libraries(${LIBRARY_NAME} PUBLIC doctest)

# Set the compile options you want (change as needed).
target_set_warnings(${LIBRARY_NAME} ENABLE ALL ALL DISABLE Annoying)

# target_compile_options(${LIBRARY_NAME} ... )  # For setting manually.

# Add an executable for the file app/main.cpp.
# If you add more executables, copy these lines accordingly.

# add_executable(main app/main.cpp)   # Name of exec. and location of file.
# target_link_libraries(main PRIVATE ${LIBRARY_NAME})  # Link the executable to library (if it uses it).
# target_set_warnings(main ENABLE ALL ALL DISABLE Annoying) # Set warnings (if needed).
# target_enable_lto(main optimized)  # enable link-time-optimization if available for non-debug configurations

set_target_properties(
    ${LIBRARY_NAME}
      PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED NO
        CXX_EXTENSIONS NO
)

add_subdirectory(apps)
add_subdirectory(tests)

