#include "core/WaveletTree.h"

#include <algorithm>

namespace fmi {

WaveletTree::WaveletTree(const std::vector<uint16_t>& symbols, size_t alphabetSize)
    : size_(symbols.size()), alphabetSize_(alphabetSize) {
    if (alphabetSize == 0 || alphabetSize > 257) {
        throw IndexError("Invalid alphabet size for the wavelet tree.");
    }
    if (std::any_of(symbols.begin(), symbols.end(), [&](uint16_t s) { return s >= alphabetSize; })) {
        throw IndexError("A symbol is outside the wavelet tree alphabet.");
    }
    root_ = build(symbols, 0, alphabetSize);
}

int WaveletTree::build(const std::vector<uint16_t>& symbols, size_t low, size_t high) {
    if (high - low <= 1) {
        return -1;
    }
    const int id = static_cast<int>(nodes_.size());
    nodes_.emplace_back();
    const size_t mid = (low + high) / 2;

    BitVector bits(symbols.size());
    std::vector<uint16_t> left;
    std::vector<uint16_t> right;
    for (size_t i = 0; i < symbols.size(); ++i) {
        if (symbols[i] >= mid) {
            bits.set(i);
            right.push_back(symbols[i]);
        } else {
            left.push_back(symbols[i]);
        }
    }
    bits.build();

    const int leftChild = build(left, low, mid);
    const int rightChild = build(right, mid, high);
    nodes_[static_cast<size_t>(id)].bits = std::move(bits);
    nodes_[static_cast<size_t>(id)].left = leftChild;
    nodes_[static_cast<size_t>(id)].right = rightChild;
    return id;
}

size_t WaveletTree::rank(uint16_t symbol, size_t i) const {
    size_t low = 0;
    size_t high = alphabetSize_;
    int node = root_;
    while (node >= 0) {
        const Node& current = nodes_[static_cast<size_t>(node)];
        const size_t mid = (low + high) / 2;
        if (symbol >= mid) {
            i = current.bits.rank1(i);
            node = current.right;
            low = mid;
        } else {
            i = current.bits.rank0(i);
            node = current.left;
            high = mid;
        }
    }
    return i;
}

uint16_t WaveletTree::at(size_t i) const {
    size_t low = 0;
    size_t high = alphabetSize_;
    int node = root_;
    while (node >= 0) {
        const Node& current = nodes_[static_cast<size_t>(node)];
        const size_t mid = (low + high) / 2;
        if (current.bits.get(i)) {
            i = current.bits.rank1(i);
            node = current.right;
            low = mid;
        } else {
            i = current.bits.rank0(i);
            node = current.left;
            high = mid;
        }
    }
    return static_cast<uint16_t>(low);
}

size_t WaveletTree::sizeInBytes() const {
    size_t total = nodes_.size() * sizeof(Node);
    for (const Node& node : nodes_) {
        total += node.bits.sizeInBytes();
    }
    return total;
}

}
