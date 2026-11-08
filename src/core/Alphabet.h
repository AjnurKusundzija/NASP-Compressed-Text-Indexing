#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace fmi {

class Alphabet {
public:
    Alphabet() = default;
    explicit Alphabet(std::vector<uint8_t> bytes);

    static Alphabet fromText(std::string_view text);

    size_t size() const { return bytes_.size() + 1; }
    uint16_t encode(uint8_t byte) const { return codes_[byte]; }
    uint8_t decode(uint16_t code) const { return bytes_[code - 1]; }
    const std::vector<uint8_t>& bytes() const { return bytes_; }

    std::vector<uint16_t> encodeText(std::string_view text) const;

private:
    std::vector<uint8_t> bytes_;
    std::array<uint16_t, 256> codes_{};
};

}
