#include "core/FMIndexSearcher.h"

#include <algorithm>

namespace fmi {

Interval FMIndexSearcher::searchInterval(std::string_view pattern) const {
    if (pattern.empty()) {
        return {};
    }
    size_t begin = 0;
    size_t end = index_.length();
    for (auto it = pattern.rbegin(); it != pattern.rend(); ++it) {
        const uint16_t symbol = index_.alphabet().encode(static_cast<uint8_t>(*it));
        if (symbol == 0) {
            return {};
        }
        begin = index_.c(symbol) + index_.rank(symbol, begin);
        end = index_.c(symbol) + index_.rank(symbol, end);
        if (begin >= end) {
            return {};
        }
    }
    return {begin, end};
}

size_t FMIndexSearcher::count(std::string_view pattern) const {
    return searchInterval(pattern).size();
}

size_t FMIndexSearcher::locateRow(size_t row, size_t* steps) const {
    const size_t n = index_.length();
    size_t taken = 0;
    while (!index_.samples().contains(row)) {
        row = index_.lf(row);
        if (++taken > n) {
            throw IndexError("The index is corrupted: sampled suffix array walk did not terminate.");
        }
    }
    if (steps) {
        *steps = taken;
    }
    return (index_.samples().value(row) + taken) % n;
}

std::vector<Occurrence> FMIndexSearcher::locateInterval(Interval interval, size_t maxResults) const {
    std::vector<Occurrence> occurrences;
    const size_t limit = std::min(interval.size(), maxResults);
    occurrences.reserve(limit);
    for (size_t row = interval.begin; row < interval.begin + limit; ++row) {
        Occurrence occurrence;
        occurrence.offset = locateRow(row, &occurrence.lfSteps);
        if (occurrence.offset >= index_.textLength()) {
            throw IndexError("The index is corrupted: a match points to the sentinel.");
        }
        occurrences.push_back(occurrence);
    }
    return occurrences;
}

std::vector<size_t> FMIndexSearcher::locate(std::string_view pattern, size_t maxResults) const {
    std::vector<size_t> positions;
    for (const Occurrence& occurrence : locateInterval(searchInterval(pattern), maxResults)) {
        positions.push_back(occurrence.offset);
    }
    std::sort(positions.begin(), positions.end());
    return positions;
}

SearchResult FMIndexSearcher::search(std::string_view pattern, size_t maxResults) const {
    const Interval interval = searchInterval(pattern);
    SearchResult result;
    result.count = interval.size();
    std::vector<Occurrence> occurrences = locateInterval(interval, maxResults);
    std::sort(occurrences.begin(), occurrences.end(),
              [](const Occurrence& a, const Occurrence& b) { return a.offset < b.offset; });
    for (const Occurrence& occurrence : occurrences) {
        result.positions.push_back(occurrence.offset);
        result.lfSteps += occurrence.lfSteps;
    }
    return result;
}

std::vector<Interval> FMIndexSearcher::traceBackwardSearch(std::string_view pattern) const {
    std::vector<Interval> trace{{0, index_.length()}};
    Interval current = trace.back();
    for (auto it = pattern.rbegin(); it != pattern.rend(); ++it) {
        const uint16_t symbol = index_.alphabet().encode(static_cast<uint8_t>(*it));
        if (symbol == 0) {
            current = {};
        } else {
            current.begin = index_.c(symbol) + index_.rank(symbol, current.begin);
            current.end = index_.c(symbol) + index_.rank(symbol, current.end);
        }
        trace.push_back(current);
        if (current.empty()) {
            break;
        }
    }
    return trace;
}

std::vector<size_t> FMIndexSearcher::traceLocate(size_t row) const {
    std::vector<size_t> rows{row};
    while (!index_.samples().contains(rows.back())) {
        rows.push_back(index_.lf(rows.back()));
        if (rows.size() > index_.length() + 1) {
            throw IndexError("The index is corrupted: sampled suffix array walk did not terminate.");
        }
    }
    return rows;
}

}
