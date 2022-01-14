#include <random>
#include "noise.hpp"

NoiseMap generate_noise_map(const NoiseSettings& settings)
{
    using namespace fluent;

    std::mt19937                        eng(settings.seed);
    std::uniform_real_distribution<f32> urd(-100000, 100000);

    std::vector<Vector2> octave_offsets(settings.octaves);
    for (u32 i = 0; i < settings.octaves; ++i)
    {
        octave_offsets[ i ].x = urd(eng) + settings.offset.x;
        octave_offsets[ i ].y = urd(eng) + settings.offset.y;
    }

    static constexpr u32 bpp = 4;
    std::vector<f32>     image_data(settings.width * settings.height * bpp);

    f32 min_noise_height = std::numeric_limits<f32>::max();
    f32 max_noise_height = std::numeric_limits<f32>::min();

    i32 half_width  = settings.width / 2;
    i32 half_height = settings.height / 2;

    for (i32 y = 0; y < settings.height; ++y)
    {
        for (i32 x = 0; x < settings.width; ++x)
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

            u32 idx           = x * bpp + y * bpp * settings.width;
            image_data[ idx ] = noise_height;
        }
    }

    NoiseMap result(image_data.size());

    for (u32 y = 0; y < settings.height; ++y)
    {
        for (u32 x = 0; x < settings.width; ++x)
        {
            u32 idx           = x * bpp + y * bpp * settings.width;
            result[ idx ]     = inverse_lerp(min_noise_height, max_noise_height, image_data[ idx ]) * 255.0f;
            result[ idx + 1 ] = result[ idx ];
            result[ idx + 2 ] = result[ idx ];
            result[ idx + 3 ] = 255;
        }
    }

    return result;
}
