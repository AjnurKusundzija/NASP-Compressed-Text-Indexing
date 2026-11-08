#include "core/RankSupport.h"

#include <algorithm>

#include "core/WaveletTree.h"

namespace fmi {

CheckpointRank::CheckpointRank(BWT bwt, size_t alphabetSize, uint32_t interval)
    : bwt_(std::move(bwt)), letters_(alphabetSize - 1), interval_(interval) {
    if (interval_ == 0 || alphabetSize == 0) {
        throw IndexError("Invalid rank structure parameters.");
    }
    const size_t n = bwt_.size();
    checkpoints_.resize((n / interval_ + 1) * letters_);
    std::vector<uint32_t> running(letters_, 0);
    for (size_t i = 0;; ++i) {
        if (i % interval_ == 0) {
            std::copy(running.begin(), running.end(), checkpoints_.begin() + static_cast<std::ptrdiff_t>(i / interval_ * letters_));
        }
        if (i == n) {
            break;
        }
        if (i != bwt_.dollarRow) {
            ++running[bwt_.data[i]];
        }
    }
}

size_t CheckpointRank::rank(uint16_t symbol, size_t i) const {
    if (symbol == 0) {
        return i > bwt_.dollarRow ? 1 : 0;
    }
    const uint8_t code = static_cast<uint8_t>(symbol - 1);
    const size_t block = i / interval_;
    size_t result = checkpoints_[block * letters_ + code];
    for (size_t j = block * interval_; j < i; ++j) {
        result += (bwt_.data[j] == code) & (j != bwt_.dollarRow);
    }
    return result;
}

size_t CheckpointRank::sizeInBytes() const {
    return bwt_.data.size() + checkpoints_.size() * sizeof(uint32_t);
}

FullOccTable::FullOccTable(const BWT& bwt, size_t alphabetSize)
    : size_(bwt.size()), symbols_(BWTBuilder::symbols(bwt)), table_(alphabetSize * (bwt.size() + 1), 0) {
    for (size_t c = 0; c < alphabetSize; ++c) {
        for (size_t i = 0; i < size_; ++i) {
            table_[c * (size_ + 1) + i + 1] = table_[c * (size_ + 1) + i] + (symbols_[i] == c ? 1 : 0);
        }
    }
}

size_t FullOccTable::sizeInBytes() const {
    return symbols_.size() * sizeof(uint16_t) + table_.size() * sizeof(uint32_t);
}

std::unique_ptr<RankSupport> makeRankSupport(RankKind kind, BWT bwt, size_t alphabetSize, uint32_t occSample) {
    if (kind == RankKind::WaveletTree) {
        return std::make_unique<WaveletTree>(BWTBuilder::symbols(bwt), alphabetSize);
    }
    return std::make_unique<CheckpointRank>(std::move(bwt), alphabetSize, occSample);
}

}
