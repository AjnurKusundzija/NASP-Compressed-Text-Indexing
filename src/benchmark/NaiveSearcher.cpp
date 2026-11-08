#include "benchmark/NaiveSearcher.h"

namespace fmi {

size_t NaiveSearcher::count(std::string_view pattern) const {
    if (pattern.empty()) {
        return 0;
    }
    size_t found = 0;
    for (size_t pos = text_.find(pattern); pos != std::string_view::npos; pos = text_.find(pattern, pos + 1)) {
        ++found;
    }
    return found;
}

std::vector<size_t> NaiveSearcher::locate(std::string_view pattern, size_t maxResults) const {
    std::vector<size_t> positions;
    if (pattern.empty()) {
        return positions;
    }
    for (size_t pos = text_.find(pattern); pos != std::string_view::npos && positions.size() < maxResults;
         pos = text_.find(pattern, pos + 1)) {
        positions.push_back(pos);
    }
    return positions;
}

}
