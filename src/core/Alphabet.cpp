#include "core/Alphabet.h"

#include "core/Types.h"

namespace fmi {

Alphabet::Alphabet(std::vector<uint8_t> bytes) : bytes_(std::move(bytes)) {
    if (bytes_.size() > 256) {
        throw IndexError("The alphabet is too large.");
    }
    for (size_t i = 0; i < bytes_.size(); ++i) {
        if (i > 0 && bytes_[i] <= bytes_[i - 1]) {
            throw IndexError("The alphabet is not strictly increasing.");
        }
        codes_[bytes_[i]] = static_cast<uint16_t>(i + 1);
    }
}

Alphabet Alphabet::fromText(std::string_view text) {
    std::array<bool, 256> seen{};
    for (unsigned char ch : text) {
        seen[ch] = true;
    }
    std::vector<uint8_t> bytes;
    for (size_t b = 0; b < seen.size(); ++b) {
        if (seen[b]) {
            bytes.push_back(static_cast<uint8_t>(b));
        }
    }
    return Alphabet(std::move(bytes));
}

std::vector<uint16_t> Alphabet::encodeText(std::string_view text) const {
    std::vector<uint16_t> codes;
    codes.reserve(text.size() + 1);
    for (unsigned char ch : text) {
        codes.push_back(codes_[ch]);
    }
    codes.push_back(0);
    return codes;
}

}
