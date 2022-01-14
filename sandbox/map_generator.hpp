#pragma once

#include <vector>
#include "fluent/fluent.hpp"
#include "noise.hpp"

using ColorMap = NoiseMap;

struct TerrainType
{
    f32             height;
    fluent::Vector3 color;
};

using TerrainTypes = std::vector<TerrainType>;

class MapGenerator
{
private:
    u32           m_width  = 0;
    u32           m_height = 0;
    NoiseSettings m_noise_settings;
    NoiseMap      m_noise_map;
    ColorMap      m_terrain_map;
    TerrainTypes  m_terrain_types;

public:
    void set_map_size(u32 width, u32 height);
    void update_noise_map(const NoiseSettings& settings);
    void update_terrain_map(const TerrainTypes& types);

    const NoiseMap& get_noise_map() const
    {
        return m_noise_map;
    }

    const ColorMap& get_terrain_map() const
    {
        return m_terrain_map;
    }
};