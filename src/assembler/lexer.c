// /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
// **                                                                         **
// **  Author: Aria Seiler                                                    **
// **                                                                         **
// **  This program is in the public domain. There is no implied warranty,    **
// **  so use it at your own risk.                                            **
// **                                                                         **
// \* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// #ifdef INCLUDE_HEADER

// typedef enum assembler_token_type {
//    Assembler_TokenType_Invalid,
//    Assembler_TokenType_Comment,
//    Assembler_TokenType_Label,
//    Assembler_TokenType_Mnemonic,
//    Assembler_TokenType_InstructionSeparator,
//    Assembler_TokenType_OperandSeparator,
//    Assembler_TokenType_Operand,
//    Assembler_TokenType_Array,
// } assembler_token_type;

// typedef struct tokenizer_token {
//    string Str;
//    assembler_token_type Type;
// } assembler_token;

// typedef struct assembler_tokenizer {
//    u64 TokenCount;
//    HEAP(assembler_token*) Tokens;
// } assembler_tokenizer;

// #endif



// #ifdef INCLUDE_SOURCE

// internal void
// Tokenizer_SkipWhitespace

// internal assembler_tokenizer
// Tokenizer_Begin(string Source)
// {
//    assembler_tokenizer Tokens;
   
//    u32 Len = Source.Length;
//    c08 *C = Source.Text;
//    while(Len--) {
//       Assembler_SkipWhitespace(&C);
      
//       C++;
//    }
   
//    return (assembler_tokenizer){TokenCount, };
// }

// #endif