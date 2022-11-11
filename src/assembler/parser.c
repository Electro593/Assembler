/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
**                                                                         **
**  Author: Aria Seiler                                                    **
**                                                                         **
**  This program is in the public domain. There is no implied warranty,    **
**  so use it at your own risk.                                            **
**                                                                         **
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef INCLUDE_HEADER

typedef enum ast_node_type {
   ASTType_Invalid,
   ASTType_Root,
   ASTType_Section,
   ASTType_Statement,
   ASTType_Instruction,
   ASTType_Operation,
   ASTType_Identifier,
   ASTType_Register,
   ASTType_Immediate,
   ASTType_Label,
} ast_node_type;

typedef struct ast_node {
   ast_node_type Type;
   
   s32 ChildCount;
   struct ast_node *Prev;
   struct ast_node *Next;
   struct ast_node *FirstChild;
   
   token *Token;
} ast_node;

ast_node *NullNode;

typedef struct ast_root {
   ast_node Header;
} ast_root;

typedef struct ast_section {
   ast_node Header;
} ast_section;

typedef struct ast_statement {
   ast_node Header;
} ast_statement;

typedef struct ast_instruction {
   ast_node Header;
   
   instruction Instruction;
} ast_instruction;

typedef struct ast_array {
   ast_node Header;
} ast_array;

typedef struct ast_operation {
   ast_node Header;
   
   // 0 if prefix
   // 1 if infix binary or postfix unary
   // 2 if postfix binary, etc.
   int OperatorIndex;
} ast_operation;

typedef struct ast_identifier {
   ast_node Header;
} ast_identifier;

typedef struct ast_immediate {
   ast_node Header;
   
   u64 Value;
} ast_immediate;

typedef struct ast_label {
   ast_node Header;
} ast_label;

typedef struct parser {
   token *Tokens;
   u32 TokenCount;
   u32 Index;
   
   ast_root *AST;
   ast_node *Parent;
   token *DummySection;
   
   string FileName;
   hashmap *InstMap;
} parser;

typedef enum log_type {
   LogLevel_Debug = 0x00000000,
   LogLevel_Info  = 0x01000000,
   LogLevel_Note  = 0x02000000,
   LogLevel_Warn  = 0x03000000,
   LogLevel_Error = 0x04000000,
   LogLevel_Fatal = 0x05000000,
   LogLevel_Mask  = 0xFF000000,
   
   LogType_Syntax    = 0x00000000,
   LogType_Assembler = 0x00000001,
   LogType_Mask      = 0x00FFFFFF,
   
   SyntaxErrorF    = LogLevel_Fatal | LogType_Syntax,
   AssemblerErrorF = LogLevel_Fatal | LogType_Assembler,
} log_type;

#endif



#ifdef INCLUDE_SOURCE

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

internal void
Assembler_Log(parser *Parser, token *Token, log_type Type, string Message)
{
   string LogType;
   switch(Type & LogType_Mask) {
      case LogType_Syntax:    LogType = CLStringL("Syntax");    break;
      case LogType_Assembler: LogType = CLStringL("Assembler"); break;
   }
   
   string LogHeader;
   switch(Type & LogLevel_Mask) {
      case LogLevel_Debug: LogHeader = CFStringL(      "%s debug info", LogType); break;
      case LogLevel_Info:  LogHeader = CFStringL(      "%s info",       LogType); break;
      case LogLevel_Note:  LogHeader = CFStringL(      "%s note",       LogType); break;
      case LogLevel_Warn:  LogHeader = CFStringL(      "%s warning",    LogType); break;
      case LogLevel_Error: LogHeader = CFStringL(      "%s error",      LogType); break;
      case LogLevel_Fatal: LogHeader = CFStringL("Fatal %s error",      LogType); break;
   }
   
   u32 IsFatal = (Type & LogLevel_Mask) >= LogLevel_Fatal ? Type : 0;
   
   if(Parser && Token) Platform_WriteError(FString(CLStringL("%s in %s on line %d, column %d: %s"), LogHeader, Parser->FileName, Token->LineNumber, Token->ColumnNumber, Message), IsFatal);
   else if(Parser)     Platform_WriteError(FString(CLStringL("%s in %s: %s"), LogHeader, Parser->FileName, Message), IsFatal);
   else if(Token)      Platform_WriteError(FString(CLStringL("%s on line %d, column %d: %s"), LogHeader, Token->LineNumber, Token->ColumnNumber, Message), IsFatal);
   else                Platform_WriteError(FString(CLStringL("%s: %s"), LogHeader, Message), IsFatal);
}

