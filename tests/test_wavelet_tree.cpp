#include <gtest/gtest.h>

#include "TestSupport.h"
#include "core/BitVector.h"
#include "core/WaveletTree.h"

using namespace fmi;

namespace {

BitVector randomBits(std::mt19937& rng, size_t size, unsigned density, std::vector<bool>* reference) {
    BitVector bits(size);
    reference->assign(size, false);
    for (size_t i = 0; i < size; ++i) {
        if (rng() % 100 < density) {
            bits.set(i);
            (*reference)[i] = true;
        }
    }
    bits.build();
    return bits;
}

}

TEST(BitVector, RankAndSelectMatchReference) {
    std::mt19937 rng(21);
    for (size_t size : {0u, 1u, 63u, 64u, 65u, 511u, 512u, 513u, 1024u, 5000u}) {
        for (unsigned density : {0u, 10u, 50u, 95u, 100u}) {
            std::vector<bool> reference;
            const BitVector bits = randomBits(rng, size, density, &reference);
            size_t ones = 0;
            size_t zeros = 0;
            for (size_t i = 0; i <= size; ++i) {
                ASSERT_EQ(bits.rank1(i), ones) << "size=" << size << " i=" << i;
                ASSERT_EQ(bits.rank0(i), zeros);
                if (i < size) {
                    ASSERT_EQ(bits.get(i), reference[i]);
                    if (reference[i]) {
                        ASSERT_EQ(bits.select1(ones), i);
                        ++ones;
                    } else {
                        ASSERT_EQ(bits.select0(zeros), i);
                        ++zeros;
                    }
                }
            }
            EXPECT_EQ(bits.select1(ones), size);
            EXPECT_EQ(bits.select0(zeros), size);
            EXPECT_EQ(bits.ones(), ones);
        }
    }
}

TEST(BitVector, RankSelectAreInverse) {
    std::mt19937 rng(8);
    std::vector<bool> reference;
    const BitVector bits = randomBits(rng, 3000, 30, &reference);
    for (size_t j = 0; j < bits.ones(); ++j) {
        EXPECT_EQ(bits.rank1(bits.select1(j)), j);
    }
    for (size_t j = 0; j < bits.size() - bits.ones(); ++j) {
        EXPECT_EQ(bits.rank0(bits.select0(j)), j);
    }
}

TEST(BitVector, FromWordsValidatesInput) {
    EXPECT_THROW(BitVector::fromWords(10, {0, 0}), IndexError);
    EXPECT_THROW(BitVector::fromWords(10, {~uint64_t{0}}), IndexError);
    EXPECT_NO_THROW(BitVector::fromWords(10, {0x3FF}));
}

TEST(WaveletTree, RankAndAccessMatchDirectCount) {
    std::mt19937 rng(14);
    for (size_t sigma : {1u, 2u, 3u, 7u, 40u, 257u}) {
        std::vector<uint16_t> symbols(300);
        for (auto& s : symbols) {
            s = static_cast<uint16_t>(rng() % sigma);
        }
        const WaveletTree tree(symbols, sigma);
        for (size_t i = 0; i < symbols.size(); ++i) {
            ASSERT_EQ(tree.at(i), symbols[i]);
        }
        for (uint16_t c = 0; c < sigma; c += (sigma > 40 ? 13 : 1)) {
            for (size_t i = 0; i <= symbols.size(); i += 7) {
                const size_t expected = static_cast<size_t>(std::count(symbols.begin(), symbols.begin() + static_cast<std::ptrdiff_t>(i), c));
                ASSERT_EQ(tree.rank(c, i), expected);
            }
        }
    }
}

TEST(WaveletTree, RejectsSymbolsOutsideAlphabet) {
    EXPECT_THROW(WaveletTree({0, 5}, 3), IndexError);
    EXPECT_THROW(WaveletTree({}, 0), IndexError);
}
