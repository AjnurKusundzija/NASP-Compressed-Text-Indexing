#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fmi {

class BitVector {
public:
    BitVector() = default;
    explicit BitVector(size_t size);

    static BitVector fromWords(size_t size, std::vector<uint64_t> words);

    void set(size_t i);
    void build();

    size_t size() const { return size_; }
    size_t ones() const { return ones_; }
    bool get(size_t i) const { return (words_[i >> 6] >> (i & 63)) & 1; }

    size_t rank1(size_t i) const;
    size_t rank0(size_t i) const { return i - rank1(i); }
    size_t select1(size_t j) const;
    size_t select0(size_t j) const;

    const std::vector<uint64_t>& words() const { return words_; }
    size_t sizeInBytes() const;

private:
    static constexpr size_t kBlockWords = 8;
    static constexpr size_t kBlockBits = kBlockWords * 64;

    size_t size_ = 0;
    size_t ones_ = 0;
    std::vector<uint64_t> words_;
    std::vector<uint32_t> blocks_;
};

}
