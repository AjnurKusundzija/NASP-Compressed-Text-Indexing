#include "core/SampledSuffixArray.h"

#include "core/Types.h"

namespace fmi {

SampledSuffixArray::SampledSuffixArray(const std::vector<uint32_t>& suffixArray, uint32_t interval)
    : interval_(interval), marked_(suffixArray.size()) {
    if (interval == 0) {
        throw IndexError("The suffix array sampling interval must be positive.");
    }
    for (size_t row = 0; row < suffixArray.size(); ++row) {
        if (suffixArray[row] % interval == 0) {
            marked_.set(row);
            values_.push_back(suffixArray[row] / interval);
        }
    }
    marked_.build();
}

SampledSuffixArray::SampledSuffixArray(uint32_t interval, BitVector marked, std::vector<uint32_t> values)
    : interval_(interval), marked_(std::move(marked)), values_(std::move(values)) {
    if (interval_ == 0 || marked_.ones() != values_.size()) {
        throw IndexError("The sampled suffix array is inconsistent.");
    }
}

size_t SampledSuffixArray::sizeInBytes() const {
    return marked_.sizeInBytes() + values_.size() * sizeof(uint32_t);
}

}
