#include <SDL.h>
#include "core/os.hpp"

namespace fluent
{
void os_init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        LOG_ERROR("SDL init failed");
    }
}

void os_shutdown()
{
    SDL_Quit();
}
} // namespace fluent