internal u64
ParseNumber(parser *Parser, token *Token, u08 Radix)
{
   Assert(Token && Parser);
   
   string Str = Token->Str;
   
   AAssert(Str.Text && Str.Length, "Cannot parse an empty number string!");
   AAssert(Radix >= 2 && Radix <= 64, "Radix must be between 2 and 64!");
   
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
      
      if(Total < PrevTotal) Assembler_Log(Parser, Token, SyntaxErrorF, CFStringL("We can't parse the number %s because it doesn't fit within 64 bits.\n", Token->Str));
      PrevTotal = Total;
   }
   
   return Total;
}

internal ast_node *
RemoveChild(ast_node *Parent, s32 Index)
{
   AAssert(Parent && Index < Parent->ChildCount && -(Index+1) < Parent->ChildCount);
   
   ast_node *Child = Parent->FirstChild;
   
   if(Parent->ChildCount == 1) {
      Parent->FirstChild = NullNode;
   } else {
      for(s32 I = 0; I <  Index; I++) Child = Child->Next;
      for(s32 I = 0; I < -Index; I++) Child = Child->Prev;
      
      Child->Next->Prev = Child->Prev;
      Child->Prev->Next = Child->Next;
      
      if(Index == 0) Parent->FirstChild = Child->Next;
   }
   
   Child->Next = NullNode;
   Child->Prev = NullNode;
   
   Parent->ChildCount--;
   return Child;
}

internal void
AddChild(ast_node *Parent, ast_node *Child, s32 Index)
{
   AAssert(Parent && Child && Index <= Parent->ChildCount && -(Index+1) <= Parent->ChildCount);
   
   if(Parent->ChildCount == 0) {
      Child->Next = Child;
      Child->Prev = Child;
      Parent->FirstChild = Child;
   } else {
      ast_node *Next = Parent->FirstChild;
      for(s32 I = 0; I <   Index;    I++) Next = Next->Next;
      for(s32 I = 0; I < -(Index+1); I++) Next = Next->Prev;
      
      Child->Next = Next;
      Child->Prev = Next->Prev;
      Child->Next->Prev = Child;
      Child->Prev->Next = Child;
      
      if(Index == 0) Parent->FirstChild = Child;
   }
   
   Parent->ChildCount++;
}

internal ast_node *
GetChild(ast_node *Parent, s32 Index)
{
   AAssert(Parent && Index < Parent->ChildCount && -(Index+1) < Parent->ChildCount);
   
   ast_node *Child = Parent->FirstChild;
   for(s32 I = 0; I <  Index; I++) Child = Child->Next;
   for(s32 I = 0; I < -Index; I++) Child = Child->Prev;
   return Child;
}

internal s32
GetPrecedenceFromNode(ast_operation *Node)
{
   u32 Types[] = {
      NullaryPrecedences[Node->Header.Token->Type & TokenType_Mask] & 0x80000000,
      UnaryPrefixPrecedences[Node->Header.Token->Type & TokenType_Mask] & 0x80000000,
      UnaryPostfixPrecedences[Node->Header.Token->Type & TokenType_Mask] & 0x80000000,
      BinaryInfixPrecedences[Node->Header.Token->Type & TokenType_Mask] & 0x80000000,
      TernaryPrecedences[Node->Header.Token->Type & TokenType_Mask] & 0x80000000,
   };
   
   u32 Precedences[] = {
      (s32)(NullaryPrecedences[Node->Header.Token->Type & TokenType_Mask] & 0x7FFFFFFF) << 1 >> 1,
      (s32)(UnaryPrefixPrecedences[Node->Header.Token->Type & TokenType_Mask] & 0x7FFFFFFF) << 1 >> 1,
      (s32)(UnaryPostfixPrecedences[Node->Header.Token->Type & TokenType_Mask] & 0x7FFFFFFF) << 1 >> 1,
      (s32)(BinaryInfixPrecedences[Node->Header.Token->Type & TokenType_Mask] & 0x7FFFFFFF) << 1 >> 1,
      (s32)(TernaryPrecedences[Node->Header.Token->Type & TokenType_Mask] & 0x7FFFFFFF) << 1 >> 1,
   };
   
   if(Node->Header.ChildCount == 0) return Precedences[0];
   if(Node->Header.ChildCount == 1) {
      if(Precedences[1] < Precedences[2]) return Precedences[1];
      else return Precedences[2];
   }
   if(Node->Header.ChildCount == 2) {
      ast_node *A = GetChild((ast_node*)Node, 0);
      ast_node *B = GetChild((ast_node*)Node, 1);
      if(A && B) return Precedences[3];
      else if(A) return Precedences[2];
      else if(B) return Precedences[1];
      else       return Precedences[0];
   }
   
   if(Node->Header.ChildCount == 3) return Precedences[4];
   
   return -1;
}

