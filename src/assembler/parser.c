/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
**                                                                         **
**  Author: Aria Seiler                                                    **
**                                                                         **
**  This program is in the public domain. There is no implied warranty,    **
**  so use it at your own risk.                                            **
**                                                                         **
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef INCLUDE_HEADER

#define GET_AST(Type, Handle) ((Type*)Handle->Data)

typedef enum ast_node_type {
   ASTType_Invalid,
   ASTType_Root,
   ASTType_Section,
   ASTType_Instruction,
   ASTType_Register,
   ASTType_Immediate,
   ASTType_Label,
   ASTType_LabelReference,
   ASTType_Operation,
} ast_node_type;

typedef enum ast_operation_type {
   ASTOp_Identity,
   ASTOp_Subtract,
} ast_operation_type;

typedef struct ast_node {
   ast_node_type Type;
} ast_node;

typedef struct ast_root {
   ast_node Node;
   
   HEAP(heap_handle**) Children;
} ast_root;

typedef struct ast_section {
   ast_node Node;
   
   HEAP(heap_handle**) Children;
   
   string Name;
} ast_section;

typedef struct ast_instruction {
   ast_node Node;
   
   HEAP(heap_handle*) Children;
   
   string Name;
   instruction Instruction;
} ast_instruction;

typedef struct ast_register {
   ast_node Node;
   
   u08 Value;
} ast_register;

typedef struct ast_immediate {
   ast_node Node;
   
   s16 Value;
} ast_immediate;

typedef struct ast_label {
   ast_node Node;
   
   string Name;
} ast_label;

typedef struct ast_label_reference {
   ast_node Node;
   
   string Name;
} ast_label_reference;

typedef struct ast_operation {
   ast_node Node;
   
   ast_operation_type Type;
   b08 IsUnary;
} ast_operation;

#endif



#ifdef INCLUDE_SOURCE

internal u32
SkipComments(heap_handle *Tokens, u32 I)
{
   u32 TokenCount = Tokens->Size/sizeof(token);
   while(I < TokenCount) {
      token NextToken = ((token*)Tokens->Data)[I];
      if(NextToken.Type != TokenType_Comment) break;
      I++;
   }
   return I;
}

internal b08
CharIsEqualCI(c08 A, c08 B)
{
   if(B <= 'Z') return (A == B) || (A == B + ('a'-'A'));
   return (A == B) || (A == B - ('a'-'A'));
}

internal u08
ParseRegister(string Name)
{
   c08 *C = Name.Text;
   
   if(CharIsEqualCI(*C, 'S') || CharIsEqualCI(*C, 'A')) {
      u08 RegIndex = 0;
      for(u32 I = Name.Length-1; I > 0; I--)
         RegIndex = RegIndex*10 + C[I] - '0';
      
      if(CharIsEqualCI(*C, 'S')) {
         Assert(RegIndex <= 6);
         if(RegIndex >= 3) RegIndex += 8-3;
      } else {
         Assert(RegIndex <= 6);
         if(RegIndex >= 4) RegIndex += 12-4;
         else              RegIndex += 4;
      }
      
      return RegIndex;
   } else if(Name.Length == 3 && CharIsEqualCI(*C, 'Z') && CharIsEqualCI(*(C+1), 'R') && CharIsEqualCI(*(C+2), 'O')) {
      return 15;
   } else if(Name.Length == 2 && CharIsEqualCI(*C, 'R') && CharIsEqualCI(*(C+1), 'A')) {
      return 3;
   } else {
      Assert(FALSE, "Syntax error: Invalid register name");
   }
   
   return 16;
}

internal u08
ParseRadix(string Str)
{
   Assert(Str.Text && Str.Length);
   
   u08 Total = 0, PrevTotal = 0;
   for(u32 I = 0; I < Str.Length; I++) {
      if(Str.Text[I] == '_') continue;
      Total = (Total*10) + Str.Text[I]-'0';
      
      Assert(Total >= PrevTotal, "Overflow error: Radix is too large!");
      PrevTotal = Total;
   }
   
   Assert(Total >= 2 && Total <= 64, "Numerical error: Radix must be between 2 and 64, inclusive");
   return Total;
}

internal u64
ParseImmediate(string Str, u08 Radix)
{
   Assert(Str.Text && Str.Length && Radix >= 2 && Radix <= 64);
   
   u64 Total = 0, PrevTotal = 0;
   for(u32 I = 0; I < Str.Length; I++) {
      c08 C = Str.Text[I];
      
      if(C == '_') continue;
      Total *= Radix;
      
      if('0' <= C && C <= '9') {
         Assert(C-'0' < Radix);
         Total += C - '0';
      } else if('A' <= C && C <= 'Z') {
         Assert(C-'A'+10 <= Radix);
         Total += C - 'A' + 10;
      } else if('a' <= C && C <= 'z') {
         if(Radix < C-'a'+36) Total += C - 'a' + 10;
         else                 Total += C - 'a' + 36;
      } else if(C == '$') {
         Assert(Radix >= 63);
         Total += 62;
      } else if(C == '@') {
         Assert(Radix == 64);
         Total += 63;
      }
      
      Assert(Total >= PrevTotal, "Overflow error: The immediate is too large!");
      PrevTotal = Total;
   }
   
   return Total;
}

