#pragma once

#include <cstdint>
#include <vector>

#include "core/BitVector.h"

namespace fmi {

class SampledSuffixArray {
public:
    SampledSuffixArray() = default;
    SampledSuffixArray(const std::vector<uint32_t>& suffixArray, uint32_t interval);
    SampledSuffixArray(uint32_t interval, BitVector marked, std::vector<uint32_t> values);

    bool contains(size_t row) const { return marked_.get(row); }
    size_t value(size_t row) const { return static_cast<size_t>(values_[marked_.rank1(row)]) * interval_; }

    uint32_t interval() const { return interval_; }
    size_t sampleCount() const { return values_.size(); }
    const BitVector& marked() const { return marked_; }
    const std::vector<uint32_t>& values() const { return values_; }
    size_t sizeInBytes() const;

private:
    uint32_t interval_ = 1;
    BitVector marked_;
    std::vector<uint32_t> values_;
};

}
