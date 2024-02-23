cmake_minimum_required(VERSION 3.14)
project(jpeg_decoder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(jpeg_decoder

        src/huffman.cpp
        src/fft.cpp
        src/bitReader.cpp
        src/markers.cpp
        src/JPEGDecoder.cpp
        src/decoder.cpp)
