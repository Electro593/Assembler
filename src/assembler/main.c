/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
**                                                                         **
**  Author: Aria Seiler                                                    **
**                                                                         **
**  This program is in the public domain. There is no implied warranty,    **
**  so use it at your own risk.                                            **
**                                                                         **
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <shared.h>

global stack *Stack;
global heap *_Heap;

#define INCLUDE_HEADER
   #include <assembler/lexer.c>
#undef INCLUDE_HEADER

#include <util/memory.c>
#include <util/string.c>

#define INCLUDE_SOURCE
   #include <assembler/lexer.c>
#undef INCLUDE_SOURCE

typedef struct instruction {
   c08 *Name;
   u32 Opcode : 6;
   u32 Fn     : 4;
   u32 Fmt    : 3;
   // u32 Size   : 1;
   // u32 RegCnt : 2;
   // u32 Mod    : 2;
   // u32 Op1    : 2;
   // u32 Op2    : 2;
   // u32 Op3    : 2;
   // u32 Unused : 9;
} instruction;

typedef enum assembly_fmt {
   FMT_B1R0  = 0b000,
   FMT_B1R0e = 0b001,
   FMT_B1R1  = 0b010,
   FMT_B2R0  = 0b011,
   FMT_B2R0e = 0b100,
   FMT_B2R1  = 0b101,
   FMT_B2R1f = 0b110,
   FMT_B2R2  = 0b111,
} assembly_fmt;

#define INSTRUCTIONS \
   INST("and",   0b00000, 0, FMT_B1R1) \
   INST("slt",   0b10000, 0, FMT_B1R1) \
   INST("or",    0b01000, 0, FMT_B1R1) \
   INST("xor",   0b11000, 0, FMT_B1R1) \
   INST("sll",   0b00100, 0, FMT_B1R1) \
   INST("sgt",   0b10100, 0, FMT_B1R1) \
   INST("srl",   0b01100, 0, FMT_B1R1) \
   INST("sra",   0b11100, 0, FMT_B1R1) \
   INST("add",   0b00010, 0, FMT_B1R1) \
   INST("sub",   0b10010, 0, FMT_B1R1) \
   INST("sac",   0b01010, 0, FMT_B1R1) \
   INST("lac",   0b11010, 0, FMT_B1R1) \
   INST("saci",  0b00110, 0, FMT_B1R0) \
   INST("jalr",  0b10110, 0, FMT_B1R1) \
   INST("icall", 0b01110, 0, FMT_B1R0) \
   INST("ecall", 0b11110, 0, FMT_B1R0) \
   INST("andi",  0b00001, 0, FMT_B1R0) \
   INST("slti",  0b10001, 0, FMT_B1R0) \
   INST("ori",   0b01001, 0, FMT_B1R0) \
   INST("xori",  0b11001, 0, FMT_B1R0) \
   INST("slli",  0b00101, 0, FMT_B1R0) \
   INST("sgti",  0b10101, 0, FMT_B1R0) \
   INST("srli",  0b01101, 0, FMT_B1R0) \
   INST("srai",  0b11101, 0, FMT_B1R0) \
   INST("addi",   0b0011, 0, FMT_B1R0e) \
   INST("lui",    0b1011, 0, FMT_B2R0e) \
   INST("push",  0b00000, 0, FMT_B2R1f) \
   INST("pop",   0b00000, 1, FMT_B2R1f) \
   INST("addsp", 0b00100, 0, FMT_B2R0) \
   INST("ssp",   0b01000, 0, FMT_B2R0) \
   INST("lsp",   0b01100, 0, FMT_B2R0) \
   INST("sm",    0b10000, 0, FMT_B2R2) \
   INST("lm",    0b10100, 0, FMT_B2R2) \
   INST("swp",   0b01001, 0, FMT_B2R2) \
   INST("jal",   0b10110, 0, FMT_B2R0) \
   INST("bez",   0b11010, 0, FMT_B2R0) \
   INST("bnez",  0b11110, 0, FMT_B2R0) \
   // INST("and",   0b00000, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("slt",   0b10000, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("or",    0b01000, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("xor",   0b11000, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("sll",   0b00100, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("sgt",   0b10100, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("srl",   0b01100, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("sra",   0b11100, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("add",   0b00010, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("sub",   0b10010, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("sac",   0b01010, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("lac",   0b11010, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("saci",  0b00110, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("jalr",  0b10110, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("icall", 0b01110, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("ecall", 0b11110, 0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   // INST("andi",  0b00001, 0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   // INST("slti",  0b10001, 0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   // INST("ori",   0b01001, 0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   // INST("xori",  0b11001, 0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   // INST("slli",  0b00101, 0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   // INST("sgti",  0b10101, 0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   // INST("srli",  0b01101, 0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   // INST("srai",  0b11101, 0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   // INST("addi",  0bx0011, 0b0000, 0b0, 0b00, 0b01, 0b10, 0b00, 0b00) \
   // INST("lui",   0bx1011, 0b0000, 0b1, 0b00, 0b01, 0b10, 0b00, 0b00) \

