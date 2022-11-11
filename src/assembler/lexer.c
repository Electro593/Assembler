/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
**                                                                         **
**  Author: Aria Seiler                                                    **
**                                                                         **
**  This program is in the public domain. There is no implied warranty,    **
**  so use it at your own risk.                                            **
**                                                                         **
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef INCLUDE_HEADER

#define TokenGroup__ 0
#define _ 0
#define TokenGroup_LeftToRight 0x00000000
#define TokenGroup_RightToLeft 0x80000000

#define OPERATORS \
   OP("+",  0x0A,  _,_,        2,RightToLeft,   _,_,              4,LeftToRight,   _,_          ) \
   OP("-",  0x0A,  _,_,        2,RightToLeft,   _,_,              4,LeftToRight,   _,_          ) \
   OP("*",  0x08,  _,_,        _,_,             _,_,              3,LeftToRight,   _,_          ) \
   OP("/",  0x08,  _,_,        _,_,             _,_,              3,LeftToRight,   _,_          ) \
   OP("%",  0x08,  _,_,        _,_,             _,_,              3,LeftToRight,   _,_          ) \
   OP("~",  0x02,  _,_,        2,RightToLeft,   _,_,              _,_,             _,_          ) \
   OP("&",  0x08,  _,_,        _,_,             _,_,              8,LeftToRight,   _,_          ) \
   OP("|",  0x08,  _,_,        _,_,             _,_,             10,LeftToRight,   _,_          ) \
   OP("^",  0x08,  _,_,        _,_,             _,_,              9,LeftToRight,   _,_          ) \
   OP("<<", 0x08,  _,_,        _,_,             _,_,              5,LeftToRight,   _,_          ) \
   OP(">>", 0x08,  _,_,        _,_,             _,_,              5,LeftToRight,   _,_          ) \
   OP("!",  0x02,  _,_,        2,RightToLeft,   _,_,              _,_,             _,_          ) \
   OP("&&", 0x08,  _,_,        _,_,             _,_,             11,LeftToRight,   _,_          ) \
   OP("||", 0x08,  _,_,        _,_,             _,_,             13,LeftToRight,   _,_          ) \
   OP("^^", 0x08,  _,_,        _,_,             _,_,             12,LeftToRight,   _,_          ) \
   OP("<",  0x08,  _,_,        _,_,             _,_,              6,LeftToRight,   _,_          ) \
   OP("<=", 0x08,  _,_,        _,_,             _,_,              6,LeftToRight,   _,_          ) \
   OP(">",  0x08,  _,_,        _,_,             _,_,              6,LeftToRight,   _,_          ) \
   OP(">=", 0x08,  _,_,        _,_,             _,_,              6,LeftToRight,   _,_          ) \
   OP("==", 0x08,  _,_,        _,_,             _,_,              7,LeftToRight,   _,_          ) \
   OP("!=", 0x08,  _,_,        _,_,             _,_,              7,LeftToRight,   _,_          ) \
   OP("?",  0x10,  _,_,        _,_,             _,_,              _,_,            14,RightToLeft) \
   OP(":",  0x10,  _,_,        _,_,             _,_,              _,_,            14,RightToLeft) \
   OP("(",  0x02,  _,_,  S32_MAX,RightToLeft,   _,_,              _,_,             _,_          ) \
   OP(")",  0x04,  _,_,        _,_,            -2,LeftToRight,    _,_,             _,_          ) \

