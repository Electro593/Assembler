/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
**                                                                         **
**  Author: Aria Seiler                                                    **
**                                                                         **
**  This program is in the public domain. There is no implied warranty,    **
**  so use it at your own risk.                                            **
**                                                                         **
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
root = section,{section} | {statement}
section = '.section',whitespace,ident,line_end,{statement}
statement = {label},(instruction | data)
instruction = ident,whitespace,[(reg|expression),{',',(reg|expression)}],line_end
data = 'bytes',whitespace,expression,',',(expression | '[',expression,{',',expression},']'),line_end
expression = number | label_reference | {'('},[expression],{op,expression},{')'}

comment = '#',{printable characters},line_end | '#*',{comment|printable characters},'*#'
op = '-','*','<<','>>','+','/','%','^','&','|','~','!','<','>','<=','>=','==','!=','^^','&&','||','(',')'
label = ident,':',{line_end}
label_reference = ident
reg = 'zro'|'ra'|('s',('0'|'1'|'2'|'3'|'4'|'5'|'6'))|('a',('0'|'1'|'2'|'3'|'4'|'5'|'6'))
ident = (letters|'_'),{letters,numbers,'_'}
number = numbers,{numbers} | (numbers|letters|'$'|'@'),{numbers|letters|'$'|'@'},radix
radix = '`',numbers,{numbers}
letters = ? All alphabetical letters, lowercase and uppercase ?
numbers = '0'|'1'|'2'|'3'|'4'|'5'|'6'|'7'|'8'|'9'|'_'
line_end = -'\\','\n' | ';'
whitespace = {' '|'\t'|'\r'}
*/

#include <shared.h>

global stack *Stack;
global heap *_Heap;

typedef struct instruction {
   u32 Type   : 2;
   u32 Opcode : 5;
   u32 Fn     : 4;
   u32 Fmt    : 3;
   u32 Size   : 1;
} instruction;

#define INCLUDE_HEADER
   #include <assembler/lexer.c>
   #include <assembler/parser.c>
   #include <assembler/generator.c>
#undef INCLUDE_HEADER

#include <util/memory.c>
#include <util/string.c>
#include <util/set.c>

#define AAssert(Expression, ...) \
   do { \
      if(!(Expression)) { \
         c08 Message[] = "(In the assembler's " __FILE__ " at line " STRINGIFY(__LINE__) "): " __VA_ARGS__ "\n"; \
         Assembler_Log(NULL, NULL, AssemblerErrorF, CLStringL(Message)); \
      } \
   } while(0)

typedef enum instruction_type {
   InstType_Text = 0b00,
   InstType_Data = 0b01,
   InstType_BSS  = 0b10,
} instruction_type;

typedef enum instruction_fmt {
   FMT_B1R0  = 0b000,
   FMT_B1R0e = 0b001,
   FMT_B1R1  = 0b010,
   FMT_B2R0  = 0b011,
   FMT_B2R0e = 0b100,
   FMT_B2R1  = 0b101,
   FMT_B2R1f = 0b110,
   FMT_B2R2  = 0b111,
} instruction_fmt;

   //    Name                   Opcode     Fmt        Fn   Size  OpN   Mod   Op1   Op2   Op3