const u32 INSTMAP_SIZE = 2*(0
   #define INST(...) +1
   INSTRUCTIONS
   #undef INST
   );

#define GENERATE_TABLE 0

#if GENERATE_TABLE == 1
instruction InstMap[2*(0
   #define INST(...) + 1
   INSTRUCTIONS
   #undef INST
   )] = {0};
#else
#include <assembler/table.h>
#endif

internal u32
GetIndex(string Name)
{
   u32 Len = Name.Length;
   c08 *Text = Name.Text;
   u32 Hash = Len*3;
   while(Len--) {
      c08 C = *Text;
      u32 Val = (C >= 'A' && C <= 'Z') ? (C - 'A') : (C - 'a');
      Hash = Hash*7 + Val;
      Text++;
   }
   Hash ^= Hash >> 8;
   
   return Hash % INSTMAP_SIZE;
}

internal instruction
GetFromMap(string Name)
{
   u32 Index = GetIndex(Name);
   
   while(Mem_Cmp(Name.Text, InstMap[Index].Name, Name.Length) != EQUAL)
      Index = (Index+1) % INSTMAP_SIZE;
   
   return InstMap[Index];
   
}

internal u32
AddToMap(instruction Inst)
{
   u32 Index = GetIndex(CString(Inst.Name));
   
   u32 I = Index;
   while(InstMap[Index].Name) {
      Index = (Index+1) % INSTMAP_SIZE;
      Assert(I != Index);
   }
   
   InstMap[Index] = Inst;
   
   return Index;
   
}

internal c08 *
PrintTableEntry(c08 *C, instruction Inst)
{
   *C++ = '\t';
   *C++ = '{';
   if(!Inst.Name) {
      *C++ = '0';
   } else {
      *C++ = '"';
      c08 *N = Inst.Name;
      while(*N) *C++ = *N++;
      *C++ = '"';
      *C++ = ',';
      if(Inst.Opcode >= 10) *C++ = ((Inst.Opcode / 10) % 10) + '0';
      *C++ = (Inst.Opcode % 10) + '0';
      *C++ = ',';
      if(Inst.Fn >= 10) *C++ = ((Inst.Fn / 10) % 10) + '0';
      *C++ = (Inst.Fn % 10) + '0';
      *C++ = ',';
      *C++ = (Inst.Fmt % 10) + '0';
      // *C++ = (Inst.Size % 10) + '0';
      // *C++ = ',';
      // *C++ = (Inst.RegCnt % 10) + '0';
      // *C++ = ',';
      // *C++ = (Inst.Mod % 10) + '0';
      // *C++ = ',';
      // *C++ = (Inst.Op1 % 10) + '0';
      // *C++ = ',';
      // *C++ = (Inst.Op2 % 10) + '0';
      // *C++ = ',';
      // *C++ = (Inst.Op3 % 10) + '0';
   }
   *C++ = '}';
   *C++ = ',';
   *C++ = '\n';
   return C;
}

internal c08 *
ParseRegister(c08 *Start, c08 *End, u32 *IndexOut)
{
   c08 *I = Start;
   c08 *C = End;
   
   c08 RegType = *I++;
   if(RegType >= 'A' && RegType <= 'Z') RegType -= ('a' - 'A');
   Assert(RegType >= 'a' && RegType <= 'z');
   u32 RegIndex = 0;
   
   if(RegType == 'z') {
      Assert((*I == 'r' || *I == 'R') && (*(I+1) == 'o' || *(I+1) == 'O'));
      RegIndex = 15;
      I += 2;
   } else if(RegType == 'r') {
      Assert(*I == 'a' || *I == 'A');
      RegIndex = 3;
      I++;
   } else if(RegType == 's' || RegType == 'a' || RegType == 'x') {
      while(I < C && *I >= '0' && *I <= '9')
         RegIndex = RegIndex*10 + *I++ - '0';
      Assert(I <= C);
      
      if(RegType == 'a' && RegIndex <= 3) RegIndex += 4;
      else if(RegType == 's' && RegIndex > 2) RegIndex += 5;
      else if(RegType == 'a' && RegIndex > 3) RegIndex += 8;
   } else Assert(FALSE);
   
   *IndexOut = RegIndex;
   return I;
}

