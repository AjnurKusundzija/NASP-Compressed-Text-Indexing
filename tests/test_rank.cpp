#include <gtest/gtest.h>

#include "TestSupport.h"
#include "core/Alphabet.h"
#include "core/BWTBuilder.h"
#include "core/RankSupport.h"
#include "core/SuffixArrayBuilder.h"
#include "core/WaveletTree.h"

using namespace fmi;

namespace {

BWT bwtOf(const std::string& text, size_t* sigma) {
    const Alphabet alphabet = Alphabet::fromText(text);
    *sigma = alphabet.size();
    const auto symbols = alphabet.encodeText(text);
    return BWTBuilder::build(symbols, SuffixArrayBuilder::build(symbols, *sigma));
}

size_t directRank(const std::vector<uint16_t>& symbols, uint16_t symbol, size_t i) {
    return static_cast<size_t>(std::count(symbols.begin(), symbols.begin() + static_cast<std::ptrdiff_t>(i), symbol));
}

}

TEST(Rank, BananaValues) {
    size_t sigma = 0;
    const BWT bwt = bwtOf("banana", &sigma);
    const FullOccTable full(bwt, sigma);
    EXPECT_EQ(full.rank(1, 7), 3u);
    EXPECT_EQ(full.rank(3, 3), 2u);
    EXPECT_EQ(full.rank(3, 2), 1u);
    EXPECT_EQ(full.rank(0, 4), 0u);
    EXPECT_EQ(full.rank(0, 5), 1u);
    EXPECT_EQ(full.rank(2, 0), 0u);
}

TEST(Rank, AllRepresentationsAgreeWithDirectCount) {
    std::mt19937 rng(3);
    for (int trial = 0; trial < 25; ++trial) {
        const std::string text = testing_support::randomText(rng, rng() % 400, 1 + static_cast<int>(rng() % 6));
        size_t sigma = 0;
        const BWT bwt = bwtOf(text, &sigma);
        const auto symbols = BWTBuilder::symbols(bwt);
        const FullOccTable full(bwt, sigma);
        const WaveletTree wavelet(symbols, sigma);
        for (uint32_t k : {1u, 2u, 16u, 32u, 64u, 128u}) {
            const CheckpointRank checkpoint(bwt, sigma, k);
            for (size_t i = 0; i <= symbols.size(); ++i) {
                for (uint16_t c = 0; c < sigma; ++c) {
                    const size_t expected = directRank(symbols, c, i);
                    ASSERT_EQ(checkpoint.rank(c, i), expected) << "k=" << k << " c=" << c << " i=" << i;
                    ASSERT_EQ(wavelet.rank(c, i), expected);
                    ASSERT_EQ(full.rank(c, i), expected);
                }
            }
        }
    }
}

TEST(Rank, AccessReturnsBwtSymbols) {
    size_t sigma = 0;
    const BWT bwt = bwtOf("mississippi", &sigma);
    const auto symbols = BWTBuilder::symbols(bwt);
    const CheckpointRank checkpoint(bwt, sigma, 4);
    const WaveletTree wavelet(symbols, sigma);
    for (size_t i = 0; i < symbols.size(); ++i) {
        EXPECT_EQ(checkpoint.at(i), symbols[i]);
        EXPECT_EQ(wavelet.at(i), symbols[i]);
    }
}

TEST(Rank, RejectsZeroInterval) {
    size_t sigma = 0;
    const BWT bwt = bwtOf("abc", &sigma);
    EXPECT_THROW(CheckpointRank(bwt, sigma, 0), IndexError);
}

TEST(Rank, WaveletIsSmallerThanFullCheckpointsForLargeAlphabets) {
    std::mt19937 rng(5);
    const std::string text = testing_support::randomText(rng, 20000, 26);
    size_t sigma = 0;
    const BWT bwt = bwtOf(text, &sigma);
    const CheckpointRank checkpoint(bwt, sigma, 32);
    const WaveletTree wavelet(BWTBuilder::symbols(bwt), sigma);
    EXPECT_LT(wavelet.sizeInBytes(), checkpoint.sizeInBytes());
}
