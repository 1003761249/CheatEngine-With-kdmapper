#pragma once
#include <cstdint>

struct ScanResult
{
    uint64_t address;
    uint64_t value;

    uint64_t lastValue = 0;
    uint64_t firstValue = 0;
    bool changed = false;
};