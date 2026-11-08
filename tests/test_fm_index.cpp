#include <gtest/gtest.h>

#include "TestSupport.h"

using namespace fmi;
using testing_support::makeIndex;
using testing_support::naiveLocate;

TEST(FMIndex, BananaAna) {
    for (const IndexConfig& config : testing_support::allConfigs()) {
        const FMIndex index = makeIndex("banana", config);
        const FMIndexSearcher searcher(index);
        EXPECT_EQ(searcher.count("ana"), 2u);
        EXPECT_EQ(searcher.locate("ana"), (std::vector<size_t>{1, 3}));
    }
}

TEST(FMIndex, BackwardSearchIntervalsOfBanana) {
    const FMIndex index = makeIndex("banana");
    const auto trace = FMIndexSearcher(index).traceBackwardSearch("ana");
    ASSERT_EQ(trace.size(), 4u);
    EXPECT_EQ(trace[0].begin, 0u);
    EXPECT_EQ(trace[0].end, 7u);
    EXPECT_EQ(trace[1].size(), 3u);
    EXPECT_EQ(trace[2].size(), 2u);
    EXPECT_EQ(trace[3].size(), 2u);
    EXPECT_EQ(trace[3].begin, 2u);
    EXPECT_EQ(trace[3].end, 4u);
}

TEST(FMIndex, MississippiAllSubstringsMatchNaive) {
    const std::string text = "mississippi";
    for (const IndexConfig& config : testing_support::allConfigs()) {
        const FMIndex index = makeIndex(text, config);
        const FMIndexSearcher searcher(index);
        for (size_t i = 0; i < text.size(); ++i) {
            for (size_t len = 1; i + len <= text.size(); ++len) {
                const std::string pattern = text.substr(i, len);
                ASSERT_EQ(searcher.locate(pattern), naiveLocate(text, pattern)) << pattern;
                ASSERT_EQ(searcher.count(pattern), naiveLocate(text, pattern).size());
            }
        }
    }
}

TEST(FMIndex, OverlappingMatchesAreReported) {
    const FMIndex index = makeIndex("aaaaaa");
    const FMIndexSearcher searcher(index);
    EXPECT_EQ(searcher.count("aaa"), 4u);
}

TEST(FMIndex, OverlappingPositions) {
    const FMIndex index = makeIndex("aaaaaa");
    EXPECT_EQ(FMIndexSearcher(index).locate("aaa"), (std::vector<size_t>{0, 1, 2, 3}));
}

TEST(FMIndex, MissingPatternHasNoMatches) {
    const FMIndex index = makeIndex("banana");
    const FMIndexSearcher searcher(index);
    EXPECT_EQ(searcher.count("nab"), 0u);
    EXPECT_TRUE(searcher.locate("nab").empty());
    EXPECT_EQ(searcher.count("z"), 0u);
    EXPECT_EQ(searcher.count("bananas"), 0u);
}

TEST(FMIndex, WholeTextMatchesOnceAtZero) {
    const FMIndex index = makeIndex("the quick brown fox");
    EXPECT_EQ(FMIndexSearcher(index).locate("the quick brown fox"), (std::vector<size_t>{0}));
}

TEST(FMIndex, EmptyDocumentIsHandled) {
    const FMIndex index = makeIndex("");
    const FMIndexSearcher searcher(index);
    EXPECT_EQ(index.textLength(), 0u);
    EXPECT_EQ(searcher.count("a"), 0u);
    EXPECT_TRUE(searcher.locate("a").empty());
}

TEST(FMIndex, EmptyPatternReturnsNothing) {
    const FMIndex index = makeIndex("banana");
    const FMIndexSearcher searcher(index);
    EXPECT_EQ(searcher.count(""), 0u);
    EXPECT_TRUE(searcher.locate("").empty());
    EXPECT_TRUE(searcher.searchInterval("").empty());
}