internal heap_handle *
ParseTokens(hashmap *InstMap, heap_handle *Tokens)
{
   heap_handle *Root = Heap_Allocate(_Heap, sizeof(ast_section));
   heap_handle *RootChildren = Heap_Allocate(_Heap, 0);
   ((ast_root*)Root->Data)->Node.Type = ASTType_Root;
   ((ast_root*)Root->Data)->Children = RootChildren;
   u32 RootChildrenCount = 0;
   
   u32 TokenCount = Tokens->Size / sizeof(token);
   for(u32 I = 0; I < TokenCount; I++) {
      token Token = ((token*)Tokens->Data)[I];
      
      switch(Token.Type) {
         case TokenType_Invalid:
            Assert(FALSE, "Invalid token!");
            break;
         
         
         case TokenType_LineSeparator:
         case TokenType_Comment:
            break;
         
         //TODO: Make these more robust
         case TokenType_LabelSuffix:
            Assert(FALSE, "Syntax error: Label suffix should not appear without a name!");
            break;
         
         case TokenType_MacroPrefix:
            Assert(FALSE, "Macros not implemented!");
            break;
         
         case TokenType_ListSeparator:
            Assert(FALSE, "Syntax error: Argument separators should only appear within an instruction or array!");
            break;
         
         case TokenType_ArrayStart:
         case TokenType_ArrayEnd:
            Assert(FALSE, "Arrays not implemented!");
            break;
         
         case TokenType_Operation:
            Assert(FALSE, "Syntax error: Operations must apply to numbers (or something)");
            break;
         
         case TokenType_Identifier: {
            u32 J = SkipComments(Tokens, I+1);
            
            token NextToken;
            if(J < TokenCount) {
               NextToken = ((token*)Tokens->Data)[J];
               
               if(NextToken.Type == TokenType_LabelSuffix) {
                  heap_handle *Node = Heap_Allocate(_Heap, sizeof(ast_label));
                  ((ast_label*)Node->Data)->Node.Type = ASTType_Label;
                  ((ast_label*)Node->Data)->Name = Token.Str;
                  
                  Heap_Resize(RootChildren, RootChildrenCount * sizeof(heap_handle*));
                  ((heap_handle**)RootChildren->Data)[RootChildrenCount] = Node;
                  RootChildrenCount++;
                  
                  I = J;
                  break;
               }
            }
            
            instruction *InstPtr = HashMap_Get(InstMap, &Token.Str);
            Assert(InstPtr);
            instruction Inst = *InstPtr;
            
            heap_handle *Node = Heap_Allocate(_Heap, sizeof(ast_instruction));
            ((ast_instruction*)Node->Data)->Node.Type = ASTType_Instruction;
            ((ast_instruction*)Node->Data)->Instruction = Inst;
            ((ast_instruction*)Node->Data)->Name = Token.Str;
            
            Heap_Resize(RootChildren, (RootChildrenCount+1) * sizeof(heap_handle*));
            ((heap_handle**)RootChildren->Data)[RootChildrenCount] = Node;
            RootChildrenCount++;
            
            switch(Inst.Fmt) {
               case FMT_B1R0:
               case FMT_B2R0:
               case FMT_B1R0e:
               case FMT_B2R0e: {
                  Assert(J < TokenCount);
                  
                  heap_handle *ImmNode;
                  if(NextToken.Type == TokenType_Identifier) {
                     ImmNode = Heap_Allocate(_Heap, sizeof(ast_label_reference));
                     ((ast_label_reference*)ImmNode->Data)->Node.Type = ASTType_LabelReference;
                     ((ast_label_reference*)ImmNode->Data)->Name = NextToken.Str;
                  } else {
                     Assert(NextToken.Type == TokenType_Number);
                     
                     u08 Radix = 10;
                     if(J+1 < TokenCount) {
                        token RadixToken = ((token*)Tokens->Data)[J+1];
                        if(RadixToken.Type == TokenType_Radix) {
                           Radix = ParseRadix(RadixToken.Str);
                           J++;
                        }
                     }
                     
                     ImmNode = Heap_Allocate(_Heap, sizeof(ast_immediate));
                     ((ast_immediate*)ImmNode->Data)->Node.Type = ASTType_Immediate;
                     ((ast_immediate*)ImmNode->Data)->Value = ParseImmediate(NextToken.Str, Radix);
                  }
                  
                  heap_handle *InstChildren = Heap_Allocate(_Heap, sizeof(heap_handle*));
                  ((heap_handle**)InstChildren->Data)[0] = ImmNode;
                  
                  ((ast_instruction*)Node->Data)->Children = InstChildren;
                  
                  I = J;
               } break;
               
               case FMT_B1R1:
               case FMT_B2R1f: {
                  Assert(J < TokenCount);
                  Assert(NextToken.Type == TokenType_Identifier);
                  
                  heap_handle *RegNode = Heap_Allocate(_Heap, sizeof(ast_register));
                  ((ast_register*)RegNode->Data)->Node.Type = ASTType_Register;
                  ((ast_register*)RegNode->Data)->Value = ParseRegister(NextToken.Str);
                  
                  heap_handle *InstChildren = Heap_Allocate(_Heap, sizeof(heap_handle*));
                  ((heap_handle**)InstChildren->Data)[0] = RegNode;
                  
                  ((ast_instruction*)Node->Data)->Children = InstChildren;
                  
                  I = J;
               } break;
               
               case FMT_B2R1: {
                  Assert(J+1 < TokenCount);
                  J = SkipComments(Tokens, J+1);
                  Assert(J < TokenCount);
                  token NextToken1 = ((token*)Tokens->Data)[J];
                  
                  Assert(NextToken.Type == TokenType_Identifier);
                  heap_handle *RegNode = Heap_Allocate(_Heap, sizeof(ast_register));
                  ((ast_register*)RegNode->Data)->Node.Type = ASTType_Register;
                  ((ast_register*)RegNode->Data)->Value = ParseRegister(NextToken.Str);
                  
                  heap_handle *ImmNode;
                  if(NextToken.Type == TokenType_Identifier) {
                     ImmNode = Heap_Allocate(_Heap, sizeof(ast_label_reference));
                     ((ast_label_reference*)ImmNode->Data)->Node.Type = ASTType_LabelReference;
                     ((ast_label_reference*)ImmNode->Data)->Name = NextToken1.Str;
                  } else {
                     Assert(NextToken.Type == TokenType_Number);
                     
                     u08 Radix = 10;
                     if(J+1 < TokenCount) {
                        token RadixToken = ((token*)Tokens->Data)[J];
                        if(RadixToken.Type == TokenType_Radix) {
                           Radix = ParseRadix(RadixToken.Str);
                           J++;
                        }
                     }
                     
                     ImmNode = Heap_Allocate(_Heap, sizeof(ast_immediate));
                     ((ast_immediate*)ImmNode->Data)->Node.Type = ASTType_Immediate;
                     ((ast_immediate*)ImmNode->Data)->Value = ParseImmediate(NextToken1.Str, Radix);
                  }
                  
                  heap_handle *InstChildren = Heap_Allocate(_Heap, 2*sizeof(heap_handle*));
                  ((heap_handle**)InstChildren->Data)[0] = RegNode;
                  ((heap_handle**)InstChildren->Data)[1] = ImmNode;
                  
                  ((ast_instruction*)Node->Data)->Children = InstChildren;
                  
                  I = J;
               } break;
               
               case FMT_B2R2: {
                  Assert(NextToken.Type == TokenType_Identifier);
                  
                  J = SkipComments(Tokens, J+1);
                  Assert(J < TokenCount);
                  token SepTok = ((token*)Tokens->Data)[J];
                  Assert(SepTok.Type == TokenType_ListSeparator);
                  
                  J = SkipComments(Tokens, J+1);
                  Assert(J < TokenCount);
                  token NextToken1 = ((token*)Tokens->Data)[J];
                  Assert(NextToken1.Type == TokenType_Identifier);
                  
                  heap_handle *RegNode0 = Heap_Allocate(_Heap, sizeof(ast_register));
                  ((ast_register*)RegNode0->Data)->Node.Type = ASTType_Register;
                  ((ast_register*)RegNode0->Data)->Value = ParseRegister(NextToken.Str);
                  
                  heap_handle *RegNode1 = Heap_Allocate(_Heap, sizeof(ast_register));
                  ((ast_register*)RegNode1->Data)->Node.Type = ASTType_Register;
                  ((ast_register*)RegNode1->Data)->Value = ParseRegister(NextToken1.Str);
                  
                  heap_handle *InstChildren = Heap_Allocate(_Heap, 2*sizeof(heap_handle*));
                  ((heap_handle**)InstChildren->Data)[0] = RegNode0;
                  ((heap_handle**)InstChildren->Data)[1] = RegNode1;
                  
                  ((ast_instruction*)Node->Data)->Children = InstChildren;
                  
                  I = J;
               } break;
            }
         } break;
         
         default: Assert(FALSE, "Unknown token type!");
      }
   }
   
   return Root;
}

#endif