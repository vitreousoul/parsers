#include <stdio.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint32_t u32;

typedef int32_t s32;

typedef uint32_t b32;

typedef size_t size;

typedef enum
{
    char_code_CR = 0x0d,
    char_code_LF = 0x0a,
    char_code_Space = ' ',
    char_code_Tab = '\t',
    char_code_Newline = '\n',
} char_code;

typedef struct
{
    s32 Size;
    u8 *Data;
} buffer;

typedef enum
{
    parser_state_None,
    parser_state_Error,
    parser_state_ContentLine,
    parser_state_Params,
    parser_state_Name,
    parser_state_Param,
    parser_state_ParamRest,
    parser_state_Value,
    parser_state_CRLF,
    parser_state_IanaToken,
    parser_state_IanaChar,
    parser_state_XName,
    parser_state_ParamName,
    parser_state_ParamValue,
    parser_state_VendorId,
    parser_state_ParamText,
    parser_state_QuotedString,
    parser_state_ValueChar,
    parser_state_QsafeChar,
    parser_state_SafeChar,
    parser_state_NonUsAscii,
    parser_state_Alpha,
    parser_state_AlphaLower,
    parser_state_AlphaUpper,
    parser_state_AlphaNum,
    parser_state_Digit,
} parser_state;

typedef struct
{
    parser_state State;
    s32 I;
    b32 InAssignment;
} parser;

typedef enum
{
    skip_state_CR_or_LF,
    skip_state_LF,
    skip_state_Space,
    skip_state_Done,
} skip_state;

typedef struct
{
    u8 *Begin;
    u8 *End;
} range;

typedef enum
{
    binding_kind_None,
    binding_kind_Bind,
    binding_kind_Assign,
} binding_kind;

typedef struct
{
    range Name;
    binding_kind Kind;
    range AssignName;
    range Value;
} binding;

char *DebugParserState(parser_state State);

char *DebugParserState(parser_state State)
{
    switch(State)
    {
    case parser_state_None: return "None";
    case parser_state_Error: return "Error";
    case parser_state_ContentLine: return "ContentLine";
    case parser_state_Params: return "Params";
    case parser_state_Name: return "Name";
    case parser_state_Param: return "Param";
    case parser_state_ParamRest: return "ParamRest";
    case parser_state_Value: return "Value";
    case parser_state_CRLF: return "CRLF";
    case parser_state_IanaToken: return "IanaToken";
    case parser_state_IanaChar: return "IanaChar";
    case parser_state_XName: return "XName";
    case parser_state_ParamName: return "ParamName";
    case parser_state_ParamValue: return "ParamValue";
    case parser_state_VendorId: return "VendorId";
    case parser_state_ParamText: return "ParamText";
    case parser_state_QuotedString: return "QuotedString";
    case parser_state_ValueChar: return "ValueChar";
    case parser_state_QsafeChar: return "QsafeChar";
    case parser_state_SafeChar: return "SafeChar";
    case parser_state_NonUsAscii: return "NonUsAscii";
    case parser_state_Alpha: return "Alpha";
    case parser_state_AlphaLower: return "AlphaLower";
    case parser_state_AlphaUpper: return "AlphaUpper";
    case parser_state_AlphaNum: return "AlphaNum";
    case parser_state_Digit: return "Digit";
    default: return "<Unspecified>";
    }
}
