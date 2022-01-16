#pragma once

#include <vector>
#include "fluent/fluent.hpp"

struct Map
{
    u32              width;
    u32              height;
    u32              bpp;
    std::vector<f32> data;
};
