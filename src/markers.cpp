#include "markers.h"
#include <glog/logging.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include "JPEGDecoder.h"
#include "cons.h"
#include "huffman.h"

void CheckStartMarker(MarkerType marker) {
    DLOG(INFO) << "Start decoding\n";
    if (marker != kMarkerStart) {
        DLOG(ERROR) << "Incorrect start marker\n";
        throw std::runtime_error("Incorrect file\n");
    }
}

void CheckEndMarker(MarkerType marker) {
    DLOG(INFO) << "End decoding\n";
    if (marker != kMarkerEnd) {
        DLOG(ERROR) << "Incorrect end marker\n";
        throw std::runtime_error("Incorrect file\n");
    }
}

void ProcessMarker(MarkerType marker, JPEGDecoder& decoder) {
    if (marker == kMarkerEnd) {
        DLOG(INFO) << "Reach end\n";
        decoder.ReachEnd();
    } else if (marker == kMarkerSOF0) {
        DLOG(INFO) << "Read meta inforamtion\n";
        ProcessSOF0(decoder);
    } else if (marker == kMarkerDHT) {
        DLOG(INFO) << "Read DHT\n";
        ProcessDHT(decoder);
    } else if (marker == kMarkerDQT) {
        DLOG(INFO) << "Read DQT\n";
        ProcessDQT(decoder);
    } else if (marker >= kMarkerAPP0 && marker <= kMarkerAPP16) {
        DLOG(INFO) << "Read APP\n";
        ProcessAPPn(decoder);
    } else if (marker == kMarkerCOM) {
        DLOG(INFO) << "Read comment\n";
        ProcessCOM(decoder);
    } else if (marker == kMarkerSOS) {
        DLOG(INFO) << "Start main part\n";
        ProcessSOS(decoder);
    } else {
        DLOG(ERROR) << "Unknown marker\n";
        throw std::runtime_error("Unknown marker\n");
    }
}

void SetChannelScale(Channel& channel, uint8_t hor_max, uint8_t ver_max) {
    if (!channel.used_) {
        return;
    }

    if (channel.horizontal == 0 || channel.vertical == 0 || hor_max == 0 || ver_max == 0) {
        DLOG(ERROR) << "Zero precision\n";
        throw std::runtime_error("Zero precision\n");
    }

    if (hor_max % channel.horizontal != 0 || hor_max / channel.horizontal > 2) {
        DLOG(ERROR) << "Incorrect horizontal scale\n";
        throw std::runtime_error("Incorrect horizontal scale\n");
    }
    channel.horizontal = hor_max / channel.horizontal;

    if (ver_max % channel.vertical != 0 || ver_max / channel.vertical > 2) {
        DLOG(ERROR) << "Incorrect vertical scale\n";
        throw std::runtime_error("Incorrect vertical scale\n");
    }
    channel.vertical = ver_max / channel.vertical;
}

void ProcessSOF0(JPEGDecoder& decoder) {
    size_t size = decoder.GetMarkerSize();

    if (decoder.ReadByte() != kByteSize) {
        DLOG(ERROR) << "Wrong precision\n";
        throw std::runtime_error("Wrong precision\n");
    }

    size_t height = static_cast<size_t>(decoder.ReadTwoBytes());
    size_t width = static_cast<size_t>(decoder.ReadTwoBytes());
    DLOG(INFO) << "Height " << height << "\n";
    DLOG(INFO) << "Width " << width << "\n";

    if (height == 0 || width == 0) {
        DLOG(ERROR) << "Empty JPEG\n";
        throw std::runtime_error("Empty JPEG\n");
    }

    decoder.SetSize(width, height);

    size_t channel_num = decoder.ReadByte();

    if (size != 6 + channel_num * 3) {
        DLOG(ERROR) << "Wrong meta size\n";
        throw std::runtime_error("Wrong meta size\n");
    }

    if (channel_num > 3 || channel_num == 0) {
        DLOG(ERROR) << "Wrong channels num\n";
        throw std::runtime_error("Wrong channels num\n");
    }

    uint8_t horizontal_max = std::numeric_limits<uint8_t>::min();
    uint8_t vertical_max = std::numeric_limits<uint8_t>::min();

    for (size_t i = 0; i < channel_num; ++i) {
        Channel channel;

        uint8_t id = decoder.ReadByte();
        uint8_t pr = decoder.ReadByte();

        channel.horizontal = (pr >> (kByteSize / 2));
        channel.vertical = (pr % (1 << (kByteSize / 2)));

        horizontal_max = std::max(horizontal_max, channel.horizontal);
        vertical_max = std::max(vertical_max, channel.vertical);

        channel.DQTid = decoder.ReadByte();

        if (id == 1) {
            decoder.Y = std::move(channel);
            decoder.Y.used_ = true;
        } else if (id == 2) {
            decoder.Cb = std::move(channel);
            decoder.Cb.used_ = true;
        } else if (id == 3) {
            decoder.Cr = std::move(channel);
            decoder.Cr.used_ = true;
        } else {
            DLOG(ERROR) << "Incorrect quant id: " << id << " channel num: " << i << "\n";
            throw std::runtime_error("Incorrect quant id\n");
        }
    }

    SetChannelScale(decoder.Y, horizontal_max, vertical_max);
    SetChannelScale(decoder.Cb, horizontal_max, vertical_max);
    SetChannelScale(decoder.Cr, horizontal_max, vertical_max);
}

