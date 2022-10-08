/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
**                                                                         **
**  Author: Aria Seiler                                                    **
**                                                                         **
**  This program is in the public domain. There is no implied warranty,    **
**  so use it at your own risk.                                            **
**                                                                         **
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef INCLUDE_HEADER

typedef enum token_type {
   TokenType_Invalid,
   TokenType_Comment,
   TokenType_LabelSuffix,
   TokenType_MacroPrefix,
   TokenType_Identifier,
   TokenType_Number,
   TokenType_Radix,
   TokenType_LineSeparator,
   TokenType_ListSeparator,
   TokenType_ArrayStart,
   TokenType_ArrayEnd,
   TokenType_Subtract,
} token_type;

typedef struct token {
   string Str;
   token_type Type;
} token;

#endif



#ifdef INCLUDE_SOURCE

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

internal heap_handle *
TokenizeFile(string Source)
{
   u32 TokenCount = 0;
   heap_handle *Tokens = Heap_Allocate(_Heap, 0);
   
   c08 *C = Source.Text;
   c08 *End = C + Source.Length;
   while(C < End) {
      SkipWhitespace(&C);
      
      if(Tokens->Size <= TokenCount*sizeof(token)) {
         Heap_Resize(Tokens, Tokens->Size + 512*sizeof(token));
         Mem_Set((token*)Tokens->Data+TokenCount, 0, 512*sizeof(token));
      }
      
      token Token;
      Token.Str.Text = C;
      Token.Str.Resizable = FALSE;
      
      if(*C == '#') {
         if(*(C+1) == '*') {
            C += 2;
            u32 CommentLevel = 1;
            while(*C && CommentLevel) {
               if(*C == '#') {
                  if(*(C-1) == '*') CommentLevel--;
                  else if(*(C+1) == '*') CommentLevel++;
               }
               C++;
            }
            Assert(C, "Multi-line comment never ended!");
         } else {
            while(*C && *C != '\n') C++;
         }
         
         Token.Str.Length = (u64)C - (u64)Token.Str.Text;
         Token.Str.Capacity = Token.Str.Length;
         Token.Type = TokenType_Comment;
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
         Token.Type = TokenType_LabelSuffix;
         C++;
      } else if(*C == '.') {
         Token.Str.Length = 1;
         Token.Type = TokenType_MacroPrefix;
         C++;
      } else if(*C == ';' || *C == '\n') {
         Token.Str.Length = 1;
         Token.Type = TokenType_LineSeparator;
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
      } else if(*C == '-') {
         Token.Str.Length = 1;
         Token.Type = TokenType_Subtract;
         C++;
      } else {
         Assert(FALSE, "Unhandled token!");
         C++;
      }
      
      Token.Str.Capacity = Token.Str.Length;
      ((token*)Tokens->Data)[TokenCount++] = Token;
   }
   
   Heap_Resize(Tokens, TokenCount*sizeof(token));
   return Tokens;
}

#endif