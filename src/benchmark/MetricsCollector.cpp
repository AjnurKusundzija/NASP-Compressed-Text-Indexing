#include "benchmark/MetricsCollector.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#ifndef FMI_BUILD_INFO
#define FMI_BUILD_INFO "unknown"
#endif

namespace fmi {

namespace {

uint64_t procStatusKilobytes(const std::string& key) {
    std::ifstream in("/proc/self/status");
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind(key, 0) == 0) {
            std::istringstream fields(line.substr(key.size()));
            uint64_t value = 0;
            fields >> value;
            return value * 1024;
        }
    }
    return 0;
}

std::string firstMatch(const std::string& path, const std::string& key) {
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind(key, 0) == 0) {
            const size_t colon = line.find(':');
            return colon == std::string::npos ? line : line.substr(colon + 2);
        }
    }
    return "unknown";
}

}

Summary MetricsCollector::summarize(std::vector<double> values) {
    Summary summary;
    if (values.empty()) {
        return summary;
    }
    std::sort(values.begin(), values.end());
    const size_t mid = values.size() / 2;
    summary.median = values.size() % 2 ? values[mid] : (values[mid - 1] + values[mid]) / 2;
    summary.minimum = values.front();
    summary.maximum = values.back();
    return summary;
}

uint64_t MetricsCollector::currentResidentBytes() {
    return procStatusKilobytes("VmRSS:");
}

uint64_t MetricsCollector::peakResidentBytes() {
    return procStatusKilobytes("VmHWM:");
}

std::vector<std::pair<std::string, std::string>> MetricsCollector::environment() {
    std::string compiler;
#if defined(__clang__)
    compiler = "Clang " + std::string(__clang_version__);
#elif defined(__GNUC__)
    compiler = "GCC " + std::string(__VERSION__);
#else
    compiler = "unknown";
#endif
    return {
        {"CPU", firstMatch("/proc/cpuinfo", "model name")},
        {"Compiler", compiler},
        {"C++ standard", std::to_string(__cplusplus)},
        {"Build configuration", FMI_BUILD_INFO},
        {"Execution model", "serial, single thread"},
        {"Timer", "std::chrono::steady_clock"},
        {"Peak resident memory (bytes)", std::to_string(peakResidentBytes())},
    };
}

}