s32 NullaryPrecedences[] = {
   #define OP(S,F, P0,A0, ...) \
      (F & 0x01) ? ((P0 & 0x7FFFFFFF) | TokenGroup_##A0) : 0x7FFFFFFF,
   OPERATORS
   #undef OP
};

s32 UnaryPrefixPrecedences[] = {
   #define OP(S,F, P0,A0, P1,A1, ...) \
      (F & 0x02) ? ((P1 & 0x7FFFFFFF) | TokenGroup_##A1) : 0x7FFFFFFF,
   OPERATORS
   #undef OP
};

s32 UnaryPostfixPrecedences[] = {
   #define OP(S,F, P0,A0, P1,A1, P2,A2, ...) \
      (F & 0x04) ? ((P2 & 0x7FFFFFFF) | TokenGroup_##A2) : 0x7FFFFFFF,
   OPERATORS
   #undef OP
};

s32 BinaryInfixPrecedences[] = {
   #define OP(S,F, P0,A0, P1,A1, P2,A2, P3,A3, ...) \
      (F & 0x08) ? ((P3 & 0x7FFFFFFF) | TokenGroup_##A3) : 0x7FFFFFFF,
   OPERATORS
   #undef OP
};

s32 TernaryPrecedences[] = {
   #define OP(S,F, P0,A0, P1,A1, P2,A2, P3,A3, P4,A4, ...) \
      (F & 0x10) ? ((P4 & 0x7FFFFFFF) | TokenGroup_##A4) : 0x7FFFFFFF,
   OPERATORS
   #undef OP
};

#undef _

typedef enum token_type {
   TokenType_Mask           = 0x00FFFFFF,
   TokenGroup_Mask          = 0xFF000000,
   
   TokenType_Invalid        = 0x00000000,
   
   TokenGroup_Ambiguous     = 0x80000000,
   
   TokenGroup_Structural    = 0x01000000,
   TokenType_LabelEnd       = 0x00000001 | TokenGroup_Structural | TokenGroup_Ambiguous,
   TokenType_DirectiveStart = 0x00000002 | TokenGroup_Structural,
   TokenType_Identifier     = 0x00000003 | TokenGroup_Structural,
   TokenType_Number         = 0x00000004 | TokenGroup_Structural,
   TokenType_Radix          = 0x00000005 | TokenGroup_Structural,
   TokenType_LineSeparator  = 0x00000006 | TokenGroup_Structural,
   TokenType_ListSeparator  = 0x00000007 | TokenGroup_Structural,
   TokenType_ArrayStart     = 0x00000008 | TokenGroup_Structural,
   TokenType_ArrayEnd       = 0x00000009 | TokenGroup_Structural,
   
   TokenGroup_Operation     = 0x02000000,
   TokenType_Addition       = 0x00000000 | TokenGroup_Operation,
   TokenType_Subtraction    = 0x00000001 | TokenGroup_Operation,
   TokenType_Multiplication = 0x00000002 | TokenGroup_Operation,
   TokenType_Division       = 0x00000003 | TokenGroup_Operation,
   TokenType_Modulo         = 0x00000004 | TokenGroup_Operation,
   TokenType_BitwiseNOT     = 0x00000005 | TokenGroup_Operation,
   TokenType_BitwiseAND     = 0x00000006 | TokenGroup_Operation,
   TokenType_BitwiseOR      = 0x00000007 | TokenGroup_Operation,
   TokenType_BitwiseXOR     = 0x00000008 | TokenGroup_Operation,
   TokenType_LeftShift      = 0x00000009 | TokenGroup_Operation,
   TokenType_RightShift     = 0x0000000A | TokenGroup_Operation,
   TokenType_LogicalNOT     = 0x0000000B | TokenGroup_Operation,
   TokenType_LogicalAND     = 0x0000000C | TokenGroup_Operation,
   TokenType_LogicalOR      = 0x0000000D | TokenGroup_Operation,
   TokenType_LogicalXOR     = 0x0000000E | TokenGroup_Operation,
   TokenType_Less           = 0x0000000F | TokenGroup_Operation,
   TokenType_LessEqual      = 0x00000010 | TokenGroup_Operation,
   TokenType_Greater        = 0x00000011 | TokenGroup_Operation,
   TokenType_GreaterEqual   = 0x00000012 | TokenGroup_Operation,
   TokenType_Equal          = 0x00000013 | TokenGroup_Operation,
   TokenType_NotEqual       = 0x00000014 | TokenGroup_Operation,
   TokenType_CondTrue       = 0x00000015 | TokenGroup_Operation,
   TokenType_CondFalse      = 0x00000015 | TokenGroup_Operation | TokenGroup_Ambiguous,
   TokenType_GroupStart     = 0x00000016 | TokenGroup_Operation,
   TokenType_GroupEnd       = 0x00000017 | TokenGroup_Operation,
} token_type;

typedef struct token {
   u32 LineNumber;
   u32 ColumnNumber;
   string Str;
   token_type Type;
} token;

typedef struct ambiguous_token {
   token Header;
   token_type *Types;
} ambiguous_token;

#endif



#ifdef INCLUDE_SOURCE

internal s32
GetPrecedence(token_type TokenType, b08 *IsPartial, b08 *RightToLeft)
{
   if((TokenType & TokenGroup_Mask) != TokenGroup_Operation) {
      if(IsPartial) *IsPartial = FALSE;
      if(RightToLeft) *RightToLeft = TRUE;
      return 0;
   }
   
   u32 Types[] = {
      NullaryPrecedences[TokenType & TokenType_Mask] & 0x80000000,
      UnaryPrefixPrecedences[TokenType & TokenType_Mask] & 0x80000000,
      UnaryPostfixPrecedences[TokenType & TokenType_Mask] & 0x80000000,
      BinaryInfixPrecedences[TokenType & TokenType_Mask] & 0x80000000,
      TernaryPrecedences[TokenType & TokenType_Mask] & 0x80000000,
   };
   
   u32 Precedences[] = {
      NullaryPrecedences[TokenType & TokenType_Mask] & 0x7FFFFFFF,
      UnaryPrefixPrecedences[TokenType & TokenType_Mask] & 0x7FFFFFFF,
      UnaryPostfixPrecedences[TokenType & TokenType_Mask] & 0x7FFFFFFF,
      BinaryInfixPrecedences[TokenType & TokenType_Mask] & 0x7FFFFFFF,
      TernaryPrecedences[TokenType & TokenType_Mask] & 0x7FFFFFFF,
   };
   
   s32 MinIndex;
   if(!IsPartial || *IsPartial) {
      MinIndex = 0;
      for(s32 I = 1; I < sizeof(Precedences)/sizeof(s32); I++)
         if(Precedences[MinIndex] > Precedences[I]) MinIndex = I;
   } else {
      // Nullary and Unary prefixes aren't allowed because we can't have
      // multiple full statements
      MinIndex = 2;
      if(Precedences[MinIndex] > Precedences[3]) MinIndex = 3;
   }
   
   if(IsPartial) {
      if(MinIndex == 1 || MinIndex == 3) *IsPartial = TRUE;
      else *IsPartial = FALSE;
   }
   
   if(RightToLeft) *RightToLeft = !!(Types[MinIndex] & TokenGroup_RightToLeft);
   
   return Precedences[MinIndex] << 1 >> 1;
}

internal b08
IsNumeric(c08 C)
{
   return ('0' <= C && C <= '9');
}

internal b08
IsAlphabetical(c08 C)
{
   return ('A' <= C && C <= 'Z') || ('a' <= C && C <= 'z');
}

internal void
SkipWhitespace(c08 **Text)
{
   c08 *C = *Text;
   while(*C == ' ' || *C == '\t' || *C == '\r') C++;
   *Text = C;
}

internal token *
TokenizeFile(string Source)
{
   u32 TokenCount = 0;
   token *Tokens = Heap_AllocateA(_Heap, 0);
   
   c08 *C = Source.Text;
   c08 *End = C + Source.Length;
   u32 LineNumber = 1;
   c08 *LineStart = C;
   while(C < End) {
      SkipWhitespace(&C);
      
      u32 TokensSize = Heap_GetHandleA(Tokens)->Size - sizeof(heap_handle*);
      if(TokensSize <= TokenCount*sizeof(token)) {
         Heap_ResizeA(&Tokens, TokensSize + 512*sizeof(token));
         Mem_Set(Tokens+TokenCount, 0, 512*sizeof(token));
      }
      
      token Token;
      Token.Str.Text = C;
      Token.Str.Resizable = FALSE;
      Token.LineNumber = LineNumber;
      Token.ColumnNumber = (u64)C - (u64)LineStart + 1;
      
      if(*C == '#') {
         if(*(C+1) == '*') {
            C += 2;
            u32 CommentLevel = 1;
            while(*C && CommentLevel) {
               if(*C == '#') {
                  if(*(C-1) == '*') CommentLevel--;
                  else if(*(C+1) == '*') CommentLevel++;
               } else if(*C == '\n') {
                  LineNumber++;
                  LineStart = C+1;
               }
               C++;
            }
            Assert(C, "Multi-line comment never ended!");
         } else {
            while(*C && *C != '\n') C++;
         }
         continue;
      } else if(*C == '_' || IsAlphabetical(*C) || IsNumeric(*C)) {
         b08 IsNumber = IsNumeric(*C);
         b08 ContainsSymbols = FALSE;
         while(IsAlphabetical(*C) || IsNumeric(*C) || *C == '_' || *C == '$' || *C == '@') {
            if(*C == '$' || *C == '@') ContainsSymbols = TRUE;
            C++;
         }
         
         if(IsNumber || *C == '`') {
            Token.Str.Length = (u64)C - (u64)Token.Str.Text;
            Token.Type = TokenType_Number;
         } else {
            Assert(!ContainsSymbols, "Syntax error: Instruction names, registers, and labels cannot contain '$' or '@'");
            
            Token.Str.Length = (u64)C - (u64)Token.Str.Text;
            Token.Type = TokenType_Identifier;
         }
      } else if(*C == '`') {
         C++;
         while(IsNumeric(*C) || *C == '_') C++;
         Token.Str.Text++; // Skip the backtick
         Token.Str.Length = (u64)C - (u64)Token.Str.Text;
         Token.Type = TokenType_Radix;
      } else if(*C == '[') {
         Token.Str.Length = 1;
         Token.Type = TokenType_ArrayStart;
         C++;
      } else if(*C == ']') {
         Token.Str.Length = 1;
         Token.Type = TokenType_ArrayEnd;
         C++;
      } else if(*C == ':') {
         Token.Str.Length = 1;
         Token.Type = TokenType_LabelEnd;
         C++;
         //TODO: TokenType_CondFalse
      } else if(*C == '.') {
         Token.Str.Length = 1;
         Token.Type = TokenType_DirectiveStart;
         C++;
      } else if(*C == ';' || *C == '\n') {
         Token.Str.Length = 1;
         Token.Type = TokenType_LineSeparator;
         
         if(*C == '\n') {
            LineNumber++;
            LineStart = C+1;
         }
         
         C++;
      } else if(*C == '\\') {
         C++;
         SkipWhitespace(&C);
         Assert(*C == '\n');
         C++;
         continue;
      } else if(*C == ',') {
         Token.Str.Length = 1;
         Token.Type = TokenType_ListSeparator;
         C++;
      } else if(*C == '+') {
         Token.Str.Length = 1;
         Token.Type = TokenType_Addition;
         C++;
      } else if(*C == '-') {
         Token.Str.Length = 1;
         Token.Type = TokenType_Subtraction;
         C++;
      } else if(*C == '*') {
         Token.Str.Length = 1;
         Token.Type = TokenType_Multiplication;
         C++;
      } else if(*C == '/') {
         Token.Str.Length = 1;
         Token.Type = TokenType_Division;
         C++;
      } else if(*C == '%') {
         Token.Str.Length = 1;
         Token.Type = TokenType_Modulo;
         C++;
      } else if(*C == '&') {
         if(*(C+1) == '&') {
            Token.Str.Length = 2;
            Token.Type = TokenType_LogicalAND;
            C += 2;
         } else {
            Token.Str.Length = 1;
            Token.Type = TokenType_BitwiseAND;
            C++;
         }
      } else if(*C == '|') {
         if(*(C+1) == '|') {
            Token.Str.Length = 2;
            Token.Type = TokenType_LogicalOR;
            C += 2;
         } else {
            Token.Str.Length = 1;
            Token.Type = TokenType_BitwiseOR;
            C++;
         }
      } else if(*C == '^') {
         if(*(C+1) == '^') {
            Token.Str.Length = 2;
            Token.Type = TokenType_LogicalXOR;
            C += 2;
         } else {
            Token.Str.Length = 1;
            Token.Type = TokenType_BitwiseXOR;
            C++;
         }
      } else if(*C == '<') {
         if(*(C+1) == '<') {
            Token.Str.Length = 2;
            Token.Type = TokenType_LeftShift;
            C += 2;
         } else if(*(C+1) == '=') {
            Token.Str.Length = 2;
            Token.Type = TokenType_LessEqual;
            C += 2;
         } else {
            Token.Str.Length = 1;
            Token.Type = TokenType_Less;
            C++;
         }
      } else if(*C == '>') {
         if(*(C+1) == '>') {
            Token.Str.Length = 2;
            Token.Type = TokenType_RightShift;
            C += 2;
         } else if(*(C+1) == '=') {
            Token.Str.Length = 2;
            Token.Type = TokenType_GreaterEqual;
            C += 2;
         } else {
            Token.Str.Length = 1;
            Token.Type = TokenType_Greater;
            C++;
         }
      } else if(*C == '=' && *(C+1) == '=') {
         Token.Str.Length = 2;
         Token.Type = TokenType_Equal;
         C += 2;
      } else if(*C == '!') {
         if(*(C+1) == '=') {
            Token.Str.Length = 2;
            Token.Type = TokenType_NotEqual;
            C += 2;
         } else {
            Token.Str.Length = 1;
            Token.Type = TokenType_LogicalNOT;
            C++;
         }
      } else if(*C == '~') {
         Token.Str.Length = 1;
         Token.Type = TokenType_BitwiseNOT;
         C++;
      } else if(*C == '?') {
         Token.Str.Length = 1;
         Token.Type = TokenType_CondTrue;
         C++;
      } else if(*C == '(') {
         Token.Str.Length = 1;
         Token.Type = TokenType_GroupStart;
         C++;
      } else if(*C == ')') {
         Token.Str.Length = 1;
         Token.Type = TokenType_GroupEnd;
         C++;
      } else {
         Assert(FALSE, "Unhandled token!");
         C++;
      }
      
      Token.Str.Capacity = Token.Str.Length;
      Tokens[TokenCount++] = Token;
   }
   
   Heap_ResizeA(&Tokens, (TokenCount+1)*sizeof(token));
   
   token Token;
   Token.Str.Text = NULL;
   Token.Str.Resizable = FALSE;
   Token.Str.Length = 1;
   Token.Str.Capacity = 1;
   Token.Type = TokenType_LineSeparator;
   Tokens[TokenCount++] = Token;
   
   return Tokens;
}

#endif