#include <gtest/gtest.h>

#include "TestSupport.h"
#include "core/Alphabet.h"
#include "core/SuffixArrayBuilder.h"

using namespace fmi;

namespace {

std::vector<uint16_t> encode(const std::string& text) {
    return Alphabet::fromText(text).encodeText(text);
}

}

TEST(SuffixArray, BananaMatchesExpected) {
    const std::vector<uint32_t> expected{6, 5, 3, 1, 0, 4, 2};
    const auto symbols = encode("banana");
    EXPECT_EQ(SuffixArrayBuilder::buildNaive(symbols), expected);
    EXPECT_EQ(SuffixArrayBuilder::build(symbols, Alphabet::fromText("banana").size()), expected);
}

TEST(SuffixArray, EmptyTextHasOnlySentinel) {
    const auto symbols = encode("");
    EXPECT_EQ(SuffixArrayBuilder::build(symbols, 1), (std::vector<uint32_t>{0}));
}

TEST(SuffixArray, FastMatchesNaiveOnRandomInputs) {
    std::mt19937 rng(7);
    for (int alphabet : {1, 2, 3, 8}) {
        for (size_t length = 0; length < 150; ++length) {
            const std::string text = testing_support::randomText(rng, length, alphabet);
            const auto symbols = encode(text);
            ASSERT_EQ(SuffixArrayBuilder::build(symbols, Alphabet::fromText(text).size()), SuffixArrayBuilder::buildNaive(symbols))
                << text;
        }
    }
}

TEST(SuffixArray, RepetitiveInputIsSorted) {
    const std::string text(5000, 'a');
    const auto symbols = encode(text);
    const auto sa = SuffixArrayBuilder::build(symbols, 2);
    for (size_t i = 0; i < sa.size(); ++i) {
        EXPECT_EQ(sa[i], sa.size() - 1 - i);
    }
}

TEST(SuffixArray, RejectsMissingOrDuplicateSentinel) {
    EXPECT_THROW(SuffixArrayBuilder::build({1, 2, 1}, 3), IndexError);
    EXPECT_THROW(SuffixArrayBuilder::build({1, 0, 2, 0}, 3), IndexError);
    EXPECT_THROW(SuffixArrayBuilder::build({}, 1), IndexError);
}

TEST(SuffixArray, LiteralDollarDoesNotCollideWithSentinel) {
    const std::string text = "a$b$a$";
    const auto symbols = encode(text);
    EXPECT_EQ(SuffixArrayBuilder::build(symbols, Alphabet::fromText(text).size()), SuffixArrayBuilder::buildNaive(symbols));
}
