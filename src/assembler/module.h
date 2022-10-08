/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
**                                                                         **
**  Author: Aria Seiler                                                    **
**                                                                         **
**  This program is in the public domain. There is no implied warranty,    **
**  so use it at your own risk.                                            **
**                                                                         **
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct assembler_state {
   heap *Heap;
} assembler_state;

#ifdef _ASSEMBLER_MODULE
external API_EXPORT void Assembler_Load(platform_state *Platform, assembler_module *Module);
external API_EXPORT void Assembler_Init(void);
external API_EXPORT void Assembler_Update(void);
external API_EXPORT void Assembler_Unload(void);
#else
typedef void func_Assembler_Load(platform_state *Platform, assembler_module *Module);
typedef void func_Assembler_Init(void);
typedef void func_Assembler_Update(void);
typedef void func_Assembler_Unload(void);

global func_Assembler_Load   *Assembler_Load;
global func_Assembler_Unload *Assembler_Unload;
global func_Assembler_Init   *Assembler_Init;
global func_Assembler_Update *Assembler_Update;
#endif