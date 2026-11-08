#pragma once

#include <string_view>
#include <vector>

namespace fmi {

class NaiveSearcher {
public:
    explicit NaiveSearcher(std::string_view text) : text_(text) {}

    size_t count(std::string_view pattern) const;
    std::vector<size_t> locate(std::string_view pattern, size_t maxResults = static_cast<size_t>(-1)) const;

private:
    std::string_view text_;
};

}
