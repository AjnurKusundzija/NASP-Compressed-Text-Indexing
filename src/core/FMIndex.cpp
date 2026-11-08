#include "core/FMIndex.h"

#include <stdexcept>

namespace fmi {

FMIndex::FMIndex(Alphabet alphabet, std::vector<size_t> c, std::unique_ptr<RankSupport> rank,
                 SampledSuffixArray samples, IndexConfig config)
    : alphabet_(std::move(alphabet)), c_(std::move(c)), rank_(std::move(rank)),
      samples_(std::move(samples)), config_(config) {
    config_.validate();
    if (!rank_ || rank_->size() == 0) {
        throw IndexError("The index must contain at least the sentinel.");
    }
    if (c_.size() != alphabet_.size() + 1 || c_.back() != rank_->size()) {
        throw IndexError("The C array does not match the indexed text.");
    }
    if (samples_.marked().size() != rank_->size()) {
        throw IndexError("The sampled suffix array does not match the indexed text.");
    }
}

size_t FMIndex::lf(size_t row) const {
    if (row >= length()) {
        throw std::out_of_range("LF-mapping row is outside the index.");
    }
    const uint16_t symbol = rank_->at(row);
    return c_[symbol] + rank_->rank(symbol, row);
}

size_t FMIndex::sizeInBytes() const {
    return rankBytes() + sampleBytes() + c_.size() * sizeof(size_t) + alphabet_.bytes().size();
}

}
