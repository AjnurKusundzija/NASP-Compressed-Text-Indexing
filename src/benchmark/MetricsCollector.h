#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace fmi {

struct Summary {
    double median = 0;
    double minimum = 0;
    double maximum = 0;
};

class MetricsCollector {
public:
    static Summary summarize(std::vector<double> values);
    static uint64_t currentResidentBytes();
    static uint64_t peakResidentBytes();
    static std::vector<std::pair<std::string, std::string>> environment();
};

}
