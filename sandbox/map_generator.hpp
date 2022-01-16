#pragma once

#include <vector>
#include "fluent/fluent.hpp"

struct Map
{
    std::vector<f32> data;
};

struct NoiseSettings
{
    f32             scale       = 100.0f;
    i32             octaves     = 4;
    f32             persistance = 0.5f;
    f32             lacunarity  = 2.0f;
    i32             seed        = 0.0f;
    fluent::Vector2 offset      = fluent::Vector2(0.0f, 0.0f);
};

struct TerrainType
{
    f32             height;
    fluent::Vector3 color;
};

using TerrainTypes = std::vector<TerrainType>;

enum class MapType : u32
{
    eNoise   = 0,
    eTerrain = 1,
    eLast
};

class MapGenerator
{
private:
    static constexpr u32 MAP_SIZE = 241;
    static constexpr u32 BPP      = 4;
    NoiseSettings        m_noise_settings;
    Map                  m_noise_map;
    Map                  m_terrain_map;
    TerrainTypes         m_terrain_types;

public:
    void init();
    void update_noise_map(const NoiseSettings& settings);
    void update_terrain_map(const TerrainTypes& types);

    const Map& get_noise_map() const
    {
        return m_noise_map;
    }

    const Map& get_terrain_map() const
    {
        return m_terrain_map;
    }

    static constexpr u32 get_map_size()
    {
        return MAP_SIZE;
    }

    static constexpr u32 get_bpp()
    {
        return BPP;
    }
};