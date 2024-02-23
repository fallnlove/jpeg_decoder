#include "JPEGDecoder.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "cons.h"
#include <glog/logging.h>
#include <math.h>
#include "fft.h"

JPEGDecoder::JPEGDecoder(std::istream& input) : reader_(input), finish_(false) {
}

Channel& JPEGDecoder::GetChannelById(size_t id) {
    if (id == 0) {
        return Y;
    } else if (id == 1) {
        return Cb;
    }
    return Cr;
}

void JPEGDecoder::Norm(std::vector<int32_t>& table) {
    for (int32_t& num : table) {
        num = std::min(255, std::max(0, num + 128));
    }
}

void JPEGDecoder::IDCT(std::vector<int32_t>& table) {
    static std::vector<double> input(kTableSize);
    static std::vector<double> output(kTableSize);
    static DctCalculator fft(kStandartMCUSize, &input, &output);
    for (size_t i = 0; i < kTableSize; ++i) {
        input[i] = static_cast<double>(table[i]);
    }
    fft.Inverse();
    for (size_t i = 0; i < kTableSize; ++i) {
        table[i] = round(output[i]);
    }
}

void JPEGDecoder::ProductTable(std::vector<int32_t>& lhs, const std::vector<int32_t>& rhs) {
    for (size_t i = 0; i < std::min(lhs.size(), rhs.size()); ++i) {
        lhs[i] *= rhs[i];  // TODO: maybe integer overflow
    }
}

uint8_t JPEGDecoder::ReadCoef(HuffmanTree* huffman) {
    int32_t value = 0;
    while (!huffman->Move(reader_.ReadBit(), value)) {
    }
    return static_cast<uint8_t>(value);
}

int32_t JPEGDecoder::ReadValue(size_t length) {
    if (length == 0) {  // TODO: maybe 0 is illegal
        return 0;
    }

    if (length > 30) {
        DLOG(ERROR) << "Very big int\n";
        throw std::runtime_error("Very big int\n");
    }

    bool bit = reader_.ReadBit();
    int32_t value = bit;

    for (size_t i = 0; i < length - 1; ++i) {
        value <<= 1;
        value += reader_.ReadBit();
    }

    if (bit == 0) {
        return value - (1 << length) + 1;
    }
    return value;
}

std::vector<int32_t> JPEGDecoder::DecodeTable(Channel& channel) {
    std::vector<int32_t> table(kTableSize);

    table[0] = ReadValue(ReadCoef(channel.DHTDC));

    for (auto iterator = kZigZag.begin() + 1; iterator != kZigZag.end();) {
        uint8_t coef = ReadCoef(channel.DHTAC);

        if (coef == 0) {
            while (iterator != kZigZag.end()) {
                table[*iterator] = 0;
                ++iterator;
            }
            break;
        }

        for (size_t i = 0; i < (coef >> (kByteSize / 2)) && iterator != kZigZag.end(); ++i) {
            table[*iterator] = 0;
            ++iterator;
        }

        if (iterator == kZigZag.end()) {
            DLOG(ERROR) << "Wrong coef\n";
            throw std::runtime_error("Wrong coef\n");
        }
        table[*iterator] = ReadValue(coef % (1 << (kByteSize / 2)));
        ++iterator;
    }

    table[0] += channel.last_value;
    channel.last_value = table[0];

    ProductTable(table, channel.DQT);

    IDCT(table);

    Norm(table);

    return table;
}

std::vector<uint8_t> JPEGDecoder::DecodeChannel(Channel& channel, size_t mcu_hieght,
                                                size_t mcu_width) {
    std::vector<uint8_t> res(mcu_hieght * mcu_width / (channel.vertical * channel.horizontal), 128);
    if (!channel.used_) {
        return res;
    }

    for (size_t i = 0; i < mcu_hieght / channel.vertical; i += kStandartMCUSize) {
        for (size_t j = 0; j < mcu_width / channel.horizontal; j += kStandartMCUSize) {
            auto table = DecodeTable(channel);
            for (size_t row = 0; row < kStandartMCUSize; ++row) {
                for (size_t column = 0; column < kStandartMCUSize; ++column) {
                    res[(row + i) * (mcu_width / channel.horizontal) + column + j] =
                        table[row * kStandartMCUSize + column];
                }
            }
        }
    }

    return res;
}

