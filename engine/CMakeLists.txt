cmake_minimum_required(VERSION 3.16)
project(SolarisHistory LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_library(solaris_sim src/content.cpp src/sim.cpp)
target_include_directories(solaris_sim PUBLIC include)
include(CTest)
if(BUILD_TESTING)
    add_executable(SimTest tests/sim_test.cpp)
    target_link_libraries(SimTest PRIVATE solaris_sim)
    add_test(NAME simulation COMMAND SimTest)
    set_tests_properties(simulation PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

if(WIN32)
add_executable(SolarisHistory WIN32
    src/main.cpp
    src/content.cpp
    src/gfx.cpp
    src/sim.cpp
    src/platform.cpp
    src/ui.cpp
    src/game.cpp
)

target_include_directories(SolarisHistory PRIVATE include)

# UTF-8 para los literales en espanol
if(MSVC)
    target_compile_options(SolarisHistory PRIVATE /utf-8 /W3)
else()
    target_compile_options(SolarisHistory PRIVATE -Wall -Wextra -Wno-unused-parameter)
    target_compile_options(SolarisHistory PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

target_link_libraries(SolarisHistory PRIVATE opengl32 glu32 gdi32 user32)

set_target_properties(SolarisHistory PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Copia el contenido junto al ejecutable en la carpeta de salida
add_custom_target(SolarisContent ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/../content"
            "$<TARGET_FILE_DIR:SolarisHistory>/content"
    DEPENDS SolarisHistory
    COMMENT "Copiando contenido junto al ejecutable"
)

endif()
