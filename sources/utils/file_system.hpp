#pragma once

#include <string>
#include "core/base.hpp"
#include "renderer/renderer_backend.hpp"

namespace fluent
{
class FileSystem
{
private:
    static std::string m_exec_path;
    static std::string m_shaders_directory;
    static std::string m_textures_directory;
    static std::string m_models_directory;

    static std::string get_exec_path(char** argv);

public:
    static void init(char** argv);
    static void set_shaders_directory(const std::string& path);
    static void set_textures_directory(const std::string& path);
    static void set_models_directory(const std::string& path);

    static const std::string& get_shaders_directory()
    {
        return m_shaders_directory;
    }

    static const std::string& get_textures_directory()
    {
        return m_textures_directory;
    }

    static const std::string& get_models_directory()
    {
        return m_models_directory;
    }
};
} // namespace fluent
