#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fmi {

struct BWT {
    std::vector<uint8_t> data;
    size_t dollarRow = 0;

    size_t size() const { return data.size(); }
    uint16_t symbol(size_t i) const { return i == dollarRow ? uint16_t{0} : static_cast<uint16_t>(data[i] + 1); }
};

class BWTBuilder {
public:
    static BWT build(const std::vector<uint16_t>& symbols, const std::vector<uint32_t>& suffixArray);
    static std::vector<uint16_t> symbols(const BWT& bwt);
};

}
