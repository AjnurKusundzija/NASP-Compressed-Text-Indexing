#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "TestSupport.h"
#include "benchmark/BenchmarkRunner.h"
#include "benchmark/NaiveSearcher.h"
#include "benchmark/SuffixArraySearcher.h"

using namespace fmi;

TEST(Baselines, SuffixArraySearchMatchesNaive) {
    std::mt19937 rng(17);
    for (int trial = 0; trial < 30; ++trial) {
        const std::string text = testing_support::randomText(rng, 1 + rng() % 400, 3);
        const NaiveSearcher naive(text);
        const SuffixArraySearcher sa(text);
        for (int q = 0; q < 30; ++q) {
            const std::string pattern = testing_support::randomText(rng, 1 + rng() % 5, 4);
            ASSERT_EQ(sa.count(pattern), naive.count(pattern));
            ASSERT_EQ(sa.locate(pattern), naive.locate(pattern));
        }
    }
}

TEST(Baselines, NaiveCountsOverlappingMatches) {
    EXPECT_EQ(NaiveSearcher("aaaaaa").count("aaa"), 4u);
    EXPECT_EQ(NaiveSearcher("banana").locate("ana"), (std::vector<size_t>{1, 3}));
    EXPECT_EQ(NaiveSearcher("banana").count(""), 0u);
}

TEST(Benchmark, RunsSeriallyAndProducesConsistentRows) {
    const std::string log = BenchmarkRunner::generateLog(60000, 3);
    BenchmarkSettings settings;
    settings.repetitions = 2;
    settings.patternsPerLength = 3;
    settings.buildRepetitions = 1;
    settings.patternLengths = {4, 8, 16};
    settings.samplingIntervals = {16, 64};
    const BenchmarkReport report = BenchmarkRunner(settings).run(log);

    EXPECT_FALSE(report.select("index_size").empty());
    EXPECT_FALSE(report.select("count_latency").empty());
    EXPECT_FALSE(report.select("locate_latency").empty());
    EXPECT_EQ(report.select("sampling_memory").size(), 4u);
    for (const BenchRow& row : report.rows) {
        EXPECT_GE(row.median, 0.0) << row.experiment;
        EXPECT_LE(row.minimum, row.median + 1e-9) << row.experiment;
        EXPECT_GE(row.maximum + 1e-9, row.median) << row.experiment;
    }
    double dense = 0;
    double sparse = 0;
    for (const BenchRow& row : report.select("sampling_memory")) {
        if (row.series == "SA sampling") {
            (row.x == 16 ? dense : sparse) = row.median;
        }
    }
    EXPECT_GT(dense, sparse);
}

TEST(Benchmark, CsvContainsHeaderAndRows) {
    BenchmarkSettings settings;
    settings.repetitions = 1;
    settings.patternsPerLength = 2;
    settings.buildRepetitions = 1;
    settings.patternLengths = {4};
    settings.samplingIntervals = {32};
    const BenchmarkReport report = BenchmarkRunner(settings).run(BenchmarkRunner::generateLog(20000, 1));
    const std::string path = (std::filesystem::temp_directory_path() / "fmi_bench_test.csv").string();
    BenchmarkRunner::writeCsv(report, path);
    std::ifstream in(path);
    std::string content((std::istreambuf_iterator<char>(in)), {});
    in.close();
    std::filesystem::remove(path);
    EXPECT_NE(content.find("experiment,series,x,median,min,max,unit"), std::string::npos);
    EXPECT_NE(content.find("\"FM-index, sampled Occ\""), std::string::npos);
    EXPECT_NE(content.find("# Execution model: serial"), std::string::npos);
}

TEST(Benchmark, EmptyTextIsRejected) {
    EXPECT_THROW(BenchmarkRunner().run(""), IndexError);
}

TEST(Benchmark, CancellationStopsTheRun) {
    BuildHooks hooks;
    hooks.isCancelled = [] { return true; };
    EXPECT_THROW(BenchmarkRunner({}, hooks).run(BenchmarkRunner::generateLog(5000, 1)), OperationCancelled);
}
