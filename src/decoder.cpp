#include <decoder.h>
#include <glog/logging.h>
#include <cstdint>

#include "cons.h"
#include "markers.h"
#include "JPEGDecoder.h"

Image Decode(std::istream& input) {
    JPEGDecoder decoder(input);
    MarkerType marker = decoder.GetMarker();

    CheckStartMarker(marker);

    while (decoder.IsDecoding()) {
        marker = decoder.GetMarker();
        ProcessMarker(marker, decoder);
    }

    CheckEndMarker(marker);

    return decoder.GetImage();
}
