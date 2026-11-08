#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fmi {

class SuffixArrayBuilder {
public:
    static std::vector<uint32_t> buildNaive(const std::vector<uint16_t>& symbols);
    static std::vector<uint32_t> build(const std::vector<uint16_t>& symbols, size_t alphabetSize);
    static size_t estimatePeakBytes(size_t symbolCount);

private:
    static void validate(const std::vector<uint16_t>& symbols);
};

}
