#include <algorithm>
#include <random>
#include "map_generator.hpp"

void MapGenerator::init()
{
    m_noise_map.data.resize(MAP_SIZE * MAP_SIZE * BPP);
    m_terrain_map.data.resize(MAP_SIZE * MAP_SIZE * BPP);
}

void MapGenerator::update_noise_map(const NoiseSettings& settings)
{
    using namespace fluent;

    m_noise_settings = settings;

    std::mt19937                        eng(settings.seed);
    std::uniform_real_distribution<f32> urd(-100000, 100000);

    std::vector<Vector2> octave_offsets(settings.octaves);
    for (u32 i = 0; i < settings.octaves; ++i)
    {
        octave_offsets[ i ].x = urd(eng) + settings.offset.x;
        octave_offsets[ i ].y = urd(eng) + settings.offset.y;
    }

    f32 min_noise_height = std::numeric_limits<f32>::max();
    f32 max_noise_height = std::numeric_limits<f32>::min();

    i32 half_width  = MAP_SIZE / 2;
    i32 half_height = MAP_SIZE / 2;

    for (i32 y = 0; y < MAP_SIZE; ++y)
    {
        for (i32 x = 0; x < MAP_SIZE; ++x)
        {
            f32 amplitude    = 1.0f;
            f32 frequency    = 1.0f;
            f32 noise_height = 0.0f;

            for (u32 i = 0; i < settings.octaves; ++i)
            {
                f32 sample_x = ( f32 ) (x - half_width) / settings.scale * frequency + octave_offsets[ i ].x;
                f32 sample_y = ( f32 ) (y - half_height) / settings.scale * frequency + octave_offsets[ i ].y;

                f32 perlin_value = perlin_noise_2d(Vector2(sample_x, sample_y)) * 2.0 - 1.0;

                noise_height += perlin_value * amplitude;

                amplitude *= settings.persistance;
                frequency *= settings.lacunarity;
            }

            if (noise_height > max_noise_height)
            {
                max_noise_height = noise_height;
            }
            else if (noise_height < min_noise_height)
            {
                min_noise_height = noise_height;
            }

            u32 idx                 = x * BPP + y * BPP * MAP_SIZE;
            m_noise_map.data[ idx ] = noise_height;
        }
    }

    for (u32 y = 0; y < MAP_SIZE; ++y)
    {
        for (u32 x = 0; x < MAP_SIZE; ++x)
        {
            u32 idx = x * BPP + y * BPP * MAP_SIZE;

            m_noise_map.data[ idx ]     = inverse_lerp(min_noise_height, max_noise_height, m_noise_map.data[ idx ]);
            m_noise_map.data[ idx + 1 ] = m_noise_map.data[ idx ];
            m_noise_map.data[ idx + 2 ] = m_noise_map.data[ idx ];
            m_noise_map.data[ idx + 3 ] = 1.0f;
        }
    }
}

void MapGenerator::update_terrain_map(const TerrainTypes& types)
{
    m_terrain_types = types;
    std::sort(m_terrain_types.begin(), m_terrain_types.end(), [](const auto& a, const auto& b) -> bool {
        return a.height > b.height;
    });

    u32 terrain_type_count = m_terrain_types.size();

    for (u32 i = 0; i < m_terrain_map.data.size(); i += 4)
    {
        for (u32 j = 0; j < terrain_type_count; ++j)
        {
            m_terrain_map.data[ i ]     = m_terrain_types[ terrain_type_count - 1 ].color.r;
            m_terrain_map.data[ i + 1 ] = m_terrain_types[ terrain_type_count - 1 ].color.g;
            m_terrain_map.data[ i + 2 ] = m_terrain_types[ terrain_type_count - 1 ].color.b;
            m_terrain_map.data[ i + 3 ] = 1.0f;
            if (m_terrain_types[ j ].height <= m_noise_map.data[ i ])
            {
                m_terrain_map.data[ i ]     = m_terrain_types[ j ].color.r;
                m_terrain_map.data[ i + 1 ] = m_terrain_types[ j ].color.g;
                m_terrain_map.data[ i + 2 ] = m_terrain_types[ j ].color.b;
                break;
            }
        }
    }
}