internal ast_node *
CreateASTNode(ast_node *Parent, token *Token, u32 Size, ast_node_type Type)
{
   ast_node *Node = Heap_AllocateA(_Heap, Size);
   Node->Type = Type;
   Node->ChildCount = 0;
   Node->FirstChild = NullNode;
   Node->Token = Token;
   
   if(Parent) AddChild(Parent, Node, -1);
   else {
      Node->Next = NullNode;
      Node->Prev = NullNode;
   }
   
   return Node;
}

internal ast_root *
CreateASTRoot(void)
{
   return (vptr)CreateASTNode(NULL, NULL, sizeof(ast_root), ASTType_Root);
}

internal ast_section *
CreateASTSection(ast_node *Parent, token *Token)
{
   ast_section *Node = (vptr)CreateASTNode(Parent, Token, sizeof(ast_section), ASTType_Section);
   return Node;
}

internal ast_statement *
CreateASTStatement(ast_node *Parent, token *Token)
{
   return (vptr)CreateASTNode(Parent, Token, sizeof(ast_statement), ASTType_Statement);
}

internal ast_label *
CreateASTLabel(ast_node *Parent, token *Token)
{
   ast_label *Node = (vptr)CreateASTNode(Parent, Token, sizeof(ast_label), ASTType_Label);
   return Node;
}

internal ast_instruction *
CreateASTInstruction(instruction Inst, ast_node *Parent, token *Token)
{
   ast_instruction *Node = (vptr)CreateASTNode(Parent, Token, sizeof(ast_instruction), ASTType_Instruction);
   Node->Instruction = Inst;
   return Node;
}

internal ast_identifier *
CreateASTIdentifier(ast_node *Parent, token *Token)
{
   ast_identifier *Node = (vptr)CreateASTNode(Parent, Token, sizeof(ast_identifier), ASTType_Identifier);
   return Node;
}

internal ast_immediate *
CreateASTImmediate(parser *Parser, token *Number, token *RadixToken)
{
   ast_immediate *Node = (vptr)CreateASTNode(Parser->Parent, Number, sizeof(ast_immediate), ASTType_Immediate);
   
   u64 Radix = (RadixToken) ? ParseNumber(Parser, RadixToken, 10) : 10;
   if(Radix < 2 || Radix > 64) Assembler_Log(Parser, RadixToken, SyntaxErrorF, CFStringL("Radixes can only be between 2 and 64, but this one isn't: %d\n", Radix));
   Node->Value = ParseNumber(Parser, Number, Radix);
   
   return Node;
}

internal ast_operation *
CreateASTOperation(int operatorIndex, ast_node *Parent, token *Token)
{
   ast_operation *Node = (vptr)CreateASTNode(Parent, Token, sizeof(ast_operation), ASTType_Operation);
   Node->OperatorIndex = operatorIndex;
   return Node;
}

internal token *
GetToken(parser *Parser, s32 Offset)
{
   AAssert(Parser, "Parser cannot be null!");
   AAssert(Parser->Tokens, "Tokens cannot be null!");
   AAssert(Parser->Index+Offset < Parser->TokenCount, "Retrieving out-of-bounds token!");
   return Parser->Tokens + Parser->Index+Offset;
}

internal string
GetStringUntil(parser *Parser, u32 LenMax, token_type Type)
{
   u32 I = 0;
   token *Token = GetToken(Parser, I);
   token *FirstToken = Token;
   c08 *Text = Token->Str.Text;
   c08 *EndText = Text;
   
   while(Parser->Index+I < Parser->TokenCount && (u64)EndText - (u64)Text < LenMax && (Token = GetToken(Parser, I++))->Type != Type)
      EndText = Token->Str.Text + Token->Str.Length;
   
   u64 Length = (u64)EndText - (u64)Text;
   return CLString(Text, Length);
}

internal token *
AcceptToken(parser *Parser, token_type Type)
{
   if(!Parser) return NULL;
   if(!Parser->Tokens) return NULL;
   if(Parser->Index >= Parser->TokenCount) return NULL;
   
   token *Token = Parser->Tokens + Parser->Index;
   if(Token->Type == Type) {
      Parser->Index += 1;
      return Token;
   }
   
   return NULL;
}

