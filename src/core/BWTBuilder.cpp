#include "core/BWTBuilder.h"

#include "core/Types.h"

namespace fmi {

BWT BWTBuilder::build(const std::vector<uint16_t>& symbols, const std::vector<uint32_t>& suffixArray) {
    const size_t n = symbols.size();
    if (suffixArray.size() != n || n == 0) {
        throw IndexError("The suffix array does not match the text.");
    }
    BWT bwt;
    bwt.data.resize(n);
    for (size_t i = 0; i < n; ++i) {
        size_t previous = suffixArray[i] == 0 ? n - 1 : suffixArray[i] - 1;
        uint16_t symbol = symbols[previous];
        if (symbol == 0) {
            bwt.dollarRow = i;
            bwt.data[i] = 0;
        } else {
            bwt.data[i] = static_cast<uint8_t>(symbol - 1);
        }
    }
    return bwt;
}

std::vector<uint16_t> BWTBuilder::symbols(const BWT& bwt) {
    std::vector<uint16_t> result(bwt.size());
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = bwt.symbol(i);
    }
    return result;
}

}
