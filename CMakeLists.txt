project(JustRX)
cmake_minimum_required(VERSION 3.13)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

option(JUSTRX_DISABLE_TESTS "Skips compiling tests for the library" OFF)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

set(autogen "${CMAKE_CURRENT_BINARY_DIR}/justrx/autogen")

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

flex_target(scanner src/re-scan.l ${autogen}/re-scan.c DEFINES_FILE ${autogen}/re-scan.h)
bison_target(parser src/re-parse.y ${autogen}/re-parse.c
             COMPILE_FLAGS "-Wno-conflicts-rr -Wno-conflicts-sr")
add_flex_bison_dependency(scanner parser)

set(SOURCES
    src/ccl.c
    src/dfa.c
    src/dfa-interpreter-std.c
    src/dfa-interpreter-min.c
    src/jlocale.c
    src/jrx.c
    src/nfa.c
    src/util.c
    ${BISON_parser_OUTPUTS}
    ${FLEX_scanner_OUTPUTS})

add_library(jrx-objects OBJECT ${SOURCES})
target_compile_options(jrx-objects PRIVATE "-fPIC")
target_include_directories(jrx-objects BEFORE PUBLIC include)
target_include_directories(jrx-objects BEFORE PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_library(jrx $<TARGET_OBJECTS:jrx-objects>)

add_subdirectory(3rdparty)

if (NOT JUSTRX_DISABLE_TESTS)
    add_subdirectory(src/tests)
endif ()
