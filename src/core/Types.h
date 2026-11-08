#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>

namespace fmi {

class IndexError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class OperationCancelled : public std::runtime_error {
public:
    OperationCancelled() : std::runtime_error("The operation was cancelled.") {}
};

enum class RankKind : uint32_t { Checkpoint = 0, WaveletTree = 1 };

struct IndexConfig {
    uint32_t occSample = 32;
    uint32_t saSample = 32;
    RankKind rank = RankKind::Checkpoint;

    void validate() const;
};

struct BuildHooks {
    std::function<void(const std::string&)> onPhase;
    std::function<bool()> isCancelled;

    void enter(const std::string& phase) const;
};

constexpr size_t kMaxSymbols = 0xFFFFFFF0u;

const char* rankKindName(RankKind kind);

}
