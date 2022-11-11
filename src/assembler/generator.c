/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
**                                                                         **
**  Author: Aria Seiler                                                    **
**                                                                         **
**  This program is in the public domain. There is no implied warranty,    **
**  so use it at your own risk.                                            **
**                                                                         **
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef INCLUDE_HEADER

#endif



#ifdef INCLUDE_SOURCE

internal b08
GetImmediate(hashmap *LabelMap, ast_node *Imm, u64 FileLoc, s64 *ValueOut)
{
   switch(Imm->Type) {
      case ASTType_Immediate: {
         *ValueOut = ((ast_immediate*)Imm)->Value;
      } break;
      
      case ASTType_Identifier: {
         u64 *Label = HashMap_Get(LabelMap, &Imm->Token->Str);
         if(Label) {
            *ValueOut = (s64)*Label - (s64)FileLoc;
         } else {
            return FALSE;
         }
      } break;
      
      case ASTType_Operation: {
         //TODO: Make this iterative
         
         ast_operation *Op = (vptr)Imm;
         
         s64 ValueA = 0;
         s64 ValueB = 0;
         
         if(Imm->Token->Type == TokenType_Subtraction && Op->OperatorIndex == 0) {
            if(!GetImmediate(LabelMap, Imm->FirstChild, FileLoc, &ValueB))
               return FALSE;
            
            *ValueOut = ValueA - ValueB;
         } else {
            Assert(FALSE, "Unsupported operator.");
         }
         
      } break;
      
      default: {
         Assert(FALSE, "Immediates must be numbers or labels.");
      }
   }
   
   return TRUE;
}

internal void
GetRegister(ast_node *Reg, u08 *ValueOut)
{
   switch(Reg->Type) {
      case ASTType_Identifier: {
         *ValueOut = ParseRegister(Reg->Token->Str);
      } break;
      
      default: {
         Assert(FALSE, "Registers must be identifiers.");
      }
   }
}

internal void
YieldInstruction(heap_handle *Yielded, ast_instruction *Inst, u64 FileLoc)
{
   u64 YieldedSize = Yielded->Size;
   Heap_Resize(Yielded, YieldedSize + sizeof(ast_instruction*) + sizeof(u64));
   
   ast_instruction **YieldedInst = (vptr)((u08*)Yielded->Data + YieldedSize);
   u64 *YieldedFileLoc = (vptr)(YieldedInst+1);
   
   *YieldedInst    = Inst;
   *YieldedFileLoc = FileLoc;
}

internal void
GenerateInstruction_B1R0(hashmap *LabelMap, heap_handle *Yielded,
                         ast_instruction *Inst, u64 *FileLoc, heap_handle *Output)
{
   instruction InstData = Inst->Instruction;
   
   Assert(InstData.Size == 0 && InstData.Fmt == FMT_B1R0);
   Assert(Inst->Header.ChildCount == 1);
   
   s64 Imm;
   if(!GetImmediate(LabelMap, Inst->Header.FirstChild, *FileLoc, &Imm)) {
      YieldInstruction(Yielded, Inst, *FileLoc);
   } else {
      Assert((-4 <= Imm && Imm <= 3) || (0 <= Imm && Imm <= 7), "1-Byte immediates must be between -4 and 3, inclusive.");
      ((u08*)Output->Data)[*FileLoc+0] = (Imm << 5) | InstData.Opcode;
   }
   
   *FileLoc += 1;
}

internal void
GenerateInstruction_B1R0e(hashmap *LabelMap, heap_handle *Yielded,
                          ast_instruction *Inst, u64 *FileLoc, heap_handle *Output)
{
   instruction InstData = Inst->Instruction;
   
   Assert(InstData.Size == 0 && InstData.Fmt == FMT_B1R0e);
   Assert(Inst->Header.ChildCount == 1);
   
   s64 Imm;
   if(!GetImmediate(LabelMap, Inst->Header.FirstChild, *FileLoc, &Imm)) {
      YieldInstruction(Yielded, Inst, *FileLoc);
   } else {
      Assert((-8 <= Imm && Imm <= 7) || (0 <= Imm && Imm <= 15), "1-Byte extended immediates must be between -8 and 7, inclusive.");
      ((u08*)Output->Data)[*FileLoc+0] = (Imm << 4) | (InstData.Opcode & 0xF);
   }
   
   *FileLoc += 1;
}

internal void
GenerateInstruction_B1R1(ast_instruction *Inst, u64 *FileLoc, heap_handle *Output)
{
   instruction InstData = Inst->Instruction;
   
   Assert(InstData.Size == 0 && InstData.Fmt == FMT_B1R1);
   Assert(Inst->Header.ChildCount == 1);
   
   u08 Reg;
   GetRegister(Inst->Header.FirstChild, &Reg);
   ((u08*)Output->Data)[*FileLoc+0] = (Reg << 5) | InstData.Opcode;
   
   *FileLoc += 1;
}

