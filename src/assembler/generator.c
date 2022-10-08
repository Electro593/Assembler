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
GetImmediate(hashmap *LabelMap, heap_handle *Yielded,
             heap_handle *Inst, heap_handle *Imm, u16 PC, u16 InstSize,
             s16 *ValueOut)
{
   ast_node_type Type = ((ast_node*)Imm->Data)->Type;
   
   if(Type == ASTType_Immediate) {
      *ValueOut = ((ast_immediate*)Imm->Data)->Value;
   } else {
      Assert(Type == ASTType_LabelReference);
      string Name = ((ast_label_reference*)Imm->Data)->Name;
      
      u16 *Label = HashMap_Get(LabelMap, &Name);
      if(Label) {
         *ValueOut = PC + InstSize - *Label;
      } else {
         u64 YieldedSize = Yielded->Size;
         Heap_Resize(Yielded, YieldedSize + sizeof(heap_handle*) + sizeof(u16));
         heap_handle **YieldedInst = (vptr)((u08*)Yielded->Data + YieldedSize);
         u16 *YieldedPC = (vptr)((u08*)Yielded->Data + YieldedSize + sizeof(heap_handle*));
         *YieldedInst = Inst;
         *YieldedPC   = (u16)PC;
         return FALSE;
      }
   }
   
   return TRUE;
}

internal void
GenerateInstruction_B1R0(hashmap *LabelMap, heap_handle *Yielded, heap_handle *Node, instruction Inst, heap_handle *Children, u64 *PC, heap_handle *Output)
{
   Assert(Inst.Size == 0 && Inst.Fmt == FMT_B1R0);
   Assert(!Children->Free && Children->Size == sizeof(heap_handle*));
   
   heap_handle *Operand = ((heap_handle**)Children->Data)[0];
   
   s16 Value;
   b08 Success = GetImmediate(LabelMap, Yielded, Node, Operand, *PC, 1, &Value);
   
   if(Success) {
      Assert(-4 <= Value && Value <= 3);
      ((u08*)Output->Data)[*PC+0] = (Value << 5) | Inst.Opcode;
   }
   
   *PC += 1;
}

internal void
GenerateInstruction_B1R0e(hashmap *LabelMap, heap_handle *Yielded, heap_handle *Node, instruction Inst, heap_handle *Children, u64 *PC, heap_handle *Output)
{
   Assert(Inst.Size == 0 && Inst.Fmt == FMT_B1R0e);
   Assert(!Children->Free && Children->Size == sizeof(heap_handle*));
   
   heap_handle *Operand = ((heap_handle**)Children->Data)[0];
   
   s16 Value;
   b08 Success = GetImmediate(LabelMap, Yielded, Node, Operand, *PC, 1, &Value);
   
   if(Success) {
      Assert(-8 <= Value && Value <= 7);
      ((u08*)Output->Data)[*PC+0] = (Value << 4) | (Inst.Opcode & 0xF);
   }
   
   *PC += 1;
}

internal void
GenerateInstruction_B1R1(hashmap *LabelMap, heap_handle *Yielded, heap_handle *Node, instruction Inst, heap_handle *Children, u64 *PC, heap_handle *Output)
{
   Assert(Inst.Size == 0 && Inst.Fmt == FMT_B1R1);
   Assert(!Children->Free && Children->Size == sizeof(heap_handle*));
   
   heap_handle *Operand = ((heap_handle**)Children->Data)[0];
   Assert(((ast_node*)Operand->Data)->Type == ASTType_Register);
   
   u08 Value = ((ast_register*)Operand->Data)->Value;
   Assert(Value < 8);
   
   ((u08*)Output->Data)[*PC+0] = (Value << 5) | Inst.Opcode;
   *PC += 1;
}

internal void
GenerateInstruction_B2R0(hashmap *LabelMap, heap_handle *Yielded, heap_handle *Node, instruction Inst, heap_handle *Children, u64 *PC, heap_handle *Output)
{
   Assert(Inst.Size == 1 && Inst.Fmt == FMT_B2R0);
   Assert(!Children->Free && Children->Size == sizeof(heap_handle*));
   
   heap_handle *Operand = ((heap_handle**)Children->Data)[0];
   
   s16 Value;
   b08 Success = GetImmediate(LabelMap, Yielded, Node, Operand, *PC, 2, &Value);
   
   if(Success) {
      Assert(-128 <= Value && Value <= 127);
      ((u08*)Output->Data)[*PC+0] = ((Value & 0x7) << 5) | ((Inst.Opcode & 0x3) << 3) | 0b111;
      ((u08*)Output->Data)[*PC+1] = ((Inst.Opcode & 0x1C) << 3) | (Value >> 3);
   }
   
   *PC += 2;
}

