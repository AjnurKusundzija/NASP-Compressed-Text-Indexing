#pragma once

#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "core/FMIndexBuilder.h"
#include "core/FMIndexSearcher.h"

namespace testing_support {

inline fmi::FMIndex makeIndex(const std::string& text, fmi::IndexConfig config = {}) {
    return fmi::FMIndexBuilder(config).build(text).index;
}

inline std::vector<size_t> naiveLocate(const std::string& text, const std::string& pattern) {
    std::vector<size_t> positions;
    if (pattern.empty()) {
        return positions;
    }
    for (size_t pos = text.find(pattern); pos != std::string::npos; pos = text.find(pattern, pos + 1)) {
        positions.push_back(pos);
    }
    return positions;
}

inline std::string randomText(std::mt19937& rng, size_t length, int alphabet) {
    std::string text(length, 'a');
    for (char& ch : text) {
        ch = static_cast<char>('a' + static_cast<int>(rng() % static_cast<unsigned>(alphabet)));
    }
    return text;
}

inline std::vector<fmi::IndexConfig> allConfigs() {
    std::vector<fmi::IndexConfig> configs;
    for (fmi::RankKind kind : {fmi::RankKind::Checkpoint, fmi::RankKind::WaveletTree}) {
        for (uint32_t occ : {1u, 3u, 32u}) {
            for (uint32_t sa : {1u, 4u, 32u}) {
                configs.push_back({occ, sa, kind});
            }
        }
    }
    return configs;
}

}