TEST(FMIndex, LiteralDollarIsOrdinarySymbol) {
    const std::string text = "cost$5 and $10 for a$";
    for (const IndexConfig& config : testing_support::allConfigs()) {
        const FMIndex index = makeIndex(text, config);
        const FMIndexSearcher searcher(index);
        EXPECT_EQ(searcher.locate("$"), naiveLocate(text, "$"));
        EXPECT_EQ(searcher.locate("a$"), naiveLocate(text, "a$"));
        EXPECT_EQ(searcher.count("$1"), 1u);
    }
}

TEST(FMIndex, BinaryBytesAreSupported) {
    std::string text;
    for (int i = 0; i < 256; ++i) {
        text.push_back(static_cast<char>(i));
    }
    text += text;
    for (RankKind kind : {RankKind::Checkpoint, RankKind::WaveletTree}) {
        const FMIndex index = makeIndex(text, {16, 8, kind});
        const FMIndexSearcher searcher(index);
        EXPECT_EQ(searcher.locate(std::string("\xff\x00\x01", 3)), (std::vector<size_t>{255}));
        EXPECT_EQ(searcher.count(std::string(1, '\0')), 2u);
    }
}

TEST(FMIndex, Utf8PatternsAreMatchedAsBytes) {
    const std::string text = "caf\xc3\xa9 au lait, caf\xc3\xa9 noir";
    const FMIndex index = makeIndex(text);
    const FMIndexSearcher searcher(index);
    EXPECT_EQ(searcher.locate("caf\xc3\xa9"), (std::vector<size_t>{0, 15}));
}

TEST(FMIndex, LfMappingMatchesSuffixArray) {
    const std::string text = "abracadabra";
    std::vector<uint32_t> sa;
    const BuildResult built = FMIndexBuilder({4, 2, RankKind::Checkpoint}).build(text, &sa);
    for (size_t row = 0; row < sa.size(); ++row) {
        const size_t expected = sa[row] == 0 ? 0 : sa[row] - 1;
        const size_t next = built.index.lf(row);
        if (sa[row] == 0) {
            EXPECT_EQ(sa[next], sa.size() - 1);
        } else {
            EXPECT_EQ(sa[next], expected);
        }
    }
    EXPECT_THROW(built.index.lf(sa.size()), std::out_of_range);
}

TEST(FMIndex, RandomizedAgreementWithNaiveSearch) {
    std::mt19937 rng(99);
    for (int trial = 0; trial < 60; ++trial) {
        const int alphabet = 1 + static_cast<int>(rng() % 5);
        const std::string text = testing_support::randomText(rng, rng() % 300, alphabet);
        const IndexConfig config{1u + static_cast<uint32_t>(rng() % 40), 1u + static_cast<uint32_t>(rng() % 40), trial % 2 ? RankKind::WaveletTree : RankKind::Checkpoint};
        const FMIndex index = makeIndex(text, config);
        const FMIndexSearcher searcher(index);
        for (int q = 0; q < 40; ++q) {
            const std::string pattern = testing_support::randomText(rng, 1 + rng() % 6, alphabet + 1);
            ASSERT_EQ(searcher.locate(pattern), naiveLocate(text, pattern)) << text << " / " << pattern;
            ASSERT_EQ(searcher.count(pattern), naiveLocate(text, pattern).size());
        }
    }
}

TEST(FMIndex, BuilderRejectsInvalidConfig) {
    EXPECT_THROW(FMIndexBuilder({0, 32, RankKind::Checkpoint}), IndexError);
    EXPECT_THROW(FMIndexBuilder({32, 0, RankKind::Checkpoint}), IndexError);
}

TEST(FMIndex, BuilderCanBeCancelled) {
    BuildHooks hooks;
    hooks.isCancelled = [] { return true; };
    EXPECT_THROW(FMIndexBuilder({}).build("banana", nullptr, hooks), OperationCancelled);
}

TEST(FMIndex, BuildStatsAreRecorded) {
    const BuildResult built = FMIndexBuilder({}).build(std::string(10000, 'x') + "end");
    EXPECT_GT(built.stats.totalSeconds, 0.0);
    EXPECT_GE(built.stats.totalSeconds, built.stats.saSeconds);
    EXPECT_GT(built.stats.estimatedPeakBytes, 10000u);
}
