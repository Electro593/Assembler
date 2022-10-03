/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
**                                                                         **
**  Author: Aria Seiler                                                    **
**                                                                         **
**  This program is in the public domain. There is no implied warranty,    **
**  so use it at your own risk.                                            **
**                                                                         **
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define _GRAPHICS _OPENGL || _DIRECTX

#define FONTS_DIR    "assets\\fonts\\"
#define SHADERS_DIR  "assets\\shaders\\"
#define TEXTURES_DIR "assets\\textures\\"

typedef struct module {
    datetime LastWriteTime;
    win32_module DLL;
    vptr State;
    b08 ShouldBeInitialized;
    b08 ShouldBeUpdated;
    b08 RequiresGraphics;
} module;