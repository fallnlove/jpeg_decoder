#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

using MarkerType = uint16_t;

constexpr size_t kChannelNum = 3;

constexpr size_t kByteSize = 8;

constexpr size_t kStandartMCUSize = 8;

constexpr size_t kSosSize = 10;

constexpr size_t kMetaSize = 15;

constexpr size_t kHuffmanSize = 16;

constexpr size_t kTableSize = 64;

constexpr MarkerType kMarkerStart = 0xffd8;

constexpr MarkerType kMarkerEnd = 0xffd9;

constexpr MarkerType kMarkerSOF0 = 0xffc0;

constexpr MarkerType kMarkerDHT = 0xffc4;

constexpr MarkerType kMarkerDQT = 0xffdb;

constexpr MarkerType kMarkerAPP0 = 0xffe0;

constexpr MarkerType kMarkerAPP16 = 0xffef;

constexpr MarkerType kMarkerCOM = 0xfffe;

constexpr MarkerType kMarkerSOS = 0xffda;

constexpr MarkerType k00 = 0x00;
constexpr MarkerType k01 = 0x01;
constexpr MarkerType k10 = 0x10;
constexpr MarkerType k11 = 0x11;

constexpr std::array<int32_t, 64> kZigZag = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48,
    41, 34, 27, 20, 13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23,
    30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
