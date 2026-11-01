cmake_minimum_required(VERSION 3.16)
project(SolarisHistory LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

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
add_custom_command(TARGET SolarisHistory POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/../content"
            "$<TARGET_FILE_DIR:SolarisHistory>/content"
    COMMENT "Copiando contenido junto al ejecutable"
)
