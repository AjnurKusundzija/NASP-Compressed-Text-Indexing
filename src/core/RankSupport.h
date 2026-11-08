#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "core/BWTBuilder.h"
#include "core/Types.h"

namespace fmi {

class RankSupport {
public:
    virtual ~RankSupport() = default;

    virtual size_t rank(uint16_t symbol, size_t i) const = 0;
    virtual uint16_t at(size_t i) const = 0;
    virtual size_t size() const = 0;
    virtual size_t sizeInBytes() const = 0;
    virtual RankKind kind() const = 0;
};

class CheckpointRank : public RankSupport {
public:
    CheckpointRank(BWT bwt, size_t alphabetSize, uint32_t interval);

    size_t rank(uint16_t symbol, size_t i) const override;
    uint16_t at(size_t i) const override { return bwt_.symbol(i); }
    size_t size() const override { return bwt_.size(); }
    size_t sizeInBytes() const override;
    RankKind kind() const override { return RankKind::Checkpoint; }

    uint32_t interval() const { return interval_; }

private:
    BWT bwt_;
    size_t letters_;
    uint32_t interval_;
    std::vector<uint32_t> checkpoints_;
};

class FullOccTable : public RankSupport {
public:
    FullOccTable(const BWT& bwt, size_t alphabetSize);

    size_t rank(uint16_t symbol, size_t i) const override { return table_[symbol * (size_ + 1) + i]; }
    uint16_t at(size_t i) const override { return symbols_[i]; }
    size_t size() const override { return size_; }
    size_t sizeInBytes() const override;
    RankKind kind() const override { return RankKind::Checkpoint; }

private:
    size_t size_;
    std::vector<uint16_t> symbols_;
    std::vector<uint32_t> table_;
};

std::unique_ptr<RankSupport> makeRankSupport(RankKind kind, BWT bwt, size_t alphabetSize, uint32_t occSample);

}
