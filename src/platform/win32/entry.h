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

#define MODULE(Name, name, ...)    \
    struct name##_module {         \
        datetime LastWriteTime;    \
        win32_module DLL;          \
        b08 ShouldBeInitialized;   \
        b08 ShouldBeUpdated;       \
        b08 RequiresGraphics;      \
        name##_state State;        \
    };
MODULES
#undef MODULE