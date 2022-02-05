#include <filesystem>
#include "utils/file_system.hpp"

namespace fluent
{
std::string FileSystem::m_exec_path;
std::string FileSystem::m_shaders_directory;
std::string FileSystem::m_textures_directory;
std::string FileSystem::m_models_directory;

std::string FileSystem::get_exec_path(char** argv)
{
    std::string exec_path =
        std::filesystem::weakly_canonical(std::filesystem::path(argv[ 0 ])).parent_path().string() + "/";

    return exec_path;
}

void FileSystem::init(char** argv)
{
    m_exec_path = get_exec_path(argv);
}

void FileSystem::set_shaders_directory(const std::string& path)
{
    m_shaders_directory = m_exec_path + std::filesystem::path(path).string() + "/";
}

void FileSystem::set_textures_directory(const std::string& path)
{
    m_textures_directory = m_exec_path + std::filesystem::path(path).string() + "/";
}

void FileSystem::set_models_directory(const std::string& path)
{
    m_models_directory = m_exec_path + std::filesystem::path(path).string() + "/";
}
} // namespace fluent
