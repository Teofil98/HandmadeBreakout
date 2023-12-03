#include <windows.h>
#include "hspaceinvaders.h"

int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    UNUSED(hInstance);
    UNUSED(hPrevInstance);
    UNUSED(lpCmdLine);
    UNUSED(nShowCmd);
    MessageBox(NULL, "Hello handmade Space invaders", "Handmade Space invaders",
        MB_OK | MB_ICONINFORMATION);
    return 0;
}