internal token *
AcceptTokenGroup(parser *Parser, token_type Group)
{
   AAssert((Group & TokenType_Mask) == 0, "AcceptTokenGroup was given a TokenType, not a group!");
   
   if(!Parser) return NULL;
   if(!Parser->Tokens) return NULL;
   if(Parser->Index >= Parser->TokenCount) return NULL;
   
   token *Token = Parser->Tokens + Parser->Index;
   if((Token->Type & TokenGroup_Mask) == Group) {
      Parser->Index += 1;
      return Token;
   }
   
   return NULL;
}

internal token *
AcceptExactIdentifier(parser *Parser, string Name)
{
   u32 Index = Parser->Index;
   token *Token;
   
   Token = AcceptToken(Parser, TokenType_Identifier);
   if(!Token) goto ret;
   
   if(String_Cmp(Token->Str, Name) == EQUAL) return Token;
   
   ret:
   Parser->Index = Index;
   return FALSE;
}

internal ast_identifier *
AcceptIdentifier(parser *Parser)
{
   token *Token;
   while(Token = AcceptToken(Parser, TokenType_LineSeparator));
   
   token *Ident = AcceptToken(Parser, TokenType_Identifier);
   if(!Ident) return NULL;
   
   return CreateASTIdentifier(Parser->Parent, Ident);
}

internal ast_label *
AcceptLabel(parser *Parser)
{
   token *Token;
   while(Token = AcceptToken(Parser, TokenType_LineSeparator));
   
   u32 Index = Parser->Index;
   
   token *Ident = AcceptToken(Parser, TokenType_Identifier);
   if(!Ident) goto ret;
   
   Token = AcceptToken(Parser, TokenType_LabelEnd);
   if(!Token) goto ret;
   
   return CreateASTLabel(Parser->Parent, Ident);
   
   ret:
   Parser->Index = Index;
   return NULL;
}

internal ast_immediate *
AcceptImmediate(parser *Parser)
{
   token *Token;
   while(Token = AcceptToken(Parser, TokenType_LineSeparator));
   
   u32 Index = Parser->Index;
   
   token *Number = AcceptToken(Parser, TokenType_Number);
   if(!Number) goto ret;
   
   token *Radix = AcceptToken(Parser, TokenType_Radix);
   
   return CreateASTImmediate(Parser, Number, Radix);
   
   ret:
   Parser->Index = Index;
   return NULL;
}

// internal ast_operation *
// AcceptOperator(parser *Parser)
// {
//    //TODO: Do this
   
//    return NULL;
// }

// internal ast_array *
// AcceptArray(parser *Parser)
// {
//    //TODO: Do this
   
//    return NULL;
// }

