#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "benchmark/MetricsCollector.h"
#include "core/Types.h"

namespace fmi {

struct BenchmarkSettings {
    std::vector<size_t> patternLengths{4, 8, 16, 32, 64};
    std::vector<uint32_t> samplingIntervals{16, 32, 64, 128};
    std::vector<double> sizeFractions{0.125, 0.25, 0.5, 1.0};
    size_t patternsPerLength = 10;
    size_t repetitions = 5;
    size_t buildRepetitions = 3;
    size_t locateLimit = 5000;
    uint64_t seed = 1;
};

struct BenchRow {
    std::string experiment;
    std::string series;
    double x = 0;
    double median = 0;
    double minimum = 0;
    double maximum = 0;
    std::string unit;
};

struct BenchmarkReport {
    std::vector<BenchRow> rows;
    std::vector<std::pair<std::string, std::string>> environment;
    std::vector<std::string> notes;

    std::vector<BenchRow> select(const std::string& experiment) const;
};

class BenchmarkRunner {
public:
    explicit BenchmarkRunner(BenchmarkSettings settings = {}, BuildHooks hooks = {})
        : settings_(std::move(settings)), hooks_(std::move(hooks)) {}

    BenchmarkReport run(std::string_view text) const;

    static void writeCsv(const BenchmarkReport& report, const std::string& path);
    static std::string generateLog(size_t bytes, uint64_t seed);

private:
    BenchmarkSettings settings_;
    BuildHooks hooks_;
};

}
