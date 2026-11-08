#pragma once

#include <cstdint>
#include <vector>

#include "core/BitVector.h"
#include "core/RankSupport.h"

namespace fmi {

class WaveletTree : public RankSupport {
public:
    WaveletTree(const std::vector<uint16_t>& symbols, size_t alphabetSize);

    size_t rank(uint16_t symbol, size_t i) const override;
    uint16_t at(size_t i) const override;
    size_t size() const override { return size_; }
    size_t sizeInBytes() const override;
    RankKind kind() const override { return RankKind::WaveletTree; }

    size_t nodeCount() const { return nodes_.size(); }

private:
    struct Node {
        BitVector bits;
        int left = -1;
        int right = -1;
    };

    int build(const std::vector<uint16_t>& symbols, size_t low, size_t high);

    size_t size_;
    size_t alphabetSize_;
    int root_ = -1;
    std::vector<Node> nodes_;
};

}