internal ast_node *
AcceptExpression(parser *Parser)
{
   u32 Index = Parser->Index;
   ast_node *Parent = Parser->Parent;
   
   token *Negative = AcceptToken(Parser, TokenType_Subtraction);
   ast_operation *Op;
   if(Negative) {
      Op = CreateASTOperation(0, Parser->Parent, Negative);
      Parser->Parent = (ast_node*)Op;
   }
   
   ast_node  *Value = (vptr)AcceptIdentifier(Parser);
   if(!Value) Value = (vptr)AcceptImmediate(Parser);
   if(!Value) goto ret;
   
   Parser->Parent = Parent;
   if(Negative)
      return (ast_node*)Op;
   else
      return Value;
   
   ret:
   if(Op) {
      RemoveChild(Parent, -1);
      Heap_FreeA(Op);
   }
   Parser->Parent = Parent;
   Parser->Index = Index;
   return NULL;
   
   // ast_node *Parent = Parser->Parent;
   // u32 Index = Parser->Index;
   // token *Token;
   
   // Token = AcceptToken(Parser, TokenType_LineSeparator);
   // if(Token) goto ret_null;
   // Token = AcceptToken(Parser, TokenType_ListSeparator);
   // if(Token) goto ret_null;
   
   // // Array (data)
   // ast_node *Left = (vptr)AcceptArray(Parser);
   // if(Left) goto ret;
   
   // Left = (vptr)AcceptIdentifier(Parser);
   // if(!Left) Left = (vptr)AcceptImmediate(Parser);
   
   // b08 IsPartial = FALSE;
   // ast_operation *Op;
   // Token = GetToken(Parser, 0);
   // Parser->Parent = NULL;
   // while(Op = AcceptOperator(Parser)) {
   //    Parser->Parent = (ast_node*)Op;
   //    ast_node *Right = (vptr)AcceptIdentifier(Parser);
   //    if(!Right) Right = (vptr)AcceptImmediate(Parser);
   //    Parser->Parent = Parent;
      
   //    b08 RightToLeft;
   //    u32 Precedence = GetPrecedence(Op->Type, &IsPartial, &RightToLeft);
   //    if( Right && Precedence == -1) Assembler_Log(Parser, Token, SyntaxErrorF, CFStringL("There isn't any binary operator '%s'\n", Token->Str));
   //    if(!Right && Precedence == -1) Assembler_Log(Parser, Token, SyntaxErrorF, CFStringL("There isn't any unary operator '%s'\n", Token->Str));
      
   //    ast_node *ParentOp = Parent;
   //    ast_node *CurrOp;
      
   //    while(ParentOp != NullNode) {
   //       CurrOp = ParentOp->FirstChild->Prev;
         
   //       s32 CurrPrecedence = 0;
   //       if(CurrOp->Type & TokenGroup_Mask) {
   //          b08 _RightToLeft;
   //          CurrPrecedence = GetPrecedenceFromNode((vptr)CurrOp);
   //          AAssert(CurrPrecedence != -1, "Unknown operator!");
   //       }
         
   //       if(CurrPrecedence < Precedence || (!RightToLeft && (u32)CurrPrecedence == Precedence)) {
   //          if(CurrOp != NullNode) RemoveChild(ParentOp, -1);
   //          AddChild(ParentOp, (ast_node*)Op, -1);
   //          AddChild((ast_node*)Op, CurrOp, 0);
   //          break;
   //       }
         
   //       ParentOp = CurrOp;
   //    }
      
   //    Left = Right;
   // }
   
   // ret:
   // Parser->Parent = Parent;
   // Token = AcceptToken(Parser, TokenType_ListSeparator);
   // if(Token) {
   //    *FinalExpression = FALSE;
   // } else {
   //    Token = AcceptToken(Parser, TokenType_LineSeparator);
   //    if(!Token) Assembler_Log(Parser, (Token = GetToken(Parser, 0)), SyntaxErrorF, CFStringL("An expression must end with a comma, a semicolon, or a new line. However, this expression ends with '%s'\n", Token->Str));
   //    *FinalExpression = TRUE;
   // }
   // return Left;
   
   // ret_null:
   // *FinalExpression = TRUE;
   // Parser->Index = Index;
   // return NULL;
}

internal ast_instruction *
AcceptInstruction(parser *Parser)
{
   ast_node *Parent = Parser->Parent;
   
   token *Token;
   while(Token = AcceptToken(Parser, TokenType_LineSeparator));
   
   u32 Index = Parser->Index;
   
   token *Ident = AcceptToken(Parser, TokenType_Identifier);
   if(!Ident) goto ret;
   
   instruction *Inst = HashMap_Get(Parser->InstMap, &Ident->Str);
   if(!Inst) Assembler_Log(Parser, Ident, SyntaxErrorF, CFStringL("The instruction '%s' is unknown, maybe it was misspelled?\n", Ident->Str));
   
   ast_instruction *Instruction = CreateASTInstruction(*Inst, Parser->Parent, Ident);
   Parser->Parent = (ast_node*)Instruction;
   
   while(AcceptExpression(Parser)) {
      if(AcceptToken(Parser, TokenType_LineSeparator)) {
         Parser->Parent = Parent;
         return Instruction;
      }
      
      if(!AcceptToken(Parser, TokenType_ListSeparator))
         break;
   }
   
   ret:
   Parser->Index = Index;
   return NULL;
}

internal ast_statement *
AcceptStatement(parser *Parser)
{
   ast_node *Parent = Parser->Parent;
   u32 Index = Parser->Index;
   token *Token;
   
   while(Token = AcceptToken(Parser, TokenType_LineSeparator));
   
   ast_statement *Statement = CreateASTStatement(Parser->Parent, Parser->Tokens+Parser->Index);
   Parser->Parent = (ast_node*)Statement;
   
   ast_label *Label;
   b08 HasLabel = FALSE;
   while(Label = AcceptLabel(Parser)) HasLabel = TRUE;
   
   ast_instruction *Instruction = AcceptInstruction(Parser);
   if(HasLabel && !Instruction) Assembler_Log(Parser, GetToken(Parser, 0), SyntaxErrorF, CFStringL("Labels must apply to an instruction or data, but '%s' is being applied to %s\n", Label->Header.Token->Str, GetStringUntil(Parser, 100, TokenType_LineSeparator)));
   if(!HasLabel && !Instruction) goto ret;
   
   Parser->Parent = Parent;
   return Statement;
   
   ret:
   Parser->Index = Index;
   RemoveChild(Parent, -1);
   Heap_FreeA(Statement);
   return NULL;
}