void ProcessDHT(JPEGDecoder& decoder) {
    size_t size = decoder.GetMarkerSize();

    while (size != 0) {
        if (size < 1 + kHuffmanSize) {
            DLOG(ERROR) << "Incorrect DHT size\n";
            throw std::runtime_error("Incorrect DHT size\n");
        }
        size -= 1 + kHuffmanSize;

        HuffmanTree huffman;
        MarkerType id = decoder.ReadByte();
        std::vector<uint8_t> code_lengths;
        std::vector<uint8_t> values;

        size_t value_size = 0;

        code_lengths.reserve(kHuffmanSize);

        DLOG(INFO) << "Build Huffman " << id << "\n";

        for (size_t i = 0; i < kHuffmanSize; ++i) {
            code_lengths.push_back(decoder.ReadByte());
            value_size += code_lengths.back();
        }

        values.reserve(value_size);

        if (size < value_size) {
            DLOG(ERROR) << "Incorrect DHT size\n";
            throw std::runtime_error("Incorrect DHT size\n");
        }

        size -= value_size;

        for (size_t i = 0; i < value_size; ++i) {
            values.push_back(decoder.ReadByte());
        }
        DLOG(INFO) << "Num of values " << values.size() << "\n";

        huffman.Build(code_lengths, values);

        if (id == k00) {
            decoder.DHTDC0 = std::move(huffman);
        } else if (id == k01) {
            decoder.DHTDC1 = std::move(huffman);
        } else if (id == k10) {
            decoder.DHTAC0 = std::move(huffman);
        } else if (id == k11) {
            decoder.DHTAC1 = std::move(huffman);
        } else {
            DLOG(ERROR) << "Unknown DHT id\n";
            throw std::runtime_error("Unknown DHT id\n");
        }
    }
}

void ProcessDQT(JPEGDecoder& decoder) {
    size_t size = decoder.GetMarkerSize();
    std::vector<int32_t> dqt(kTableSize);
    std::function<int32_t()> reader;

    while (size != 0) {
        MarkerType id = decoder.ReadByte();

        if (id >> (kByteSize / 2) == 0) {
            reader = [&]() { return static_cast<int32_t>(decoder.ReadByte()); };

            if (size < kTableSize + 1) {
                DLOG(ERROR) << "Incorrect DQT size\n";
                throw std::runtime_error("Incorrect DQT size\n");
            }
            size -= kTableSize + 1;
        } else if (id >> (kByteSize / 2) == 1) {
            reader = [&]() { return static_cast<int32_t>(decoder.ReadTwoBytes()); };

            if (size < 2 * kTableSize + 1) {
                DLOG(ERROR) << "Incorrect DQT size\n";
                throw std::runtime_error("Incorrect DQT size\n");
            }
            size -= 2 * kTableSize + 1;
        } else {
            DLOG(ERROR) << "Unknown DQT length\n";
            throw std::runtime_error("Unknown DQT length\n");
        }

        for (auto index : kZigZag) {
            dqt[index] = reader();
        }

        decoder.GetTableById(id) = dqt;
    }
}

void ProcessAPPn(JPEGDecoder& decoder) {
    size_t size = decoder.GetMarkerSize();
    for (size_t i = 0; i < size; ++i) {
        decoder.ReadByte();
    }
}

void ProcessCOM(JPEGDecoder& decoder) {
    size_t size = decoder.GetMarkerSize();
    std::string comment;

    comment.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        comment += static_cast<char>(decoder.ReadByte());
    }

    decoder.SetComment(comment);
}

void ProcessSOS(JPEGDecoder& decoder) {
    if (decoder.GetImage().Height() == 0 || decoder.GetImage().Width() == 0) {
        DLOG(ERROR) << "Empty Image\n";
        throw std::runtime_error("Empty Image\n");
    }

    decoder.ProcessChannel(decoder.Y);
    decoder.ProcessChannel(decoder.Cb);
    decoder.ProcessChannel(decoder.Cr);

    size_t size = decoder.GetMarkerSize();
    size_t channel_num = decoder.ReadByte();

    if (size != 1 + channel_num * 2 + 3 ||
        channel_num != static_cast<size_t>(decoder.Y.used_) +
                           static_cast<size_t>(decoder.Cb.used_) +
                           static_cast<size_t>(decoder.Cr.used_)) {
        DLOG(ERROR) << "Error in SOS\n";
        throw std::runtime_error("Error in SOS\n");
    }

    for (size_t i = 0; i < 3; ++i) {
        if (!decoder.GetChannelById(i).used_) {
            continue;
        }

        size_t id = decoder.ReadByte();

        if (id != i + 1) {
            DLOG(ERROR) << "Wrong channel id\n";
            throw std::runtime_error("Wrong channel id\n");
        }

        uint8_t table_id = decoder.ReadByte();

        if ((table_id >> (kByteSize / 2)) == 0) {
            decoder.GetChannelById(i).DHTDC = &decoder.DHTDC0;
        } else if ((table_id >> (kByteSize / 2)) == 1) {
            decoder.GetChannelById(i).DHTDC = &decoder.DHTDC1;
        } else {
            DLOG(ERROR) << "Wrong DHT num\n";
            throw std::runtime_error("Wrong DHT num\n");
        }

        if ((table_id % (1 << (kByteSize / 2))) == 0) {
            decoder.GetChannelById(i).DHTAC = &decoder.DHTAC0;
        } else if ((table_id % (1 << (kByteSize / 2))) == 1) {
            decoder.GetChannelById(i).DHTAC = &decoder.DHTAC1;
        } else {
            DLOG(ERROR) << "Wrong DHT num\n";
            throw std::runtime_error("Wrong DHT num\n");
        }
    }

    if (decoder.ReadByte() != 0x00 || decoder.ReadByte() != 0x3f || decoder.ReadByte() != 0x00) {
        DLOG(ERROR) << "Wrong end of SOS meta information\n";
        throw std::runtime_error("Wrong end of SOS meta information\n");
    }

    decoder.StartImageCreation();
}
