#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace fmi {

class SuffixArraySearcher {
public:
    explicit SuffixArraySearcher(std::string_view text);

    size_t count(std::string_view pattern) const;
    std::vector<size_t> locate(std::string_view pattern, size_t maxResults = static_cast<size_t>(-1)) const;
    size_t sizeInBytes() const { return suffixes_.size() * sizeof(uint32_t); }

private:
    struct Range {
        size_t begin;
        size_t end;
    };

    int compare(uint32_t suffix, std::string_view pattern) const;
    Range range(std::string_view pattern) const;

    std::string_view text_;
    std::vector<uint32_t> suffixes_;
};

}
