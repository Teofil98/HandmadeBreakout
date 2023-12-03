#include <windows.h>
#include <winuser.h>

int CALLBACK WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd
)
{
    MessageBox(NULL, "Hello handmade Space invaders", "Handmade Space invaders",
                MB_OK | MB_ICONINFORMATION);
    return 0;
}