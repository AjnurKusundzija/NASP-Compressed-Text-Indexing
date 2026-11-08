#include <gtest/gtest.h>

#include "TestSupport.h"
#include "core/BWTBuilder.h"
#include "core/SuffixArrayBuilder.h"

using namespace fmi;

namespace {

std::string render(const BWT& bwt, const Alphabet& alphabet) {
    std::string text;
    for (size_t i = 0; i < bwt.size(); ++i) {
        const uint16_t symbol = bwt.symbol(i);
        text += symbol == 0 ? '$' : static_cast<char>(alphabet.decode(symbol));
    }
    return text;
}

}

TEST(BWT, BananaTransform) {
    const Alphabet alphabet = Alphabet::fromText("banana");
    const auto symbols = alphabet.encodeText("banana");
    const auto sa = SuffixArrayBuilder::build(symbols, alphabet.size());
    EXPECT_EQ(render(BWTBuilder::build(symbols, sa), alphabet), "annb$aa");
}

TEST(BWT, DollarRowIsRowOfFullText) {
    const Alphabet alphabet = Alphabet::fromText("banana");
    const auto symbols = alphabet.encodeText("banana");
    const auto sa = SuffixArrayBuilder::build(symbols, alphabet.size());
    const BWT bwt = BWTBuilder::build(symbols, sa);
    EXPECT_EQ(sa[bwt.dollarRow], 0u);
}

TEST(BWT, IsInvertibleThroughLF) {
    std::mt19937 rng(11);
    for (int trial = 0; trial < 50; ++trial) {
        const std::string text = testing_support::randomText(rng, 1 + rng() % 120, 4);
        const fmi::FMIndex index = testing_support::makeIndex(text);
        std::string rebuilt(text.size(), '\0');
        size_t row = 0;
        for (size_t i = text.size(); i-- > 0;) {
            rebuilt[i] = static_cast<char>(index.alphabet().decode(index.bwtSymbol(row)));
            row = index.lf(row);
        }
        EXPECT_EQ(rebuilt, text);
    }
}

TEST(BWT, CArrayOfBanana) {
    const FMIndex index = testing_support::makeIndex("banana");
    const Alphabet& alphabet = index.alphabet();
    EXPECT_EQ(index.c(0), 0u);
    EXPECT_EQ(index.c(alphabet.encode('a')), 1u);
    EXPECT_EQ(index.c(alphabet.encode('b')), 4u);
    EXPECT_EQ(index.c(alphabet.encode('n')), 5u);
    EXPECT_EQ(index.cArray().back(), 7u);
}