#define INSTRUCTIONS \
   INST("and",   InstType_Text, 0b00000, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("slt",   InstType_Text, 0b10000, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("or",    InstType_Text, 0b01000, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("xor",   InstType_Text, 0b11000, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("sll",   InstType_Text, 0b00100, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("sgt",   InstType_Text, 0b10100, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("srl",   InstType_Text, 0b01100, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("sra",   InstType_Text, 0b11100, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("add",   InstType_Text, 0b00010, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("sub",   InstType_Text, 0b10010, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("sac",   InstType_Text, 0b01010, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("lac",   InstType_Text, 0b11010, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("saci",  InstType_Text, 0b00110, FMT_B1R0,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("jalr",  InstType_Text, 0b10110, FMT_B1R1,  0b0000, 0b0, 0b01, 0b00, 0b01, 0b00, 0b00) \
   INST("icall", InstType_Text, 0b01110, FMT_B1R0,  0b0000, 0b0, 0b00, 0b00, 0b01, 0b00, 0b00) \
   INST("ecall", InstType_Text, 0b11110, FMT_B1R0,  0b0000, 0b0, 0b00, 0b00, 0b01, 0b00, 0b00) \
   INST("andi",  InstType_Text, 0b00001, FMT_B1R0,  0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("slti",  InstType_Text, 0b10001, FMT_B1R0,  0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("ori",   InstType_Text, 0b01001, FMT_B1R0,  0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("xori",  InstType_Text, 0b11001, FMT_B1R0,  0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("slli",  InstType_Text, 0b00101, FMT_B1R0,  0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("sgti",  InstType_Text, 0b10101, FMT_B1R0,  0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("srli",  InstType_Text, 0b01101, FMT_B1R0,  0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("srai",  InstType_Text, 0b11101, FMT_B1R0,  0b0000, 0b0, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("addi",  InstType_Text,  0b0011, FMT_B1R0e, 0b0000, 0b0, 0b00, 0b01, 0b10, 0b00, 0b00) \
   INST("lui",   InstType_Text,  0b1011, FMT_B2R0e, 0b0000, 0b1, 0b00, 0b01, 0b10, 0b00, 0b00) \
   INST("push",  InstType_Text, 0b00000, FMT_B2R1f, 0b1110, 0b1, 0b01, 0b10, 0b00, 0b00, 0b00) \
   INST("pop",   InstType_Text, 0b00000, FMT_B2R1f, 0b0010, 0b1, 0b01, 0b10, 0b00, 0b00, 0b00) \
   INST("setsp", InstType_Text, 0b00000, FMT_B2R1f, 0b0000, 0b1, 0b01, 0b10, 0b00, 0b00, 0b00) \
   INST("addsp", InstType_Text, 0b00100, FMT_B2R0,  0b0000, 0b1, 0b00, 0b00, 0b00, 0b00, 0b00) \
   INST("ssp",   InstType_Text, 0b01000, FMT_B2R0,  0b0000, 0b1, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("lsp",   InstType_Text, 0b01100, FMT_B2R0,  0b0000, 0b1, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("spc",   InstType_Text, 0b11000, FMT_B2R0,  0b0000, 0b1, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("lpc",   InstType_Text, 0b11100, FMT_B2R0,  0b0000, 0b1, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("sm",    InstType_Text, 0b10000, FMT_B2R2,  0b0000, 0b1, 0b10, 0b00, 0b01, 0b01, 0b00) \
   INST("lm",    InstType_Text, 0b10100, FMT_B2R2,  0b0000, 0b1, 0b10, 0b00, 0b01, 0b01, 0b00) \
   INST("swp",   InstType_Text, 0b01001, FMT_B2R2,  0b0000, 0b1, 0b10, 0b00, 0b01, 0b01, 0b00) \
   INST("jal",   InstType_Text, 0b10110, FMT_B2R0,  0b0000, 0b1, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("bez",   InstType_Text, 0b11010, FMT_B2R0,  0b0000, 0b1, 0b00, 0b00, 0b10, 0b00, 0b00) \
   INST("bnez",  InstType_Text, 0b11110, FMT_B2R0,  0b0000, 0b1, 0b00, 0b00, 0b10, 0b00, 0b00) \
   \
   INST("bytes", InstType_Data, 0,0,0,0,0,0,0,0,0) \
   INST("resvd", InstType_BSS,  0,0,0,0,0,0,0,0,0) \

const u32 INSTMAP_SIZE = 2*(0
   #define INST(...) +1
   INSTRUCTIONS
   #undef INST
   );

#define GENERATE_TABLE 0

#if GENERATE_TABLE == 0
#pragma pack(push, 1)
typedef struct instmap_entry {
   hashmap_header Header;
   string Key;
   instruction Value;
} instmap_entry;
#include <assembler/table.h>
#pragma pack(pop)
#endif

#define INCLUDE_SOURCE
   #include <assembler/lexer.c>
   #include <assembler/parser.c>
   #include <assembler/generator.c>
#undef INCLUDE_SOURCE

internal u64
Assembler_LabelHash(string *Name)
{
   u64 Hash = Name->Length * 3;
   
   for(u32 I = 0; I < Name->Length; I++)
      Hash = Hash*7 + Name->Text[I];
   
   Hash ^= Hash >> 7;
   return Hash;
}

internal u64
Assembler_InstrHash(string *Name)
{
   u64 Hash = Name->Length * 3;
   
   for(u32 I = 0; I < Name->Length; I++) {
      c08 C = Name->Text[I];
      u32 Val = (C < 'a') ? C : (C - ('a' - 'A'));
      Hash = Hash*7 + Val;
   }
   
   Hash ^= Hash >> 8;
   return Hash;
}

#if GENERATE_TABLE == 0
heap_handle DummyHandle = {
   InstMapArr,
   0,
   0,
   FALSE,
   TRUE,
   sizeof(InstMapArr),
   0,
   0,
   0,
   0,
   0,
   0,
};

hashmap _InstMap = {
   NULL,
   0,
   sizeof(hashmap_header) + sizeof(string) + sizeof(instruction),
   sizeof(string),
   sizeof(instruction),
   2.0,
   0.5,
   NULL,
   (cmp_func*)&_String_CmpCaseInsensitive,
   (hash_func*)&Assembler_InstrHash
};

#endif

internal c08 *
PrintU64ToBuffer(c08 *C, u64 Value)
{
   u64 _Value = Value;
   u32 Len = 0;
   do { Len++; } while(Value /= 10);
   Value = _Value;
   u32 _Len = Len;
   do { C[--Len] = (Value % 10) + '0'; } while(Value /= 10);
   return C + _Len;
}

internal c08 *
PrintTableEntry(c08 *C, u64 Hash, string Key, instruction Value)
{
   *C++ = '\t';
   *C++ = '{';
   if(!(Hash & HASH_EXISTS)) {
      *C++ = '0';
   } else {
      *C++ = '{';
      C = PrintU64ToBuffer(C, Hash);
      *C++ = '}';
      *C++ = ',';
      *C++ = '{';
      C = PrintU64ToBuffer(C, Key.Length);
      *C++ = ',';
      C = PrintU64ToBuffer(C, Key.Capacity);
      *C++ = ',';
      C = PrintU64ToBuffer(C, Key.Resizable);
      *C++ = ',';
      *C++ = '"';
      c08 *N = Key.Text;
      while(*N) *C++ = *N++;
      *C++ = '"';
      *C++ = '}';
      *C++ = ',';
      *C++ = '{';
      C = PrintU64ToBuffer(C, Value.Type);
      *C++ = ',';
      C = PrintU64ToBuffer(C, Value.Opcode);
      *C++ = ',';
      C = PrintU64ToBuffer(C, Value.Fn);
      *C++ = ',';
      C = PrintU64ToBuffer(C, Value.Fmt);
      *C++ = ',';
      C = PrintU64ToBuffer(C, Value.Size);
      *C++ = '}';
   }
   *C++ = '}';
   *C++ = ',';
   *C++ = '\n';
   return C;
}

external void
Assembler_Load(platform_state *Platform, assembler_module *Module)
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
   _InstMap.Array = &DummyHandle;
   _InstMap.EntryCount = InstMapEntryCount;
   
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
   string AsmFileNameT = String_Terminate(AsmFileName);
   
   if(!AsmFileNoExt.Text) AsmFileNoExt = AsmFileNameT;
   else AsmFileNoExt = String_Terminate(AsmFileNoExt);
   
   if(!ObjFileName.Text) ObjFileName = String_Cat(AsmFileNoExt, CString(".o"));
   else ObjFileName = String_Terminate(ObjFileName);
   
   file_handle File;
   Platform_OpenFile(&File, AsmFileNameT.Text, FILE_READ);
   u64 FileLen = Platform_GetFileLength(File);
   c08 *FileData = Heap_AllocateA(_Heap, FileLen+1);
   string FileStr = CLString(FileData, FileLen);
   Platform_ReadFile(File, FileData, FileLen, 0);
   Platform_CloseFile(File);
   FileData[FileLen] = 0;
   
   hashmap LabelMap = HashMap_Init(_Heap, sizeof(string), sizeof(u64), 13, 2, 0.5, (hash_func*)&Assembler_LabelHash, (cmp_func*)&_String_Cmp, NULL);
   
   token *Tokens = TokenizeFile(FileStr);
   parser Parser = ParseTokens(Tokens, &_InstMap, AsmFileName);
   heap_handle *Output = GenerateASM((ast_node*)Parser.AST, &LabelMap);
   
   Platform_OpenFile(&File, ObjFileName.Text, FILE_WRITE);
   Platform_WriteFile(File, Output->Data, Output->Size, 0);
   Platform_CloseFile(File);
   #else

   c08 *Buffer = Stack_GetCursor();
   c08 *C = Buffer;
   
   hashmap Map = HashMap_Init(_Heap, sizeof(string), sizeof(instruction), 67, 2, 0.5, (hash_func*)&Assembler_InstrHash, (cmp_func*)&_String_Cmp, NULL);
   
   string NameStr;
   instruction Inst;
   #define INST(Name, Type, Opcode, Fmt, Fn, Size, OpN, Mod, Op1, Op2, Op3) \
      NameStr = CString(Name); \
      Inst = (instruction){Type, Opcode, Fn, Fmt, Size}; \
      HashMap_Add(&Map, &NameStr, &Inst);
   INSTRUCTIONS
   #undef INST
   
   c08 InstMapCountStr[] = "u64 InstMapEntryCount = ";
   Mem_Cpy(C, InstMapCountStr, sizeof(InstMapCountStr)-1);
   C += sizeof(InstMapCountStr)-1;
   
   u64 Value = Map.EntryCount;
   u32 Len = 0;
   do { Len++; } while(Value /= 10);
   Value = Map.EntryCount;
   u32 _Len = Len;
   do { C[--Len] = (Value % 10) + '0'; } while(Value /= 10);
   C += _Len;
   
   c08 Header0[] = ";\n\ninstmap_entry InstMapArr[";
   Mem_Cpy(C, Header0, sizeof(Header0)-1);
   C += sizeof(Header0)-1;
   
   u64 MapCount = Map.Array->Size / Map.EntrySize;
   
   Value = MapCount;
   Len = 0;
   do { Len++; } while(Value /= 10);
   Value = MapCount;
   _Len = Len;
   do { C[--Len] = (Value % 10) + '0'; } while(Value /= 10);
   C += _Len;
   
   c08 Header1[] = "] = {\n";
   Mem_Cpy(C, Header1, sizeof(Header1)-1);
   C += sizeof(Header1)-1;
   
   for(u32 I = 0; I < MapCount; I++) {
      hashmap_header *Header = (vptr)((u08*)Map.Array->Data + I*Map.EntrySize);
      if(Header->Hash & HASH_EXISTS) {
         string *StrPtr = (vptr)((u08*)Map.Array->Data + I*Map.EntrySize + sizeof(hashmap_header));
         instruction *InstPtr = (vptr)((u08*)Map.Array->Data + I*Map.EntrySize + sizeof(hashmap_header) + Map.KeySize);
         C = PrintTableEntry(C, Header->Hash, *StrPtr, *InstPtr);
      } else {
         *C++ = '\t';
         *C++ = '{';
         *C++ = '0';
         *C++ = '}';
         *C++ = ',';
         *C++ = '\n';
      }
   }
   
   *C++ = '}';
   *C++ = ';';
   
   file_handle File;
   Platform_OpenFile(&File, "src/assembler/table.h", FILE_WRITE);
   Platform_WriteFile(File, Buffer, (u64)C - (u64)Buffer, 0);
   Platform_CloseFile(File);
   #endif
   
   Platform_FreeMemory(_Heap);
   Stack_Pop();
}

external void
Assembler_Unload(void)
{
   
}