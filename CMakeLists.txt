cmake_minimum_required(VERSION 3.10)
project(CHIP_8_Emulator C)

set(CMAKE_C_STANDARD 99)

find_package(SDL2 REQUIRED)

add_executable(CHIP_8_Emulator
        main.c
        chip8.c
        display.c
)

target_include_directories(CHIP_8_Emulator PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(CHIP_8_Emulator PRIVATE ${SDL2_LIBRARIES})