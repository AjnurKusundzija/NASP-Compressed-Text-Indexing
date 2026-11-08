#include <gtest/gtest.h>

#include "TestSupport.h"

using namespace fmi;

TEST(Locate, SampledValuesMatchFullSuffixArray) {
    std::mt19937 rng(31);
    const std::string text = testing_support::randomText(rng, 500, 4);
    for (uint32_t k : {1u, 2u, 7u, 16u, 32u, 64u, 128u}) {
        std::vector<uint32_t> sa;
        const BuildResult built = FMIndexBuilder({16, k, RankKind::Checkpoint}).build(text, &sa);
        const SampledSuffixArray& samples = built.index.samples();
        size_t sampled = 0;
        for (size_t row = 0; row < sa.size(); ++row) {
            const bool expected = sa[row] % k == 0;
            ASSERT_EQ(samples.contains(row), expected);
            if (expected) {
                ASSERT_EQ(samples.value(row), sa[row]);
                ++sampled;
            }
        }
        EXPECT_EQ(samples.sampleCount(), sampled);
    }
}

TEST(Locate, EveryRowIsRecoveredForAllIntervals) {
    const std::string text = "the rain in spain stays mainly in the plain";
    for (RankKind kind : {RankKind::Checkpoint, RankKind::WaveletTree}) {
        for (uint32_t k : {1u, 3u, 8u, 32u, 128u}) {
            std::vector<uint32_t> sa;
            const BuildResult built = FMIndexBuilder({8, k, kind}).build(text, &sa);
            const FMIndexSearcher searcher(built.index);
            for (size_t row = 0; row < sa.size(); ++row) {
                size_t steps = 0;
                ASSERT_EQ(searcher.locateRow(row, &steps), sa[row]);
                ASSERT_LT(steps, k);
            }
        }
    }
}

TEST(Locate, SmallerIntervalNeedsFewerStepsButMoreMemory) {
    std::mt19937 rng(2);
    const std::string text = testing_support::randomText(rng, 20000, 6);
    const FMIndex dense = testing_support::makeIndex(text, {32, 4, RankKind::Checkpoint});
    const FMIndex sparse = testing_support::makeIndex(text, {32, 128, RankKind::Checkpoint});
    EXPECT_GT(dense.sampleBytes(), sparse.sampleBytes());
    const auto denseResult = FMIndexSearcher(dense).search("ab");
    const auto sparseResult = FMIndexSearcher(sparse).search("ab");
    EXPECT_EQ(denseResult.positions, sparseResult.positions);
    EXPECT_LT(denseResult.lfSteps, sparseResult.lfSteps);
}

TEST(Locate, MaxResultsLimitsWorkButNotCount) {
    const FMIndex index = testing_support::makeIndex(std::string(1000, 'a'));
    const SearchResult result = FMIndexSearcher(index).search("a", 10);
    EXPECT_EQ(result.count, 1000u);
    EXPECT_EQ(result.positions.size(), 10u);
    EXPECT_TRUE(std::is_sorted(result.positions.begin(), result.positions.end()));
}

TEST(Locate, TraceEndsAtSampledRow) {
    const FMIndex index = testing_support::makeIndex("banana", {2, 3, RankKind::Checkpoint});
    const FMIndexSearcher searcher(index);
    const auto rows = searcher.traceLocate(searcher.searchInterval("nan").begin);
    EXPECT_TRUE(index.samples().contains(rows.back()));
    for (size_t i = 0; i + 1 < rows.size(); ++i) {
        EXPECT_FALSE(index.samples().contains(rows[i]));
    }
}
