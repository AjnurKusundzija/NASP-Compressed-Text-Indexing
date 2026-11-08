#include "core/SuffixArrayBuilder.h"

#include <algorithm>
#include <numeric>

#include "core/Types.h"

namespace fmi {

void SuffixArrayBuilder::validate(const std::vector<uint16_t>& symbols) {
    if (symbols.empty() || symbols.size() > kMaxSymbols) {
        throw IndexError("The input length is not supported by the suffix array builder.");
    }
    if (symbols.back() != 0) {
        throw IndexError("The input must end with the sentinel symbol.");
    }
    if (std::count(symbols.begin(), symbols.end(), uint16_t{0}) != 1) {
        throw IndexError("The sentinel symbol must be unique.");
    }
}

std::vector<uint32_t> SuffixArrayBuilder::buildNaive(const std::vector<uint16_t>& symbols) {
    validate(symbols);
    std::vector<uint32_t> sa(symbols.size());
    std::iota(sa.begin(), sa.end(), 0u);
    std::sort(sa.begin(), sa.end(), [&](uint32_t a, uint32_t b) {
        return std::lexicographical_compare(symbols.begin() + a, symbols.end(), symbols.begin() + b, symbols.end());
    });
    return sa;
}

std::vector<uint32_t> SuffixArrayBuilder::build(const std::vector<uint16_t>& symbols, size_t alphabetSize) {
    validate(symbols);
    const size_t n = symbols.size();
    std::vector<uint32_t> order(n), shifted(n), classes(n), nextClasses(n);
    std::vector<uint32_t> counts(std::max(n, alphabetSize), 0);

    for (uint16_t s : symbols) {
        ++counts[s];
    }
    for (size_t i = 1; i < alphabetSize; ++i) {
        counts[i] += counts[i - 1];
    }
    for (size_t i = n; i-- > 0;) {
        order[--counts[symbols[i]]] = static_cast<uint32_t>(i);
    }

    size_t classCount = 1;
    classes[order[0]] = 0;
    for (size_t i = 1; i < n; ++i) {
        if (symbols[order[i]] != symbols[order[i - 1]]) {
            ++classCount;
        }
        classes[order[i]] = static_cast<uint32_t>(classCount - 1);
    }

    for (size_t length = 1; length < n && classCount < n; length <<= 1) {
        for (size_t i = 0; i < n; ++i) {
            shifted[i] = order[i] >= length ? order[i] - static_cast<uint32_t>(length)
                                            : order[i] + static_cast<uint32_t>(n - length);
        }
        std::fill(counts.begin(), counts.begin() + static_cast<std::ptrdiff_t>(classCount), 0u);
        for (size_t i = 0; i < n; ++i) {
            ++counts[classes[shifted[i]]];
        }
        for (size_t i = 1; i < classCount; ++i) {
            counts[i] += counts[i - 1];
        }
        for (size_t i = n; i-- > 0;) {
            order[--counts[classes[shifted[i]]]] = shifted[i];
        }

        auto second = [&](uint32_t p) {
            size_t q = p + length;
            return classes[q >= n ? q - n : q];
        };
        classCount = 1;
        nextClasses[order[0]] = 0;
        for (size_t i = 1; i < n; ++i) {
            uint32_t a = order[i];
            uint32_t b = order[i - 1];
            if (classes[a] != classes[b] || second(a) != second(b)) {
                ++classCount;
            }
            nextClasses[a] = static_cast<uint32_t>(classCount - 1);
        }
        classes.swap(nextClasses);
    }
    return order;
}

size_t SuffixArrayBuilder::estimatePeakBytes(size_t symbolCount) {
    return symbolCount * (sizeof(uint16_t) + 5 * sizeof(uint32_t));
}

}
