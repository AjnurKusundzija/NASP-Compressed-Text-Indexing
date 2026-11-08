#include "core/BitVector.h"

#include <algorithm>
#include <bit>

#include "core/Types.h"

namespace fmi {

namespace {

size_t selectInWord(uint64_t word, size_t j) {
    for (size_t k = 0; k < j; ++k) {
        word &= word - 1;
    }
    return static_cast<size_t>(std::countr_zero(word));
}

}

BitVector::BitVector(size_t size) : size_(size), words_((size + 63) / 64, 0) {}

BitVector BitVector::fromWords(size_t size, std::vector<uint64_t> words) {
    if (words.size() != (size + 63) / 64) {
        throw IndexError("The bit vector has an invalid word count.");
    }
    if (size % 64 != 0 && (words.back() >> (size % 64)) != 0) {
        throw IndexError("The bit vector has non-zero padding bits.");
    }
    BitVector bits;
    bits.size_ = size;
    bits.words_ = std::move(words);
    bits.build();
    return bits;
}

void BitVector::set(size_t i) {
    words_[i >> 6] |= uint64_t{1} << (i & 63);
}

void BitVector::build() {
    blocks_.assign(words_.size() / kBlockWords + 1, 0);
    uint64_t total = 0;
    for (size_t w = 0; w <= words_.size(); ++w) {
        if (w % kBlockWords == 0) {
            blocks_[w / kBlockWords] = static_cast<uint32_t>(total);
        }
        if (w < words_.size()) {
            total += static_cast<uint64_t>(std::popcount(words_[w]));
        }
    }
    ones_ = total;
}

size_t BitVector::rank1(size_t i) const {
    const size_t word = i >> 6;
    size_t result = blocks_[word / kBlockWords];
    for (size_t w = word - word % kBlockWords; w < word; ++w) {
        result += static_cast<size_t>(std::popcount(words_[w]));
    }
    if (i & 63) {
        result += static_cast<size_t>(std::popcount(words_[word] & ((uint64_t{1} << (i & 63)) - 1)));
    }
    return result;
}

size_t BitVector::select1(size_t j) const {
    if (j >= ones_) {
        return size_;
    }
    size_t block = static_cast<size_t>(std::upper_bound(blocks_.begin(), blocks_.end(), j) - blocks_.begin()) - 1;
    size_t remaining = j - blocks_[block];
    for (size_t w = block * kBlockWords;; ++w) {
        size_t count = static_cast<size_t>(std::popcount(words_[w]));
        if (remaining < count) {
            return w * 64 + selectInWord(words_[w], remaining);
        }
        remaining -= count;
    }
}

size_t BitVector::select0(size_t j) const {
    if (j >= size_ - ones_) {
        return size_;
    }
    size_t low = 0;
    size_t high = blocks_.size();
    while (low < high) {
        size_t mid = (low + high) / 2;
        if (mid * kBlockBits - blocks_[mid] > j) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    size_t block = low - 1;
    size_t remaining = j - (block * kBlockBits - blocks_[block]);
    for (size_t w = block * kBlockWords;; ++w) {
        uint64_t word = ~words_[w];
        size_t count = static_cast<size_t>(std::popcount(word));
        if (remaining < count) {
            return w * 64 + selectInWord(word, remaining);
        }
        remaining -= count;
    }
}

size_t BitVector::sizeInBytes() const {
    return words_.size() * sizeof(uint64_t) + blocks_.size() * sizeof(uint32_t);
}

}
