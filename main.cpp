#include "System.h"
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_messagebox.h>

// Main code
int main(int, char**)
{
    if (auto error = sys().Init())
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error.value(), nullptr);
        return -1;
    }

    while (!sys().Run()) ;

    sys().Shutdown();

    return 0;
}