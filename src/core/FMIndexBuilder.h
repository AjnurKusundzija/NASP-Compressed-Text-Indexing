#pragma once

#include <string_view>
#include <vector>

#include "core/FMIndex.h"

namespace fmi {

struct BuildStats {
    double saSeconds = 0;
    double bwtSeconds = 0;
    double rankSeconds = 0;
    double sampleSeconds = 0;
    double totalSeconds = 0;
    uint64_t estimatedPeakBytes = 0;
};

struct BuildResult {
    FMIndex index;
    BuildStats stats;
};

class FMIndexBuilder {
public:
    explicit FMIndexBuilder(IndexConfig config);

    BuildResult build(std::string_view text, std::vector<uint32_t>* keepSuffixArray = nullptr,
                      const BuildHooks& hooks = {}) const;

    static std::vector<size_t> buildC(const std::vector<uint16_t>& symbols, size_t alphabetSize);

private:
    IndexConfig config_;
};

}