internal void
GenerateInstruction_B2R0e(hashmap *LabelMap, heap_handle *Yielded, heap_handle *Node, instruction Inst, heap_handle *Children, u64 *PC, heap_handle *Output)
{
   Assert(Inst.Size == 1 && Inst.Fmt == FMT_B2R0e);
   Assert(!Children->Free && Children->Size == sizeof(heap_handle*));
   
   heap_handle *Operand = ((heap_handle**)Children->Data)[0];
   
   s16 Value;
   b08 Success = GetImmediate(LabelMap, Yielded, Node, Operand, *PC, 2, &Value);
   
   if(Success) {
      Assert(-2048 <= Value && Value <= 2047);
      ((u08*)Output->Data)[*PC+0] = ((Value & 0xF) << 4) | (Inst.Opcode & 0xF);
      ((u08*)Output->Data)[*PC+1] = Value >> 4;
   }
   
   *PC += 2;
}

internal void
GenerateInstruction_B2R1(hashmap *LabelMap, heap_handle *Yielded, heap_handle *Node, instruction Inst, heap_handle *Children, u64 *PC, heap_handle *Output)
{
   Assert(Inst.Size == 1 && Inst.Fmt == FMT_B2R1);
   Assert(!Children->Free && Children->Size == 2*sizeof(heap_handle*));
   
   heap_handle *Operand0 = ((heap_handle**)Children->Data)[0];
   Assert(((ast_node*)Operand0->Data)->Type == ASTType_Register);
   u08 Reg = ((ast_register*)Operand0->Data)->Value;
   Assert(Reg < 16);
   
   s16 Imm;
   heap_handle *Operand1 = ((heap_handle**)Children->Data)[1];
   b08 Success = GetImmediate(LabelMap, Yielded, Node, Operand1, *PC, 2, &Imm);
   
   if(Success) {
      Assert(-8 <= Imm && Imm <= 7);
      ((u08*)Output->Data)[*PC+0] = ((Imm & 0x7) << 5) | ((Inst.Opcode & 0x3) << 3) | 0b111;
      ((u08*)Output->Data)[*PC+1] = ((Inst.Opcode & 0x1C) << 3) | (Reg << 1) | (Imm >> 3);
   }
   
   *PC += 2;
}

internal void
GenerateInstruction_B2R1f(hashmap *LabelMap, heap_handle *Yielded, heap_handle *Node, instruction Inst, heap_handle *Children, u64 *PC, heap_handle *Output)
{
   Assert(Inst.Size == 1 && Inst.Fmt == FMT_B2R1f);
   Assert(!Children->Free && Children->Size == sizeof(heap_handle*));
   
   heap_handle *Operand = ((heap_handle**)Children->Data)[0];
   Assert(((ast_node*)Operand->Data)->Type == ASTType_Register);
   
   u08 Value = ((ast_register*)Operand->Data)->Value;
   Assert(Value < 16);
   
   ((u08*)Output->Data)[*PC+0] = ((Inst.Fn & 0x7) << 5) | ((Inst.Opcode & 0x3) << 3) | 0b111;
   ((u08*)Output->Data)[*PC+1] = ((Inst.Opcode & 0x1C) << 3) | (Value << 1) | (Inst.Fn >> 3);
   *PC += 2;
}

internal void
GenerateInstruction_B2R2(hashmap *LabelMap, heap_handle *Yielded, heap_handle *Node, instruction Inst, heap_handle *Children, u64 *PC, heap_handle *Output)
{
   Assert(Inst.Size == 1 && Inst.Fmt == FMT_B2R2);
   Assert(!Children->Free && Children->Size == 2*sizeof(heap_handle*));
   
   heap_handle *Operand0 = ((heap_handle**)Children->Data)[0];
   Assert(((ast_node*)Operand0->Data)->Type == ASTType_Register);
   u08 Reg0 = ((ast_register*)Operand0->Data)->Value;
   Assert(Reg0 < 16);
   
   heap_handle *Operand1 = ((heap_handle**)Children->Data)[1];
   Assert(((ast_node*)Operand1->Data)->Type == ASTType_Register);
   u08 Reg1 = ((ast_register*)Operand1->Data)->Value;
   Assert(Reg1 < 16);
   
   ((u08*)Output->Data)[*PC+0] = ((Reg1 & 0x7) << 5) | ((Inst.Opcode & 0x3) << 3) | 0b111;
   ((u08*)Output->Data)[*PC+1] = ((Inst.Opcode & 0x1C) << 3) | (Reg0 << 1) | (Reg1 >> 3);
   *PC += 2;
}

