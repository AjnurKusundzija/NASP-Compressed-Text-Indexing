#include "core/FMIndexBuilder.h"

#include "core/BWTBuilder.h"
#include "core/Stopwatch.h"
#include "core/SuffixArrayBuilder.h"

namespace fmi {

FMIndexBuilder::FMIndexBuilder(IndexConfig config) : config_(config) {
    config_.validate();
}

std::vector<size_t> FMIndexBuilder::buildC(const std::vector<uint16_t>& symbols, size_t alphabetSize) {
    std::vector<size_t> c(alphabetSize + 1, 0);
    for (uint16_t symbol : symbols) {
        ++c[symbol + 1];
    }
    for (size_t i = 1; i <= alphabetSize; ++i) {
        c[i] += c[i - 1];
    }
    return c;
}

BuildResult FMIndexBuilder::build(std::string_view text, std::vector<uint32_t>* keepSuffixArray,
                                  const BuildHooks& hooks) const {
    if (text.size() + 1 > kMaxSymbols) {
        throw IndexError("The text is too large for this index (limit is about 4 billion symbols).");
    }
    Stopwatch total;
    Stopwatch phase;
    BuildStats stats;
    stats.estimatedPeakBytes = text.size() + SuffixArrayBuilder::estimatePeakBytes(text.size() + 1);

    const Alphabet alphabet = Alphabet::fromText(text);
    const size_t sigma = alphabet.size();
    std::vector<uint16_t> symbols = alphabet.encodeText(text);

    hooks.enter("Building suffix array");
    phase.restart();
    std::vector<uint32_t> sa = SuffixArrayBuilder::build(symbols, sigma);
    stats.saSeconds = phase.seconds();

    hooks.enter("Computing Burrows-Wheeler transform");
    phase.restart();
    BWT bwt = BWTBuilder::build(symbols, sa);
    std::vector<size_t> c = buildC(symbols, sigma);
    stats.bwtSeconds = phase.seconds();
    std::vector<uint16_t>().swap(symbols);

    hooks.enter("Building rank structure");
    phase.restart();
    std::unique_ptr<RankSupport> rank = makeRankSupport(config_.rank, std::move(bwt), sigma, config_.occSample);
    stats.rankSeconds = phase.seconds();

    hooks.enter("Sampling suffix array");
    phase.restart();
    SampledSuffixArray samples(sa, config_.saSample);
    stats.sampleSeconds = phase.seconds();

    if (keepSuffixArray) {
        *keepSuffixArray = std::move(sa);
    }
    stats.totalSeconds = total.seconds();
    return {FMIndex(alphabet, std::move(c), std::move(rank), std::move(samples), config_), stats};
}

}
