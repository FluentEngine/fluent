#include <algorithm>
#include "map_generator.hpp"

void MapGenerator::set_map_size(u32 width, u32 height)
{
    m_width  = width;
    m_height = height;
}

void MapGenerator::update_noise_map(const NoiseSettings& settings)
{
    FT_ASSERT(m_width != 0 && m_height != 0);

    m_noise_settings = settings;

    m_noise_map = generate_noise_map(settings);

    // TODO: if bpp not 4?
    m_terrain_map.resize(m_width * m_height * 4);
}

void MapGenerator::update_terrain_map(const TerrainTypes& types)
{
    FT_ASSERT(m_width != 0 && m_height != 0);

    m_terrain_types = types;
    std::sort(m_terrain_types.begin(), m_terrain_types.end(), [](const auto& a, const auto& b) -> bool {
        return a.height > b.height;
    });

    for (u32 i = 0; i < m_noise_map.size(); i += 4)
    {
        for (u32 j = 0; j < m_terrain_types.size(); ++j)
        {
            m_terrain_map[ i ]     = m_terrain_types[ 0 ].color.r * 255.0f;
            m_terrain_map[ i + 1 ] = m_terrain_types[ 0 ].color.g * 255.0f;
            m_terrain_map[ i + 2 ] = m_terrain_types[ 0 ].color.b * 255.0f;
            m_terrain_map[ i + 3 ] = 255;
            if (m_terrain_types[ j ].height <= m_noise_map[ i ])
            {
                m_terrain_map[ i ]     = m_terrain_types[ j ].color.r * 255.0f;
                m_terrain_map[ i + 1 ] = m_terrain_types[ j ].color.g * 255.0f;
                m_terrain_map[ i + 2 ] = m_terrain_types[ j ].color.b * 255.0f;
                break;
            }
        }
    }
}