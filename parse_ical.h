#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    default: return "<Unspecified>";
    }
}
