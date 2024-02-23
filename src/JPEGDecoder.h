#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "bitReader.h"
#include "cons.h"
#include "image.h"
#include "huffman.h"
#include "fft.h"

struct Channel {
    uint8_t horizontal = 1;
    uint8_t vertical = 1;
    std::vector<int32_t> DQT;
    MarkerType DQTid;
    HuffmanTree* DHTAC;
    HuffmanTree* DHTDC;
    int32_t last_value = 0;
    bool used_ = false;
};

class JPEGDecoder {
public:
    std::vector<int32_t> DQT00;
    std::vector<int32_t> DQT01;
    std::vector<int32_t> DQT10;
    std::vector<int32_t> DQT11;

    HuffmanTree DHTAC0;
    HuffmanTree DHTAC1;
    HuffmanTree DHTDC0;
    HuffmanTree DHTDC1;

    Channel Y;
    Channel Cb;
    Channel Cr;

public:
    JPEGDecoder() = delete;

    JPEGDecoder(std::istream& input);

    void StartImageCreation();

    bool IsDecoding();

    MarkerType GetMarker();

    size_t GetMarkerSize();

    Image& GetImage();

    uint8_t ReadByte();

    uint16_t ReadTwoBytes();

    void SetComment(const std::string& comment);

    void ReachEnd();

    void SetSize(size_t width, size_t height);

    std::vector<int32_t>& GetTableById(MarkerType id);

    Channel& GetChannelById(size_t id);

    void ProcessChannel(Channel& channel);

private:
    BitReader reader_;
    Image image_;
    bool finish_;

    void DecodeMCUBlock(size_t row, size_t column, size_t mcu_hieght, size_t mcu_width);

    std::vector<uint8_t> DecodeChannel(Channel& channel, size_t mcu_hieght, size_t mcu_width);

    std::vector<int32_t> DecodeTable(Channel& channel);

    RGB YCbCrToRGB(uint8_t y, uint8_t cb, uint8_t cr);

    uint8_t Get(std::vector<uint8_t>& vec, size_t i, size_t j, size_t width);

    uint8_t ReadCoef(HuffmanTree* huffman);

    int32_t ReadValue(size_t length);

    void ProductTable(std::vector<int32_t>& lhs, const std::vector<int32_t>& rhs);

    void IDCT(std::vector<int32_t>& table);

    void Norm(std::vector<int32_t>& table);
};
