#include "benchmark/SuffixArraySearcher.h"

#include <algorithm>
#include <cstring>

#include "core/Alphabet.h"
#include "core/SuffixArrayBuilder.h"

namespace fmi {

SuffixArraySearcher::SuffixArraySearcher(std::string_view text) : text_(text) {
    const Alphabet alphabet = Alphabet::fromText(text);
    suffixes_ = SuffixArrayBuilder::build(alphabet.encodeText(text), alphabet.size());
    suffixes_.erase(suffixes_.begin());
}

int SuffixArraySearcher::compare(uint32_t suffix, std::string_view pattern) const {
    const size_t length = std::min(pattern.size(), text_.size() - suffix);
    const int result = std::memcmp(text_.data() + suffix, pattern.data(), length);
    if (result != 0) {
        return result;
    }
    return length < pattern.size() ? -1 : 0;
}

SuffixArraySearcher::Range SuffixArraySearcher::range(std::string_view pattern) const {
    if (pattern.empty()) {
        return {0, 0};
    }
    const auto first = std::partition_point(suffixes_.begin(), suffixes_.end(),
                                            [&](uint32_t suffix) { return compare(suffix, pattern) < 0; });
    const auto last = std::partition_point(first, suffixes_.end(),
                                           [&](uint32_t suffix) { return compare(suffix, pattern) == 0; });
    return {static_cast<size_t>(first - suffixes_.begin()), static_cast<size_t>(last - suffixes_.begin())};
}

size_t SuffixArraySearcher::count(std::string_view pattern) const {
    const Range r = range(pattern);
    return r.end - r.begin;
}

std::vector<size_t> SuffixArraySearcher::locate(std::string_view pattern, size_t maxResults) const {
    const Range r = range(pattern);
    const size_t limit = std::min(r.end - r.begin, maxResults);
    std::vector<size_t> positions(suffixes_.begin() + static_cast<std::ptrdiff_t>(r.begin),
                                  suffixes_.begin() + static_cast<std::ptrdiff_t>(r.begin + limit));
    std::sort(positions.begin(), positions.end());
    return positions;
}

}