internal c08 *
ParseImmediate(c08 *Start, c08 *End, s32 *ImmOut)
{
   c08 *I = Start;
   c08 *C = End;
   
   c08 *ImmBase = I;
   while(I < C && *I >= '0' && *I <= '9')
      I++;
   Assert(I <= C);
   
   string ImmStr = CLString(ImmBase, (u64)I - (u64)ImmBase);
   s64 Imm = String_ToS64(ImmStr);
   Assert(Imm >= -16 && Imm <= 15);
   
   *ImmOut = Imm;
   return I;
}

external void
Assembler_Load(platform_state *Platform, module *Module)
{
   Stack = Platform->Stack;
   Stack_Push();
   
   #define EXPORT(ReturnType, Namespace, Name, ...) \
      Namespace##_##Name = Platform->Functions.Namespace##_##Name;
   #define X PLATFORM_FUNCS
   #include <x.h>
   
   u64 HeapSize = 48*1024*1024;
   vptr HeapMem = Platform_AllocateMemory(HeapSize);
   _Heap = Heap_Init(HeapMem, HeapSize);
   
   
   
   #if GENERATE_TABLE == 0
   u32 ArgIndex = 0;
   string AsmFileNoExt = CLString(NULL, 0);
   string AsmFileName = CLString(NULL, 0);
   string ObjFileName = CLString(NULL, 0);
   
   c08 *C = Platform->CommandLine;
   c08 *ArgBase = C;
   while(*C) {
      u64 ArgDiff = (u64)C - (u64)ArgBase;
      
      switch(*C) {
         case ' ': {
            if(ArgIndex) {
               string PrevArg = CLString(ArgBase, ArgDiff);
               
               switch(ArgIndex) {
                  case 1: AsmFileName = PrevArg; break;
                  case 2: ObjFileName = PrevArg; break;
               }
            }
            
            ArgBase = C+1;
            ArgIndex++;
         } break;
         
         case '.': {
            switch(ArgIndex) {
               case 1: {
                  AsmFileNoExt = CLString(ArgBase, ArgDiff);
               } break;
            }
         } break;
      }
      
      C++;
   }
   
   if(!AsmFileName.Text) return;
   AsmFileName = String_Terminate(AsmFileName);
   
   if(!AsmFileNoExt.Text) AsmFileNoExt = AsmFileName;
   else AsmFileNoExt = String_Terminate(AsmFileNoExt);
   
   if(!ObjFileName.Text) ObjFileName = String_Cat(AsmFileNoExt, CString(".o"));
   else ObjFileName = String_Terminate(ObjFileName);
   
   file_handle File;
   Platform_OpenFile(&File, AsmFileName.Text, FILE_READ);
   u64 FileLen = Platform_GetFileLength(File);
   c08 *FileData = Stack_Allocate(FileLen+1);
   Platform_ReadFile(File, FileData, FileLen, 0);
   FileData[FileLen] = 0;
   
   c08 Chars[] = "0123456789ABCDEF";
   
   C = FileData;
   c08 *InstBase = C;
   u08 *Buffer = Stack_GetCursor();
   c08 *B = Buffer;
   c08 *Backslash = NULL;
   do {
      u64 InstDiff = (u64)C - (u64)InstBase;
      
      switch(*C) {
         case ' ':
         case '\t':
         case '\r':
            break;
         
         case '#': {
            if(*(C+1) == '*') {
               C += 2;
               u32 CommentLevel = 1;
               while(*C && CommentLevel) {
                  if(*C == '#') {
                     if(*(C-1) == '*') CommentLevel--;
                     else if(*(C+1) == '*') CommentLevel++;
                  }
                  if(*C == '\n') {
                     *B++ = '\n';
                  }
                  C++;
               }
               InstBase = C+1;
               Assert(C, "Multi-line comment never ended!");
            } else {
               while(*C && *C != '\n') C++;
               *B++ = '\n';
               InstBase = C+1;
            }
         } break;
         
         case '\0':
         case ';':
         case '\n': {
            //TODO: Handle assertions
            
            #define SKIP_WHITESPACE(Cursor) \
               while(Cursor < C && (*Cursor == ' ' || *Cursor == '\t' || *Cursor == '\r' || *Cursor == '\\' || *Cursor == '\n')) \
                  Cursor++;
            
            SKIP_WHITESPACE(InstBase)
            
            if(Backslash) {
               Assert(*C == '\n');
               Backslash++;
               while(Backslash < C && *Backslash == '\r') Backslash++;
               Assert(Backslash == C);
               Backslash = NULL;
               break;
            }
            
            if(InstBase == C) break;
            
            if(*InstBase == '.') {
               
            } else {
               c08 *I = InstBase+1;
               while(I < C && ((*I >= 'a' && *I <= 'z') || (*I >= 'A' && *I <= 'Z')))
                  I++;
               Assert(InstBase+1 < I);
               Assert((u64)I - (u64)InstBase <= 8);
               
               string Name = CLString(InstBase, (u64)I - (u64)InstBase);
               instruction Inst = GetFromMap(Name);
               
               switch(Inst.Fmt) {
                  case FMT_B1R0: {
                     SKIP_WHITESPACE(I);
                     
                     s32 Imm;
                     I = ParseImmediate(I, C, &Imm);
                     Assert(Imm >= -8 && Imm <= 7);
                     
                     *B++ = Chars[(Imm << 1) | (Inst.Opcode >> 4)];
                     *B++ = Chars[Inst.Opcode & 0xF];
                  } break;
                  
                  case FMT_B1R0e: {
                     SKIP_WHITESPACE(I);
                     
                     s32 Imm;
                     I = ParseImmediate(I, C, &Imm);
                     Assert(Imm >= -16 && Imm <= 15);
                     
                     *B++ = Chars[Imm];
                     *B++ = Chars[Inst.Opcode];
                  } break;
                  
                  case FMT_B1R1: {
                     SKIP_WHITESPACE(I);
                     
                     u32 RegIndex;
                     I = ParseRegister(I, C, &RegIndex);
                     Assert(RegIndex < 8);
                     
                     *B++ = Chars[(RegIndex << 1) | (Inst.Opcode >> 4)];
                     *B++ = Chars[Inst.Opcode & 0xF];
                  } break;
                  
                  case FMT_B2R0: {
                     SKIP_WHITESPACE(I);
                     
                     s32 ImmS;
                     I = ParseImmediate(I, C, &ImmS);
                     Assert(ImmS >= -128 && ImmS <= 127);
                     u32 Imm = ImmS & 0xFF;
                     
                     *B++ = Chars[((Inst.Opcode >> 1) & 0xE) | Imm >> 7];
                     *B++ = Chars[(Imm >> 3) & 0xF];
                     *B++ = ' ';
                     *B++ = Chars[((Imm & 7) << 1) | ((Inst.Opcode >> 1) & 1)];
                     *B++ = Chars[((Inst.Opcode & 1) << 3) | 7];
                  } break;
                  
                  case FMT_B2R0e: {
                     
                  } break;
                  
                  case FMT_B2R1: {
                     Assert(FALSE, "Unimplemented!");
                  } break;
                  
                  case FMT_B2R1f: {
                     SKIP_WHITESPACE(I);
                     
                     u32 RegIndex;
                     I = ParseRegister(I, C, &RegIndex);
                     Assert(RegIndex < 16);
                     
                     *B++ = Chars[((Inst.Opcode >> 1) & 0xE) | RegIndex >> 3];
                     *B++ = Chars[((RegIndex & 0x7) << 1) | (Inst.Fn >> 3)];
                     *B++ = ' ';
                     *B++ = Chars[((Inst.Fn & 0x7) << 1) | ((Inst.Opcode >> 1) & 1)];
                     *B++ = Chars[((Inst.Opcode & 1) << 3) | 7];
                  } break;
                  
                  case FMT_B2R2: {
                     
                  } break;
                  
               }
               
               *B++ = '\n';
            }
            
            InstBase = C+1;
         } break;
         
         case '\\': {
            Backslash = C;
         } break;
      }
   } while(*C++);
   
   Platform_CloseFile(File);
   Platform_OpenFile(&File, ObjFileName.Text, FILE_WRITE);
   Platform_WriteFile(File, Buffer, (u64)B - (u64)Buffer, 0);
   Platform_CloseFile(File);
   
   #else

   c08 *Buffer = Stack_GetCursor();
   c08 *C = Buffer;
   
   string MapLen = U32_ToString(INSTMAP_SIZE, 10);
   
   c08 Header0[] = "instruction InstMap[";
   c08 Header1[] = "] = {\n";
   Mem_Cpy(C, Header0, sizeof(Header0)-1);
   C += sizeof(Header0)-1;
   Mem_Cpy(C, MapLen.Text, MapLen.Length);
   C += MapLen.Length;
   Mem_Cpy(C, Header1, sizeof(Header1)-1);
   C += sizeof(Header1)-1;
   
   #define INST(Name, ...) \
      AddToMap((instruction){Name, __VA_ARGS__});
   INSTRUCTIONS
   #undef INST
   
   for(u32 I = 0; I < INSTMAP_SIZE; I++) {
      C = PrintTableEntry(C, InstMap[I]);
   }
   
   *C++ = '}';
   *C++ = ';';
   
   file_handle File;
   Platform_OpenFile(&File, "src/assembler/table.h", FILE_WRITE);
   Platform_WriteFile(File, Buffer, (u64)C - (u64)Buffer, 0);
   Platform_CloseFile(File);
   #endif
   
   Stack_Pop();
}

external void
Assembler_Unload(void)
{
   
}