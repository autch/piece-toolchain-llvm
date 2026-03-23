cmake_minimum_required(VERSION 3.13)
project(ppack C)

find_package(ZLIB REQUIRED)

add_executable(ppack
    ppack.c
    crc32.c
    iz.c
)

target_include_directories(ppack PRIVATE ${ZLIB_INCLUDE_DIRS})
target_link_libraries(ppack PRIVATE ${ZLIB_LIBRARIES})
