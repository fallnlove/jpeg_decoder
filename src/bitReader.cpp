#include "bitReader.h"
#include <glog/logging.h>
#include <cstdint>
#include <stdexcept>
#include "cons.h"

BitReader::BitReader(std::istream& istream) : istream_(istream), bit_(0), used_bits_(kByteSize) {
}

uint8_t BitReader::ReadByte() {
    // if (used_bits_ != kByteSize) {  TODO: maybe need
    //     DLOG(ERROR) << "Try to read byte before read all buffer\n";
    //     throw std::runtime_error("");
    // }
    return Read();
}

uint16_t BitReader::ReadTwoBytes() {
    return (static_cast<uint16_t>(ReadByte()) << kByteSize) + ReadByte();
}

bool BitReader::ReadBit() {
    if (used_bits_ == kByteSize) {
        bit_ = Read();
        if (bit_ == 0xff) {
            if (Read() != 0) {
                throw std::runtime_error("Wrong byte after 0xff\n");
            }
        }
        used_bits_ = 0;
    }
    uint8_t cur = bit_ >> (kByteSize - 1);
    bit_ <<= 1;
    // uint8_t cur = bit_;
    // bit_ >>= 1;
    ++used_bits_;
    return cur & 1;
}

bool BitReader::IsEnd() {
    return istream_.eof();
}

uint8_t BitReader::Read() {
    if (IsEnd()) {
        DLOG(ERROR) << "Read after reach end of file\n";
        throw std::runtime_error("");
    }
    char cur = 0;
    istream_.read(&cur, 1);
    return static_cast<uint8_t>(cur);
}
