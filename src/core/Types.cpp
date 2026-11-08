#include "core/Types.h"

namespace fmi {

void IndexConfig::validate() const {
    if (occSample == 0 || saSample == 0) {
        throw IndexError("Sampling intervals must be positive.");
    }
    if (rank != RankKind::Checkpoint && rank != RankKind::WaveletTree) {
        throw IndexError("Unknown rank representation.");
    }
}

void BuildHooks::enter(const std::string& phase) const {
    if (isCancelled && isCancelled()) {
        throw OperationCancelled();
    }
    if (onPhase) {
        onPhase(phase);
    }
}

const char* rankKindName(RankKind kind) {
    return kind == RankKind::Checkpoint ? "Sampled Occ table" : "Wavelet tree";
}

}
