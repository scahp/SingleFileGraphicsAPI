#include "SingleFileGraphicsAPI.h"
#include "SingleFileDX11.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    Run_Triangle_DX11();
    return 0;
}