internal heap_handle *
GenerateASM(heap_handle *AST, hashmap *LabelMap)
{
   u64 FileInc = 512;
   u64 FileSize = 0;
   heap_handle *Output = Heap_Allocate(_Heap, 0);
   
   heap_handle *Yielded = Heap_Allocate(_Heap, 0);
   
   vptr StackStart = Stack_GetCursor();
   
   Stack_Push();
   heap_handle **Entry = Stack_Allocate(sizeof(heap_handle*));
   *Entry = AST;
   
   while(Stack_GetCursor() > StackStart) {
      heap_handle *Node = *(heap_handle**)Stack_GetEntry();
      Stack_Pop();
      
      switch(((ast_node*)Node->Data)->Type) {
         case ASTType_Invalid:
            Assert(FALSE, "Generation error: Invalid AST type!");
            break;
         
         case ASTType_Root: {
            heap_handle *Children = ((ast_root*)Node->Data)->Children;
            
            for(s32 I = (Children->Size/sizeof(heap_handle*))-1; I >= 0; I--) {
               heap_handle *Child = ((heap_handle**)Children->Data)[I];
               Stack_Push();
               Entry = Stack_Allocate(sizeof(heap_handle*));
               *Entry = Child;
            }
         } break;
         
         case ASTType_Section:
            Assert(FALSE, "Sections not implemented!");
            break;
         
         case ASTType_Instruction: {
            heap_handle *Children = ((ast_instruction*)Node->Data)->Children;
            instruction Inst = ((ast_instruction*)Node->Data)->Instruction;
            
            Assert(FileInc >= Inst.Size+1);
            if(FileSize+Inst.Size+1 > Output->Size)
               Heap_Resize(Output, FileSize+FileInc);
            
            switch(Inst.Fmt) {
               case FMT_B1R0:
                  GenerateInstruction_B1R0(LabelMap, Yielded, Node, Inst, Children, &FileSize, Output);
                  break;
               
               case FMT_B1R0e:
                  GenerateInstruction_B1R0e(LabelMap, Yielded, Node, Inst, Children, &FileSize, Output);
                  break;
               
               case FMT_B1R1:
                  GenerateInstruction_B1R1(LabelMap, Yielded, Node, Inst, Children, &FileSize, Output);
                  break;
               
               case FMT_B2R0:
                  GenerateInstruction_B2R0(LabelMap, Yielded, Node, Inst, Children, &FileSize, Output);
                  break;
               
               case FMT_B2R0e:
                  GenerateInstruction_B2R0e(LabelMap, Yielded, Node, Inst, Children, &FileSize, Output);
                  break;
               
               case FMT_B2R1:
                  GenerateInstruction_B2R1(LabelMap, Yielded, Node, Inst, Children, &FileSize, Output);
                  break;
               
               case FMT_B2R1f:
                  GenerateInstruction_B2R1f(LabelMap, Yielded, Node, Inst, Children, &FileSize, Output);
                  break;
               
               case FMT_B2R2:
                  GenerateInstruction_B2R2(LabelMap, Yielded, Node, Inst, Children, &FileSize, Output);
                  break;
               
               default: Assert(FALSE, "Unknown instruction format!");
            }
         } break;
         
         case ASTType_Register:
            Assert(FALSE, "Register not attatched to an instruction!");
            break;
         
         case ASTType_Immediate:
            Assert(FALSE, "Immediate not attatched to an instruction!");
            break;
         
         case ASTType_LabelReference:
            Assert(FALSE, "Label reference not attatched to an instruction!");
            break;
         
         case ASTType_Label: {
            string Name = ((ast_label*)Node->Data)->Name;
            
            HashMap_Add(LabelMap, &Name, &FileSize);
         } break;
         
         default: Assert(FALSE, "Unknown AST type!");
      }
   }
   
   u64 Cursor = 0;
   while(Cursor < Yielded->Size) {
      heap_handle *Node = *(heap_handle**)((u08*)Yielded->Data + Cursor);
      Cursor += sizeof(heap_handle*);
      ast_node_type NodeType = ((ast_node*)Node->Data)->Type;
      
      switch(NodeType) {
         case ASTType_Instruction: {
            heap_handle *Children = ((ast_instruction*)Node->Data)->Children;
            instruction Inst = ((ast_instruction*)Node->Data)->Instruction;
            u64 PC = *(u16*)((u08*)Yielded->Data + Cursor);
            Cursor += sizeof(u16);
            
            switch(Inst.Fmt) {
               case FMT_B1R0:
                  GenerateInstruction_B1R0(LabelMap, Yielded, Node, Inst, Children, &PC, Output);
                  break;
               
               case FMT_B2R0:
                  GenerateInstruction_B1R0(LabelMap, Yielded, Node, Inst, Children, &PC, Output);
                  break;
                  
               case FMT_B2R1:
                  GenerateInstruction_B1R0(LabelMap, Yielded, Node, Inst, Children, &PC, Output);
                  break;
                  
               case FMT_B1R0e:
                  GenerateInstruction_B1R0(LabelMap, Yielded, Node, Inst, Children, &PC, Output);
                  break;
                  
               case FMT_B2R0e:
                  GenerateInstruction_B1R0(LabelMap, Yielded, Node, Inst, Children, &PC, Output);
                  break;
               
               default:
                  Assert(FALSE, "Non-yieldable instruction format!");
            }
         } break;
         
         default:
            Assert(FALSE, "Unknown or unsupported yield type!");
      }
   }
   
   Heap_Free(Yielded);
   
   Heap_Resize(Output, FileSize);
   return Output;
}

#endif