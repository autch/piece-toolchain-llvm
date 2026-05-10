cmake_minimum_required(VERSION 3.13)
project(ppack C)

find_package(ZLIB REQUIRED)
find_package(Iconv)

add_executable(ppack
    ppack.c
    crc32.c
    iz.c
)

target_include_directories(ppack PRIVATE ${ZLIB_INCLUDE_DIRS})
target_link_libraries(ppack PRIVATE ${ZLIB_LIBRARIES})

if(Iconv_FOUND)
    target_compile_definitions(ppack PRIVATE PPACK_HAVE_ICONV)
    target_include_directories(ppack PRIVATE ${Iconv_INCLUDE_DIRS})
    target_link_libraries(ppack PRIVATE ${Iconv_LIBRARIES})
else()
    message(STATUS "ppack: iconv not found; -N option will be unavailable. Use -n with raw CP932 bytes instead.")
endif()
