#pragma once

#include <limits>
#include <string_view>
#include <vector>

#include "core/FMIndex.h"

namespace fmi {

struct Interval {
    size_t begin = 0;
    size_t end = 0;

    bool empty() const { return end <= begin; }
    size_t size() const { return empty() ? 0 : end - begin; }
};

struct Occurrence {
    size_t offset = 0;
    size_t lfSteps = 0;
};

struct SearchResult {
    size_t count = 0;
    std::vector<size_t> positions;
    uint64_t lfSteps = 0;
};

class FMIndexSearcher {
public:
    static constexpr size_t kAll = std::numeric_limits<size_t>::max();

    explicit FMIndexSearcher(const FMIndex& index) : index_(index) {}
    explicit FMIndexSearcher(FMIndex&&) = delete;

    Interval searchInterval(std::string_view pattern) const;
    size_t count(std::string_view pattern) const;
    std::vector<size_t> locate(std::string_view pattern, size_t maxResults = kAll) const;
    SearchResult search(std::string_view pattern, size_t maxResults = kAll) const;

    std::vector<Occurrence> locateInterval(Interval interval, size_t maxResults = kAll) const;
    size_t locateRow(size_t row, size_t* steps = nullptr) const;

    std::vector<Interval> traceBackwardSearch(std::string_view pattern) const;
    std::vector<size_t> traceLocate(size_t row) const;

private:
    const FMIndex& index_;
};

}
