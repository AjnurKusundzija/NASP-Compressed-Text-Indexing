#include "benchmark/BenchmarkRunner.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <optional>
#include <cstdio>
#include <random>

#include "benchmark/NaiveSearcher.h"
#include "benchmark/SuffixArraySearcher.h"
#include "core/Alphabet.h"
#include "core/FMIndexBuilder.h"
#include "core/FMIndexSearcher.h"
#include "core/Stopwatch.h"

namespace fmi {

namespace {

volatile size_t sink = 0;

struct Method {
    std::string name;
    std::function<size_t(std::string_view)> count;
    std::function<std::vector<size_t>(std::string_view, size_t)> locate;
};

struct PatternSet {
    std::vector<std::string> existing;
    std::vector<std::string> missing;
};

BenchRow makeRow(const std::string& experiment, const std::string& series, double x, const Summary& summary,
                 const std::string& unit) {
    return {experiment, series, x, summary.median, summary.minimum, summary.maximum, unit};
}

BenchRow makeRow(const std::string& experiment, const std::string& series, double x, double value, const std::string& unit) {
    return {experiment, series, x, value, value, value, unit};
}

template <class Work>
Summary measure(const std::vector<std::string>& patterns, size_t repetitions, Work&& work) {
    std::vector<double> perPattern;
    for (const std::string& pattern : patterns) {
        std::vector<double> samples;
        for (size_t r = 0; r < repetitions; ++r) {
            Stopwatch watch;
            sink = work(pattern);
            samples.push_back(watch.seconds() * 1e6);
        }
        perPattern.push_back(MetricsCollector::summarize(samples).median);
    }
    return MetricsCollector::summarize(perPattern);
}

PatternSet makePatterns(std::string_view text, size_t length, size_t count, std::mt19937_64& rng) {
    PatternSet set;
    if (length == 0 || length > text.size()) {
        return set;
    }
    const NaiveSearcher naive(text);
    const Alphabet alphabet = Alphabet::fromText(text);
    for (size_t i = 0; i < count; ++i) {
        const size_t start = rng() % (text.size() - length + 1);
        std::string pattern(text.substr(start, length));
        set.existing.push_back(pattern);
        for (int attempt = 0; attempt < 50; ++attempt) {
            std::string mutated = pattern;
            const size_t at = rng() % length;
            mutated[at] = static_cast<char>(alphabet.bytes()[rng() % alphabet.bytes().size()]);
            if (mutated != pattern && naive.count(mutated) == 0) {
                set.missing.push_back(std::move(mutated));
                break;
            }
        }
    }
    return set;
}

void verify(const std::vector<Method>& methods, const PatternSet& patterns, size_t locateLimit) {
    auto check = [&](const std::string& pattern) {
        const size_t expected = methods.front().count(pattern);
        for (const Method& method : methods) {
            if (method.count(pattern) != expected) {
                throw std::runtime_error("Benchmark correctness check failed: " + method.name + " disagrees on count.");
            }
        }
        if (expected > 0 && expected <= locateLimit) {
            const std::vector<size_t> reference = methods.front().locate(pattern, locateLimit);
            for (const Method& method : methods) {
                if (method.locate(pattern, locateLimit) != reference) {
                    throw std::runtime_error("Benchmark correctness check failed: " + method.name + " disagrees on positions.");
                }
            }
        }
    };
    for (const std::string& pattern : patterns.existing) check(pattern);
    for (const std::string& pattern : patterns.missing) check(pattern);
}

Method fmMethod(const std::string& name, const FMIndex& index) {
    return {name,
            [&index](std::string_view p) { return FMIndexSearcher(index).count(p); },
            [&index](std::string_view p, size_t limit) { return FMIndexSearcher(index).locate(p, limit); }};
}

struct Built {
    std::optional<FMIndex> index;
    std::vector<double> total;
    std::vector<double> suffixArray;
};

Built buildRepeated(std::string_view text, const IndexConfig& config, size_t repetitions, const BuildHooks& hooks) {
    Built built;
    for (size_t r = 0; r < std::max<size_t>(1, repetitions); ++r) {
        BuildResult result = FMIndexBuilder(config).build(text, nullptr, hooks);
        built.total.push_back(result.stats.totalSeconds);
        built.suffixArray.push_back(result.stats.saSeconds);
        built.index.emplace(std::move(result.index));
    }
    return built;
}

std::string quoted(const std::string& value) {
    std::string out = "\"";
    for (char ch : value) {
        out += ch == '"' ? "\"\"" : std::string(1, ch);
    }
    return out + "\"";
}

}

std::vector<BenchRow> BenchmarkReport::select(const std::string& experiment) const {
    std::vector<BenchRow> selected;
    std::copy_if(rows.begin(), rows.end(), std::back_inserter(selected),
                 [&](const BenchRow& row) { return row.experiment == experiment; });
    return selected;
}

BenchmarkReport BenchmarkRunner::run(std::string_view text) const {
    if (text.empty()) {
        throw IndexError("The benchmark needs a non-empty text.");
    }
    BenchmarkReport report;
    report.environment = MetricsCollector::environment();
    report.environment.emplace_back("Text size (bytes)", std::to_string(text.size()));
    report.environment.emplace_back("Repetitions per measurement", std::to_string(settings_.repetitions));
    report.environment.emplace_back("Patterns per length", std::to_string(settings_.patternsPerLength));
    std::mt19937_64 rng(settings_.seed);

    hooks_.enter("Experiment 1: index size and construction time");
    std::optional<FMIndex> checkpoint;
    std::optional<FMIndex> wavelet;
    for (double fraction : settings_.sizeFractions) {
        const size_t length = std::clamp<size_t>(static_cast<size_t>(static_cast<double>(text.size()) * fraction), 1, text.size());
        const std::string_view prefix = text.substr(0, length);
        Built a = buildRepeated(prefix, {32, 32, RankKind::Checkpoint}, settings_.buildRepetitions, hooks_);
        Built b = buildRepeated(prefix, {32, 32, RankKind::WaveletTree}, settings_.buildRepetitions, hooks_);
        const double x = static_cast<double>(length);
        const double a_bytes = static_cast<double>(a.index->sizeInBytes());
        const double b_bytes = static_cast<double>(b.index->sizeInBytes());
        report.rows.push_back(makeRow("index_size", "Original text", x, x, "bytes"));
        report.rows.push_back(makeRow("index_size", "Suffix array (32-bit)", x, 4 * x, "bytes"));
        report.rows.push_back(makeRow("index_size", "FM-index, sampled Occ", x, a_bytes, "bytes"));
        report.rows.push_back(makeRow("index_size", "FM-index, wavelet tree", x, b_bytes, "bytes"));
        report.rows.push_back(makeRow("bits_per_character", "Suffix array (32-bit)", x, 32.0, "bits/char"));
        report.rows.push_back(makeRow("bits_per_character", "FM-index, sampled Occ", x, 8 * a_bytes / x, "bits/char"));
        report.rows.push_back(makeRow("bits_per_character", "FM-index, wavelet tree", x, 8 * b_bytes / x, "bits/char"));
        report.rows.push_back(makeRow("construction_time", "FM-index, sampled Occ", x, MetricsCollector::summarize(a.total), "s"));
        report.rows.push_back(makeRow("construction_time", "FM-index, wavelet tree", x, MetricsCollector::summarize(b.total), "s"));
        report.rows.push_back(makeRow("construction_time", "Suffix array only", x, MetricsCollector::summarize(a.suffixArray), "s"));
        if (length == text.size()) {
            checkpoint = std::move(a.index);
            wavelet = std::move(b.index);
        }
    }
    if (!checkpoint) {
        checkpoint = FMIndexBuilder({32, 32, RankKind::Checkpoint}).build(text, nullptr, hooks_).index;
        wavelet = FMIndexBuilder({32, 32, RankKind::WaveletTree}).build(text, nullptr, hooks_).index;
    }

    hooks_.enter("Building baseline structures");
    const NaiveSearcher naive(text);
    const SuffixArraySearcher suffixArray(text);
    report.rows.push_back(makeRow("memory_summary", "Original text", 0, static_cast<double>(text.size()), "bytes"));
    report.rows.push_back(makeRow("memory_summary", "Suffix array (32-bit)", 0, static_cast<double>(suffixArray.sizeInBytes()), "bytes"));
    report.rows.push_back(makeRow("memory_summary", "FM-index, sampled Occ", 0, static_cast<double>(checkpoint->sizeInBytes()), "bytes"));
    report.rows.push_back(makeRow("memory_summary", "FM-index, wavelet tree", 0, static_cast<double>(wavelet->sizeInBytes()), "bytes"));

    const std::vector<Method> methods{
        {"Naive scan", [&](std::string_view p) { return naive.count(p); },
         [&](std::string_view p, size_t limit) { return naive.locate(p, limit); }},
        {"Suffix array", [&](std::string_view p) { return suffixArray.count(p); },
         [&](std::string_view p, size_t limit) { return suffixArray.locate(p, limit); }},
        fmMethod("FM-index (sampled Occ)", *checkpoint),
        fmMethod("FM-index (wavelet tree)", *wavelet),
    };

    for (size_t length : settings_.patternLengths) {
        hooks_.enter("Experiments 2 and 3: patterns of length " + std::to_string(length));
        const PatternSet patterns = makePatterns(text, length, settings_.patternsPerLength, rng);
        if (patterns.existing.empty()) {
            report.notes.push_back("Skipped pattern length " + std::to_string(length) + ": longer than the text.");
            continue;
        }
        verify(methods, patterns, settings_.locateLimit);
        const double x = static_cast<double>(length);

        std::vector<double> occurrences;
        for (const std::string& pattern : patterns.existing) {
            occurrences.push_back(static_cast<double>(naive.count(pattern)));
        }
        report.rows.push_back(makeRow("match_count", "Existing patterns", x, MetricsCollector::summarize(occurrences), "matches"));

        for (const Method& method : methods) {
            hooks_.enter("Measuring " + method.name + ", length " + std::to_string(length));
            auto count = [&](const std::string& p) { return method.count(p); };
            auto locate = [&](const std::string& p) { return method.locate(p, settings_.locateLimit).size(); };
            report.rows.push_back(makeRow("count_latency", method.name + " (existing)", x,
                                          measure(patterns.existing, settings_.repetitions, count), "us"));
            if (!patterns.missing.empty()) {
                report.rows.push_back(makeRow("count_latency", method.name + " (missing)", x,
                                              measure(patterns.missing, settings_.repetitions, count), "us"));
            }
            report.rows.push_back(makeRow("locate_latency", method.name, x,
                                          measure(patterns.existing, settings_.repetitions, locate), "us"));
        }

        for (const auto& [name, index] : {std::pair<const char*, const FMIndex*>{"FM-index (sampled Occ)", &*checkpoint},
                                          std::pair<const char*, const FMIndex*>{"FM-index (wavelet tree)", &*wavelet}}) {
            std::vector<double> stepsPerMatch;
            for (const std::string& pattern : patterns.existing) {
                const SearchResult result = FMIndexSearcher(*index).search(pattern, settings_.locateLimit);
                if (!result.positions.empty()) {
                    stepsPerMatch.push_back(static_cast<double>(result.lfSteps) / static_cast<double>(result.positions.size()));
                }
            }
            report.rows.push_back(makeRow("locate_lf_steps", name, x, MetricsCollector::summarize(stepsPerMatch), "LF steps/match"));
        }
    }

    hooks_.enter("Experiment 4: sampling intervals");
    const size_t probeLength = std::min<size_t>(16, text.size());
    const PatternSet probe = makePatterns(text, probeLength, settings_.patternsPerLength, rng);
    for (uint32_t interval : settings_.samplingIntervals) {
        hooks_.enter("Sampling interval " + std::to_string(interval));
        const double x = interval;
        const FMIndex occVariant = FMIndexBuilder({interval, 32, RankKind::Checkpoint}).build(text, nullptr, hooks_).index;
        const FMIndex saVariant = FMIndexBuilder({32, interval, RankKind::Checkpoint}).build(text, nullptr, hooks_).index;
        report.rows.push_back(makeRow("sampling_memory", "Occ sampling", x, static_cast<double>(occVariant.sizeInBytes()), "bytes"));
        report.rows.push_back(makeRow("sampling_memory", "SA sampling", x, static_cast<double>(saVariant.sizeInBytes()), "bytes"));
        report.rows.push_back(makeRow("sampling_count_latency", "Occ sampling", x,
                                      measure(probe.existing, settings_.repetitions,
                                              [&](const std::string& p) { return FMIndexSearcher(occVariant).count(p); }),
                                      "us"));
        report.rows.push_back(makeRow("sampling_locate_latency", "SA sampling", x,
                                      measure(probe.existing, settings_.repetitions,
                                              [&](const std::string& p) {
                                                  return FMIndexSearcher(saVariant).locate(p, settings_.locateLimit).size();
                                              }),
                                      "us"));
    }
    report.notes.push_back("All measurements are serial (single thread) and use std::chrono::steady_clock.");
    report.notes.push_back("Locate latency is limited to the first " + std::to_string(settings_.locateLimit) +
                           " matches for every method.");
    report.environment.emplace_back("Peak resident memory after run (bytes)", std::to_string(MetricsCollector::peakResidentBytes()));
    return report;
}

void BenchmarkRunner::writeCsv(const BenchmarkReport& report, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        throw IndexError("The CSV file could not be created.");
    }
    for (const auto& [key, value] : report.environment) {
        out << "# " << key << ": " << value << "\n";
    }
    out << "experiment,series,x,median,min,max,unit\n";
    for (const BenchRow& row : report.rows) {
        out << row.experiment << ',' << quoted(row.series) << ',' << row.x << ',' << row.median << ',' << row.minimum << ','
            << row.maximum << ',' << row.unit << "\n";
    }
    if (!out) {
        throw IndexError("The CSV file could not be written.");
    }
}

std::string BenchmarkRunner::generateLog(size_t bytes, uint64_t seed) {
    static const char* const levels[] = {"INFO", "INFO", "INFO", "DEBUG", "WARN", "ERROR"};
    static const char* const messages[] = {"request completed", "connection refused", "timeout while waiting for upstream",
                                           "user session started", "cache miss for key", "disk usage above threshold",
                                           "retrying operation", "configuration reloaded"};
    std::mt19937_64 rng(seed);
    std::string log;
    log.reserve(bytes + 128);
    unsigned second = 0;
    while (log.size() < bytes) {
        second += static_cast<unsigned>(rng() % 3);
        char line[256];
        std::snprintf(line, sizeof(line), "2024-03-%02u %02u:%02u:%02u %s [worker-%u] 192.168.%u.%u %s id=%llu\n",
                      1 + second / 86400 % 28, second / 3600 % 24, second / 60 % 60, second % 60,
                      levels[rng() % 6], static_cast<unsigned>(rng() % 16), static_cast<unsigned>(rng() % 4),
                      static_cast<unsigned>(rng() % 255), messages[rng() % 8], static_cast<unsigned long long>(rng() % 100000));
        log += line;
    }
    return log;
}

}