RGB JPEGDecoder::YCbCrToRGB(uint8_t y, uint8_t cb, uint8_t cr) {
    int r = round(static_cast<double>(y) + 1.402 * (static_cast<double>(cr) - 128.));
    int g = round(static_cast<double>(y) - 0.34414 * (static_cast<double>(cb) - 128.) -
                  0.71414 * (static_cast<double>(cr) - 128.));
    int b = round(static_cast<double>(y) + 1.772 * (static_cast<double>(cb) - 128.));

    return {.r = std::min(255, std::max(0, r)),
            .g = std::min(255, std::max(0, g)),
            .b = std::min(255, std::max(0, b))};
}

uint8_t JPEGDecoder::Get(std::vector<uint8_t>& vec, size_t i, size_t j, size_t width) {
    return vec[i * width + j];
}

void JPEGDecoder::DecodeMCUBlock(size_t row, size_t column, size_t mcu_hieght, size_t mcu_width) {
    std::vector<uint8_t> y_vec = DecodeChannel(Y, mcu_hieght, mcu_width);
    std::vector<uint8_t> cb_vec = DecodeChannel(Cb, mcu_hieght, mcu_width);
    std::vector<uint8_t> cr_vec = DecodeChannel(Cr, mcu_hieght, mcu_width);

    for (size_t i = row; i < std::min(row + mcu_hieght, image_.Height()); ++i) {
        for (size_t j = column; j < std::min(column + mcu_width, image_.Width()); ++j) {
            image_.SetPixel(
                i, j,
                YCbCrToRGB(Get(y_vec, (i - row) / Y.vertical, (j - column) / Y.horizontal,
                               mcu_width / Y.horizontal),
                           Get(cb_vec, (i - row) / Cb.vertical, (j - column) / Cb.horizontal,
                               mcu_width / Cb.horizontal),
                           Get(cr_vec, (i - row) / Cr.vertical, (j - column) / Cr.horizontal,
                               mcu_width / Cr.horizontal)));
        }
    }
}

void JPEGDecoder::StartImageCreation() {
    size_t mcu_hieght = kStandartMCUSize * std::max({Y.vertical, Cb.vertical, Cr.vertical});
    size_t mcu_width = kStandartMCUSize * std::max({Y.horizontal, Cb.horizontal, Cr.horizontal});

    for (size_t row = 0; row < (image_.Height() - 1) / mcu_hieght + 1; ++row) {
        for (size_t column = 0; column < (image_.Width() - 1) / mcu_width + 1; ++column) {
            DecodeMCUBlock(row * mcu_hieght, column * mcu_width, mcu_hieght, mcu_width);
        }
    }
}

bool JPEGDecoder::IsDecoding() {
    return !finish_;
}

MarkerType JPEGDecoder::GetMarker() {
    return reader_.ReadTwoBytes();
}

size_t JPEGDecoder::GetMarkerSize() {
    size_t size = static_cast<size_t>(reader_.ReadTwoBytes());
    if (size < 2) {
        DLOG(ERROR) << "Wrong marker size\n";
        throw std::runtime_error("Wrong marker size\n");
    }
    DLOG(INFO) << "Marker size " << size - 2 << "\n";
    return size - 2;
}

Image& JPEGDecoder::GetImage() {
    return image_;
}

uint8_t JPEGDecoder::ReadByte() {
    return reader_.ReadByte();
}

uint16_t JPEGDecoder::ReadTwoBytes() {
    return reader_.ReadTwoBytes();
}

void JPEGDecoder::SetComment(const std::string& comment) {
    image_.SetComment(comment);
}

void JPEGDecoder::ReachEnd() {
    finish_ = true;
}

void JPEGDecoder::SetSize(size_t width, size_t height) {
    if (image_.Height() != 0) {
        DLOG(ERROR) << "Set size twice\n";
        throw std::runtime_error("Set size twice\n");
    }
    image_.SetSize(width, height);
}

std::vector<int32_t>& JPEGDecoder::GetTableById(MarkerType id) {
    if (id == k00) {
        return DQT00;
    } else if (id == k01) {
        return DQT01;
    } else if (id == k10) {
        return DQT10;
    } else if (id == k11) {
        return DQT11;
    }
    DLOG(ERROR) << "Wrong DQT  id\n";
    throw std::runtime_error("Wrong DQT  id\n");
}

void JPEGDecoder::ProcessChannel(Channel& channel) {
    if (!channel.used_) {
        return;
    }

    channel.DQT = GetTableById(channel.DQTid);
    if (channel.DQT.empty()) {
        DLOG(ERROR) << "Empty DQT table\n";
        throw std::runtime_error("Empty DQT table\n");
    }
}
