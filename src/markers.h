#pragma once

#include "cons.h"
#include "JPEGDecoder.h"

void CheckStartMarker(MarkerType marker);

void CheckEndMarker(MarkerType marker);

void ProcessMarker(MarkerType marker, JPEGDecoder& decoder);

void ProcessSOF0(JPEGDecoder& decoder);

void ProcessDHT(JPEGDecoder& decoder);

void ProcessDQT(JPEGDecoder& decoder);

void ProcessAPPn(JPEGDecoder& decoder);

void ProcessCOM(JPEGDecoder& decoder);

void ProcessSOS(JPEGDecoder& decoder);

// void ProcessSOF2();  // for progressive