internal ast_section *
AcceptSection(parser *Parser)
{
   ast_node *Parent = Parser->Parent;
   
   token *Token;
   while(Token = AcceptToken(Parser, TokenType_LineSeparator));
   
   u32 Index = Parser->Index;
   
   Token = AcceptToken(Parser, TokenType_DirectiveStart);
   if(!Token) goto ret;
   
   Token = AcceptExactIdentifier(Parser, CLStringL("section"));
   if(!Token) goto ret;
   
   Token = AcceptToken(Parser, TokenType_Identifier);
   if(!Token) Assembler_Log(Parser, Token, SyntaxErrorF, CFStringL("We expected an identifier after '.section', but found %s instead\n", Token->Str));
   
   ast_section *Section = CreateASTSection(Parent, Token);
   Parser->Parent = (vptr)Section;
   
   Token = AcceptToken(Parser, TokenType_LineSeparator);
   if(!Token) Assembler_Log(Parser, Token, SyntaxErrorF, CFStringL("A section declaration needs to end with a ';' or with a new line, but we found %s instead\n", Token->Str));
   
   ast_statement *Statement;
   while(Statement = AcceptStatement(Parser));
   
   Parser->Parent = Parent;
   return Section;
   
   ret:
   Parser->Index = Index;
   return NULL;
}

internal parser
ParseTokens(token *Tokens, hashmap *InstMap, string FileName)
{
   b08 Found;
   
   ast_node _NullNode;
   NullNode = &_NullNode;
   _NullNode.Type = ASTType_Invalid;
   _NullNode.ChildCount = 0;
   _NullNode.Prev = NullNode;
   _NullNode.Next = NullNode;
   _NullNode.FirstChild = NullNode;
   
   parser Parser;
   Parser.Tokens = Tokens;
   Parser.TokenCount = Heap_GetHandleA(Tokens)->Size / sizeof(token);
   Parser.Index = 0;
   Parser.FileName = FileName;
   Parser.InstMap = InstMap;
   
   Parser.AST = CreateASTRoot();
   Parser.Parent = (ast_node*)Parser.AST;
   
   ast_section *Section;
   b08 HasSection = FALSE;
   while(Section = AcceptSection(&Parser)) HasSection = TRUE;
   
   if(!HasSection) {
      //TODO Clean this up somehow
      token *Token = Parser.DummySection = Heap_AllocateA(_Heap, sizeof(token));
      Token->Str = CLStringL("text");
      Token->Type = TokenType_Identifier;
      Token->ColumnNumber = 0;
      Token->LineNumber = 0;
      
      Parser.Parent = (vptr)CreateASTSection((vptr)Parser.Parent, Token);
      
      ast_statement *Statement;
      while(Statement = AcceptStatement(&Parser));
   }
   
   while(AcceptToken(&Parser, TokenType_LineSeparator));
   
   if(Parser.Index < Parser.TokenCount) {
      u32 Length = 0;
      token Token = Parser.Tokens[Parser.Index++];
      token FirstToken = Token;
      c08 *Text = Token.Str.Text;
      c08 *EndText = Text;
      
      u32 LenMax = 100;
      while(Parser.Index < Parser.TokenCount && (u64)EndText - (u64)Text < LenMax && Token.Type != TokenType_LineSeparator) {
         Token = Parser.Tokens[Parser.Index++];
         EndText = Token.Str.Text + Token.Str.Length;
      }
      
      Length = (u64)EndText - (u64)Text;
      
      if(Token.Type == TokenType_LineSeparator && Length < LenMax) {
         Assembler_Log(&Parser, &FirstToken, SyntaxErrorF, CFStringL("We expected a statement, but got \n%s\n", CLString(Text, Length)));
      } else if(Length >= 100) {
         Assembler_Log(&Parser, &FirstToken, SyntaxErrorF, CFStringL("We expected a statement, but got \n%s ...\n", CLString(Text, Length)));
      } else {
         AAssert(FALSE, "The statement never ended! Likely, EOF wasn't registered as a line separator.");
      }
   }
   
   return Parser;
}

#endif