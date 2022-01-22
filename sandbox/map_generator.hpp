#pragma once

#include <vector>
#include "fluent/fluent.hpp"

struct MeshData;

struct Map
{
    std::vector<f32> data;
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
    static constexpr u32 MAP_SIZE = 31;

    static struct NoiseSettings
    {
        f32 scale       = 200.0f;
        i32 octaves     = 4;
        f32 persistance = 0.5f;
        f32 lacunarity  = 2.0f;
        i32 seed        = 0;
    } noise_settings;

public:
    static void init();
    static void shutdown();
    static Map  generate_noise_map(fluent::Vector2 const& offset);

    static constexpr u32 get_map_size()
    {
        return MAP_SIZE;
    }
};