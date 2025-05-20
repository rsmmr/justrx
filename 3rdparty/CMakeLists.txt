
if ( NOT JUSTRX_DISABLE_TESTS AND NOT JUSTRX_HAVE_BENCHMARK )
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable gtest tests")
    set(BENCHMARK_ENABLE_TESTS OFF CACHE BOOL "Disable tests")
    add_subdirectory(benchmark)
endif()
