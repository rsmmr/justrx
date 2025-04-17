include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(retest retest.c)
target_link_libraries(retest jrx)

add_executable(testregex testregex.c)
target_link_libraries(testregex jrx)

add_executable(bench benchmark.cc)
target_link_libraries(bench benchmark::benchmark jrx)