internal void
GenerateInstruction_B2R0(hashmap *LabelMap, heap_handle *Yielded,
                         ast_instruction *Inst, u64 *FileLoc, heap_handle *Output)
{
   instruction InstData = Inst->Instruction;
   
   Assert(InstData.Size == 1 && InstData.Fmt == FMT_B2R0);
   Assert(Inst->Header.ChildCount == 1);
   
   s64 Imm;
   if(!GetImmediate(LabelMap, Inst->Header.FirstChild, *FileLoc, &Imm)) {
      YieldInstruction(Yielded, Inst, *FileLoc);
   } else {
      Assert((-128 <= Imm && Imm <= 127) || (0 <= Imm && Imm <= 255), "2-Byte large immediates must be between -128 and 127, inclusive.");
      ((u08*)Output->Data)[*FileLoc+0] = ((Imm & 0x7) << 5) | ((InstData.Opcode & 0x3) << 3) | 0b111;
      ((u08*)Output->Data)[*FileLoc+1] = ((InstData.Opcode & 0x1C) << 3) | ((Imm & 0xF8) >> 3);
   }
   
   *FileLoc += 2;
}

internal void
GenerateInstruction_B2R0e(hashmap *LabelMap, heap_handle *Yielded,
                          ast_instruction *Inst, u64 *FileLoc, heap_handle *Output)
{
   instruction InstData = Inst->Instruction;
   
   Assert(InstData.Size == 1 && InstData.Fmt == FMT_B2R0e);
   Assert(Inst->Header.ChildCount == 1);
   
   s64 Imm;
   if(!GetImmediate(LabelMap, Inst->Header.FirstChild, *FileLoc, &Imm)) {
      YieldInstruction(Yielded, Inst, *FileLoc);
   } else {
      Assert((-2048 <= Imm && Imm <= 2047) || (0 <= Imm && Imm <= 4095), "2-Byte extended immediates must be between -2048 and 2047, inclusive.");
      ((u08*)Output->Data)[*FileLoc+0] = ((Imm & 0xF) << 4) | (InstData.Opcode & 0xF);
      ((u08*)Output->Data)[*FileLoc+1] = Imm >> 4;
   }
   
   *FileLoc += 2;
}

internal void
GenerateInstruction_B2R1f(ast_instruction *Inst, u64 *FileLoc, heap_handle *Output)
{
   instruction InstData = Inst->Instruction;
   
   Assert(InstData.Size == 1 && InstData.Fmt == FMT_B2R1f);
   Assert(Inst->Header.ChildCount == 1);
   
   u08 Reg;
   GetRegister(Inst->Header.FirstChild, &Reg);
   ((u08*)Output->Data)[*FileLoc+0] = ((InstData.Fn & 0x7) << 5) | ((InstData.Opcode & 0x3) << 3) | 0b111;
   ((u08*)Output->Data)[*FileLoc+1] = ((InstData.Opcode & 0x1C) << 3) | (Reg << 1) | (InstData.Fn >> 3);
   
   *FileLoc += 2;
}

internal void
GenerateInstruction_B2R2(ast_instruction *Inst, u64 *FileLoc, heap_handle *Output)
{
   instruction InstData = Inst->Instruction;
   
   Assert(InstData.Size == 1 && InstData.Fmt == FMT_B2R2);
   Assert(Inst->Header.ChildCount == 2);
   
   u08 RegA;
   u08 RegB;
   GetRegister(Inst->Header.FirstChild, &RegA);
   GetRegister(Inst->Header.FirstChild->Next, &RegB);
   ((u08*)Output->Data)[*FileLoc+0] = ((RegB & 0x7) << 5) | ((InstData.Opcode & 0x3) << 3) | 0b111;
   ((u08*)Output->Data)[*FileLoc+1] = ((InstData.Opcode & 0x1C) << 3) | (RegA << 1) | (RegB >> 3);
   
   *FileLoc += 2;
}

