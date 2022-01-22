#include <algorithm>
#include <random>
#include "map_generator.hpp"
#include "mesh_generator.hpp"

using namespace fluent;

MapGenerator::NoiseSettings MapGenerator::noise_settings;

Map MapGenerator::generate_noise_map(Vector2 const& offset)
{
    Map noise_map{};
    noise_map.data.resize(MAP_SIZE * MAP_SIZE);

    std::mt19937                        eng(noise_settings.seed);
    std::uniform_real_distribution<f32> urd(-100000, 100000);

    std::vector<Vector2> octave_offsets(noise_settings.octaves);

    for (u32 i = 0; i < noise_settings.octaves; ++i)
    {
        octave_offsets[ i ].x = urd(eng) + offset.x;
        octave_offsets[ i ].y = urd(eng) - offset.y;
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

            for (u32 i = 0; i < noise_settings.octaves; ++i)
            {
                f32 sample_x = (x - half_width + octave_offsets[ i ].x) / noise_settings.scale * frequency;
                f32 sample_y = (y - half_height + octave_offsets[ i ].y) / noise_settings.scale * frequency;

                f32 perlin_value = perlin_noise_2d(Vector2(sample_x, sample_y)) * 2.0 - 1.0;

                noise_height += perlin_value * amplitude;

                amplitude *= noise_settings.persistance;
                frequency *= noise_settings.lacunarity;
            }

            if (noise_height > max_noise_height)
            {
                max_noise_height = noise_height;
            }
            else if (noise_height < min_noise_height)
            {
                min_noise_height = noise_height;
            }

            noise_map.data[ x + y * MAP_SIZE ] = noise_height;
        }
    }

    return noise_map;
}