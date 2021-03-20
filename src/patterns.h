#pragma once

#include <cstdint>

#include <functional>

struct PatternDefinition {
    const char* name;
    bool alwaysWritesAlpha;
    const std::function<void(uint8_t* data, int width, int height, bool alpha)> render;
};

extern const int NumPatterns;
extern const PatternDefinition Patterns[];
