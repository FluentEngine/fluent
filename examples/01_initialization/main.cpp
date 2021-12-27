#include <iostream>

#include "fluent/fluent.hpp"
#include <SDL.h>

using namespace fluent;

int main()
{
    fluent::os_init();

    WindowDescription desc{};
    desc.title = "fluent-engine";
    desc.x = 0;
    desc.y = 0;
    desc.width = 800;
    desc.height = 600;

    auto window = fluent::create_window(desc);

    SDL_Event e;
    bool quit = false;

    // main loop
    while (!quit)
    {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            // close the window when user clicks the X button or alt-f4s
            if (e.type == SDL_QUIT)
                quit = true;
        }
    }

    fluent::destroy_window(window);
    fluent::os_shutdown();

    return 0;
}
