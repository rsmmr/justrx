#include <benchmark/benchmark.h>

#include <random>
#include <string>

extern "C" {
#include "regex.h"
}

const int MAX_CAPTURES = 10;


#if 0
std::mt19937 pseudo_random(42);

// Generate random binary data of specified length
std::string generateRandomData(size_t length) {
    std::string result;
    result.reserve(length);
    std::uniform_int_distribution<> dis(1, 255);

    for ( size_t i = 0; i < length; ++i ) {
        result.push_back(static_cast<char>(dis(pseudo_random)));
    }

    return result;
}
#endif

// Generate single-character data of specified length.
std::string generateSingleCharData(size_t length) { return std::string(length, '#'); }

template<class... Args>
static void benchmark_regexec(benchmark::State& state, Args&&... args) {
    auto args_tuple = std::make_tuple(std::move(args)...);
    const size_t input_size = static_cast<size_t>(state.range(0));

    std::string data = generateSingleCharData(input_size);

    regex_t re;
    if ( regcomp(&re, std::get<0>(args_tuple), REG_EXTENDED | REG_ICASE) != 0 ) {
        state.SkipWithError("Failed to compile regex");
        return;
    }

    regmatch_t pmatch[MAX_CAPTURES];
    for ( auto _ : state ) {
        auto rc = regexec(&re, data.c_str(), MAX_CAPTURES, pmatch, 0);
        benchmark::DoNotOptimize(rc);
    }

    regfree(&re);

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(input_size));
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_CAPTURE(benchmark_regexec, plain, "x*")->RangeMultiplier(4)->Range(1024, 1024 * 1024);
BENCHMARK_CAPTURE(benchmark_regexec, ccl, "[ACEGIKMOQSUWYZ]*")->RangeMultiplier(4)->Range(1024, 1024 * 1024);

BENCHMARK_MAIN();