internal heap_handle *
GenerateASM(ast_node *AST, hashmap *LabelMap)
{
   u64 FileInc = 512;
   u64 FileSize = 0;
   heap_handle *Output = Heap_Allocate(_Heap, 0);
   
   heap_handle *Yielded = Heap_Allocate(_Heap, 0);
   
   vptr StackStart = Stack_GetCursor();
   ast_node **Cursor = StackStart;
   *Cursor++ = AST;
   
   while((u64)Cursor > (u64)StackStart) {
      ast_node *Node = *(--Cursor);
      
      switch(Node->Type) {
         case ASTType_Invalid:
            Assert(FALSE, "Generation error: Invalid AST type!");
            break;
         
         case ASTType_Root: {
            ast_node *Child = Node->FirstChild->Prev;
            
            for(u32 I = 0; I < Node->ChildCount; I++) {
               *Cursor++ = Child;
               Child = Child->Prev;
            }
         } break;
         
         case ASTType_Section: {
            ast_section *Section = (vptr)Node;
            
            if(String_Cmp(Node->Token->Str, CLStringL("text")) == EQUAL) {
               ast_node *Child = Node->FirstChild->Prev;
               
               for(u32 I = 0; I < Node->ChildCount; I++) {
                  *Cursor++ = Child;
                  Child = Child->Prev;
               }
            } else {
               Assert(FALSE, "Unrecognized section!");
            }
         } break;
         
         case ASTType_Statement: {
            ast_node *Child = Node->FirstChild->Prev;
            
            for(u32 I = 0; I < Node->ChildCount; I++) {
               *Cursor++ = Child;
               Child = Child->Prev;
            }
         } break;
         
         case ASTType_Instruction: {
            ast_instruction *Inst = (vptr)Node;
            
            Assert(FileInc >= Inst->Instruction.Size+1);
            if(FileSize+Inst->Instruction.Size+1 > Output->Size)
               Heap_Resize(Output, FileSize+FileInc);
            
            switch(Inst->Instruction.Fmt) {
               case FMT_B1R0:  GenerateInstruction_B1R0 (LabelMap, Yielded, Inst, &FileSize, Output); break;
               case FMT_B1R0e: GenerateInstruction_B1R0e(LabelMap, Yielded, Inst, &FileSize, Output); break;
               case FMT_B1R1:  GenerateInstruction_B1R1 (                   Inst, &FileSize, Output); break;
               case FMT_B2R0:  GenerateInstruction_B2R0 (LabelMap, Yielded, Inst, &FileSize, Output); break;
               case FMT_B2R0e: GenerateInstruction_B2R0e(LabelMap, Yielded, Inst, &FileSize, Output); break;
               case FMT_B2R1f: GenerateInstruction_B2R1f(                   Inst, &FileSize, Output); break;
               case FMT_B2R2:  GenerateInstruction_B2R2 (                   Inst, &FileSize, Output); break;
               
               default: Assert(FALSE, "Unknown instruction format!");
            }
         } break;
         
         case ASTType_Register:
            Assert(FALSE, "Register not attatched to an instruction!");
            break;
         
         case ASTType_Immediate:
            Assert(FALSE, "Immediate not attatched to an instruction!");
            break;
         
         case ASTType_Label: {
            HashMap_Add(LabelMap, &Node->Token->Str, &FileSize);
         } break;
         
         default: Assert(FALSE, "Unknown AST type!");
      }
   }
   
   u64 YieldedCursor = 0;
   while(YieldedCursor < Yielded->Size) {
      ast_node *Node = *(ast_node**)((u08*)Yielded->Data + YieldedCursor);
      u64 OldSize = Yielded->Size;
      YieldedCursor += sizeof(ast_node*);
      
      switch(Node->Type) {
         case ASTType_Instruction: {
            ast_instruction *Inst = (vptr)Node;
            instruction InstData = Inst->Instruction;
            
            u64 FileLoc = *(u64*)((u08*)Yielded->Data + YieldedCursor);
            YieldedCursor += sizeof(u64);
            
            switch(InstData.Fmt) {
               case FMT_B1R0:  GenerateInstruction_B1R0 (LabelMap, Yielded, Inst, &FileLoc, Output); break;
               case FMT_B1R0e: GenerateInstruction_B1R0e(LabelMap, Yielded, Inst, &FileLoc, Output); break;
               case FMT_B2R0:  GenerateInstruction_B2R0 (LabelMap, Yielded, Inst, &FileLoc, Output); break;
               case FMT_B2R0e: GenerateInstruction_B2R0e(LabelMap, Yielded, Inst, &FileLoc, Output); break;
               
               default: Assert(FALSE, "Yielded on a non-yieldable instruction format!");
            }
         } break;
         
         default: Assert(FALSE, "Unknown or unsupported yield type!");
      }
      
      if(Yielded->Size != OldSize) {
         Assembler_Log(NULL, Node->Token, LogType_Syntax|LogLevel_Fatal, CFStringL("Label operand for '%s' not found.", Node->Token->Str));
      }
   }
   
   Heap_Free(Yielded);
   
   Heap_Resize(Output, FileSize);
   return Output;
}

#endif