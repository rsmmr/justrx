if (NOT JUSTRX_DISABLE_TESTS AND NOT JUSTRX_HAVE_BENCHMARK)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
    set(BENCHMARK_ENABLE_TESTING OFF)
    add_subdirectory(benchmark)
endif ()
