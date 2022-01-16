#include <algorithm>
#include "map_generator.hpp"

void MapGenerator::set_map_size(u32 width, u32 height)
{
    m_noise_map.width  = width;
    m_noise_map.height = height;
    m_noise_map.bpp    = 4;

    m_terrain_map.width  = width;
    m_terrain_map.height = height;
    m_terrain_map.bpp    = 4;

    m_noise_map.data.resize(m_noise_map.width * m_noise_map.height * m_noise_map.bpp);
    m_terrain_map.data.resize(m_terrain_map.width * m_terrain_map.height * m_terrain_map.bpp);
}

void MapGenerator::update_noise_map(const NoiseSettings& settings)
{
    FT_ASSERT(m_noise_map.width != 0 && m_noise_map.height != 0 && m_noise_map.bpp != 0);

    m_noise_settings = settings;

    m_noise_map = generate_noise_map(settings);
}

void MapGenerator::update_terrain_map(const TerrainTypes& types)
{
    FT_ASSERT(m_terrain_map.width != 0 && m_terrain_map.height != 0 && m_terrain_map.bpp != 0);

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