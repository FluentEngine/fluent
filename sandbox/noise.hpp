#pragma once

#include <vector>
#include "fluent/fluent.hpp"

using Map      = std::vector<u8>;
using NoiseMap = Map;

struct NoiseSettings
{
    u32             width       = 100;
    u32             height      = 100;
    f32             scale       = 27.6f;
    i32             octaves     = 4;
    f32             persistance = 0.5f;
    f32             lacunarity  = 2.0f;
    i32             seed        = 0.0f;
    fluent::Vector2 offset      = fluent::Vector2(0.0f, 0.0f);
};

NoiseMap generate_noise_map(const NoiseSettings& settings);
