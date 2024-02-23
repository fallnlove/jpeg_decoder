#pragma once

#include <cstdint>
#include <istream>

class BitReader {
public:
    BitReader() = delete;

    BitReader(std::istream& istream);

    uint8_t ReadByte();

    uint16_t ReadTwoBytes();

    bool ReadBit();

    bool IsEnd();

private:
    std::istream& istream_;
    uint8_t bit_;
    uint8_t used_bits_;

    uint8_t Read();
};
