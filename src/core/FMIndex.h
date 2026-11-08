#pragma once

#include <memory>
#include <vector>

#include "core/Alphabet.h"
#include "core/RankSupport.h"
#include "core/SampledSuffixArray.h"
#include "core/Types.h"

namespace fmi {

class FMIndex {
public:
    FMIndex(Alphabet alphabet, std::vector<size_t> c, std::unique_ptr<RankSupport> rank,
            SampledSuffixArray samples, IndexConfig config);

    size_t length() const { return rank_->size(); }
    size_t textLength() const { return length() - 1; }

    const Alphabet& alphabet() const { return alphabet_; }
    const IndexConfig& config() const { return config_; }
    const std::vector<size_t>& cArray() const { return c_; }
    const RankSupport& rankSupport() const { return *rank_; }
    const SampledSuffixArray& samples() const { return samples_; }

    size_t c(uint16_t symbol) const { return c_[symbol]; }
    size_t rank(uint16_t symbol, size_t i) const { return rank_->rank(symbol, i); }
    uint16_t bwtSymbol(size_t row) const { return rank_->at(row); }
    size_t lf(size_t row) const;

    size_t rankBytes() const { return rank_->sizeInBytes(); }
    size_t sampleBytes() const { return samples_.sizeInBytes(); }
    size_t sizeInBytes() const;

private:
    Alphabet alphabet_;
    std::vector<size_t> c_;
    std::unique_ptr<RankSupport> rank_;
    SampledSuffixArray samples_;
    IndexConfig config_;
};

}
