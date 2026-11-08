cmake_minimum_required(VERSION 3.20)
project(FMIndexStudy VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FMI_BUILD_TESTS "Build the correctness tests" ON)

if(MSVC)
    add_compile_options(/W4 /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
endif()

add_library(fmi_core STATIC
    src/core/Types.cpp
    src/core/Alphabet.cpp
    src/core/SuffixArrayBuilder.cpp
    src/core/BWTBuilder.cpp
    src/core/BitVector.cpp
    src/core/RankSupport.cpp
    src/core/WaveletTree.cpp
    src/core/SampledSuffixArray.cpp
    src/core/FMIndex.cpp
    src/core/FMIndexBuilder.cpp
    src/core/FMIndexSearcher.cpp)
target_include_directories(fmi_core PUBLIC src)

add_library(fmi_benchmark STATIC
    src/benchmark/NaiveSearcher.cpp
    src/benchmark/SuffixArraySearcher.cpp
    src/benchmark/MetricsCollector.cpp
    src/benchmark/BenchmarkRunner.cpp)
target_link_libraries(fmi_benchmark PUBLIC fmi_core)

add_executable(fmi_benchmark_cli src/cli/main.cpp)
target_link_libraries(fmi_benchmark_cli PRIVATE fmi_benchmark)

if(FMI_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_executable(fmi_tests
        tests/test_suffix_array.cpp
        tests/test_bwt.cpp
        tests/test_rank.cpp
        tests/test_wavelet_tree.cpp
        tests/test_fm_index.cpp
        tests/test_locate.cpp
        tests/test_benchmark.cpp)
    target_link_libraries(fmi_tests PRIVATE fmi_benchmark GTest::gtest_main)
    add_test(NAME fmi_tests COMMAND fmi_tests)
endif()
