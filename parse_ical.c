#include "parse_ical.h"

#define CHAR_IS_LOWER_CASE(char) (((char) >= 'a') && ((char) <= 'z'))
#define CHAR_IS_UPPER_CASE(char) (((char) >= 'A') && ((char) <= 'Z'))
#define CHAR_IS_IDENT_START(char) (CHAR_IS_LOWER_CASE(char) || CHAR_IS_UPPER_CASE(char))
#define CHAR_IS_IDENT_REST(char) (((char) == '-') || CHAR_IS_IDENT_START(char))
#define CHAR_IS_SPACE(char) (((char) == ' ') || ((char) == '\t'))
#define IS_ICAL_ESCAPE_CODE(char) ((char) == ',' || (char) == ';')

#define PEEK(buffer, parser) ((parser)->I < (buffer)->Size ? (buffer)->Data[(parser)->I] : 0)

#define IDENT_CACHE_MAX 2048
s32 IdentifierCacheCount = 0;
u8 IDENTIFIER_CACHE[IDENT_CACHE_MAX];

static void *AllocBuffer(u32 Size)
{
    u8 *Data = malloc(sizeof(u8) * Size);
    buffer *Buffer = malloc(sizeof(buffer));
    Buffer->Size = Size;
    Buffer->Data = Data;
    return Buffer;
}

static void FreeBuffer(buffer *Buffer)
{
    free(Buffer->Data);
    free(Buffer);
}

static buffer *ReadFileIntoBuffer(char *FilePath)
{
    u32 Size = 0;
    buffer *Buffer;
    FILE *File = fopen(FilePath, "rb");
    if(!File)
    {
        return 0;
    }
    /* get file size */
    fseek(File, 0, SEEK_END);
    Size = ftell(File);
    /* allocate and write to buffer */
    Buffer = AllocBuffer(Size);
    fseek(File, 0, SEEK_SET);
    fread(Buffer->Data, sizeof(u8), Size, File);
    return Buffer;
}

static parser CreateParser()
{
    parser Parser;
    Parser.State = parser_state_None;
    Parser.I = 0;
    Parser.InAssignment = 0;
    return Parser;
}

#if 0
static void ParseComment(buffer *Buffer, parser *Parser)
{
    b32 Running = 1;
    while(Running && Parser->I < Buffer->Size)
    {
        s32 BytesLeft = Buffer->Size - Parser->I;
        b32 CRFirst = Buffer->Data[Parser->I] == char_code_CR;
        b32 LFFirst = Buffer->Data[Parser->I] == char_code_LF;
        b32 LFSecond = BytesLeft >= 2 && Buffer->Data[Parser->I+1] == char_code_LF;
        b32 NewlineFirst = Buffer->Data[Parser->I] == char_code_Newline;
        s32 Newline1 = LFFirst || NewlineFirst;
        s32 Newline2 = CRFirst && LFSecond;
        if(Newline1)
        {
            ++Parser->I;
            break;
        }
        else if(Newline2)
        {
            Parser->I += 2;
            break;
        }
        else
        {
            ++Parser->I;
        }
    }
}

static void SkipSpace(parser *Parser, buffer *Buffer)
{
    /* printf("SkipSpace %d\n", Buffer->Data[Parser->I] == '\n'); */
    b32 Running = 1;
    /* s32 SkipState = skip_state_CR_or_LF; */
    while(Running && Parser->I < Buffer->Size)
    {
        /* u8 Char = Buffer->Data[Parser->I]; */
        /* u8 PeekChar = PEEK(Buffer, Parser); */
        s32 BytesLeft = Buffer->Size - Parser->I;
        b32 CRFirst = Buffer->Data[Parser->I] == char_code_CR;
        b32 LFFirst = Buffer->Data[Parser->I] == char_code_LF;
        b32 NewlineFirst = Buffer->Data[Parser->I] == char_code_Newline;
        b32 LFSecond = BytesLeft >= 2 && Buffer->Data[Parser->I+1] == char_code_LF;
        /* b32 NewlineSecond = BytesLeft >= 2 && Buffer->Data[Parser->I+1] == char_code_Newline; */
        b32 SpaceSecond = BytesLeft >= 2 && CHAR_IS_SPACE(Buffer->Data[Parser->I+1]);
        b32 SpaceThird = BytesLeft >= 3 && CHAR_IS_SPACE(Buffer->Data[Parser->I+2]);
        b32 LFSpace = (LFFirst || NewlineFirst) && SpaceSecond;
        b32 CRLFSpace = CRFirst && LFSecond && SpaceThird;
        b32 Newline1 = LFFirst || NewlineFirst;
        b32 Newline2 = CRFirst && LFSecond;
        b32 DashFirst = Buffer->Data[Parser->I] == '-';
        b32 DashSecond = BytesLeft >= 2 && Buffer->Data[Parser->I+1] == '-';
        if(DashFirst && DashSecond)
        {
            ParseComment(Buffer, Parser);
        }
        else if(LFSpace)
        {
            Parser->I += 2;
        }
        else if(CRLFSpace)
        {
            Parser->I += 3;
        }
        else if(Newline1)
        {
            ++Parser->I;
            Parser->State = parser_state_Root;
            Parser->InAssignment = 0;
        }
        else if(Newline2)
        {
            Parser->I += 2;
            Parser->State = parser_state_Root;
            Parser->InAssignment = 0;
            Running = 0;
        }
        else
        {
            Running = 0;
        }
    }
}

static range ParseIdentifier(buffer *Buffer, parser *Parser)
{
    u8 *Begin = Buffer->Data + Parser->I;
    range Range = {Begin, Begin};
    printf("%c", Buffer->Data[Parser->I]);
    if(!Parser->InAssignment)
    {
        IdentifierCacheCount = 0;
        /* printf("IDENTIFIER_CACHE %c\n", Buffer->Data[Parser->I]); */
        IDENTIFIER_CACHE[++IdentifierCacheCount] = Buffer->Data[Parser->I];
    }
    ++Parser->I; /* increment because we know the first char is ident-start */
    while(Parser->I < Buffer->Size)
    {
        if(CHAR_IS_IDENT_REST(Buffer->Data[Parser->I]))
        {
            printf("%c", Buffer->Data[Parser->I]);
            if(!Parser->InAssignment)
            {
                /* printf("IDENTIFIER_CACHE %c\n", Buffer->Data[Parser->I]); */
                IDENTIFIER_CACHE[++IdentifierCacheCount] = Buffer->Data[Parser->I];
            }
            ++Parser->I;
        }
        else
        {
            Parser->State = parser_state_BindOrAssign;
            break;
        }
    }
    printf(",");
    Range.End = Buffer->Data + Parser->I;
    return Range;
}

static range ParseData(buffer *Buffer, parser *Parser)
{
    /* printf("\nParseData %c\n", Buffer->Data[Parser->I]); */
    u8 *Begin = Buffer->Data + Parser->I;
    range Range = {Begin, Begin};
    s32 BytesLeft = Buffer->Size - Parser->I;
    b32 Running = 1;
    printf("\"");
    while(Running && Parser->I < Buffer->Size)
    {
        b32 CRFirst = Buffer->Data[Parser->I] == char_code_CR;
        b32 LFFirst = Buffer->Data[Parser->I] == char_code_LF;
        b32 NewlineFirst = Buffer->Data[Parser->I] == char_code_Newline;
        b32 LFSecond = BytesLeft >= 2 && Buffer->Data[Parser->I+1] == char_code_LF;
        /* b32 NewlineSecond = BytesLeft >= 2 && Buffer->Data[Parser->I+1] == char_code_Newline; */
        b32 SpaceSecond = BytesLeft >= 2 && CHAR_IS_SPACE(Buffer->Data[Parser->I+1]);
        b32 SpaceThird = BytesLeft >= 3 && CHAR_IS_SPACE(Buffer->Data[Parser->I+2]);
        b32 LFSpace = (LFFirst || NewlineFirst) && SpaceSecond;
        b32 CRLFSpace = CRFirst && LFSecond && SpaceThird;
        s32 Newline1 = LFFirst || NewlineFirst;
        s32 Newline2 = CRFirst && LFSecond;
        if(LFSpace)
        {
            Parser->I += 2;
        }
        else if(CRLFSpace)
        {
            Parser->I += 3;
        }
        else if (Newline1)
        {
            Parser->State = parser_state_Root;
            Parser->InAssignment = 0;
            ++Parser->I;
            Running = 0;
        }
        else if (Newline2)
        {
            Parser->State = parser_state_Root;
            Parser->InAssignment = 0;
            Parser->I += 2;
            Running = 0;
        }
        else if(Buffer->Data[Parser->I] == ';')
        {
            Parser->State = parser_state_Assign;
            Parser->InAssignment = 1;
            ++Parser->I;
            Running = 0;
        }
        else
        {
            u8 PeekChar = PEEK(Buffer, Parser);
            if(Buffer->Data[Parser->I] == '\\' && IS_ICAL_ESCAPE_CODE(PeekChar))
            {
                printf("%c", PeekChar); /* strip ical escape-codes */
                Parser->I += 2;
            }
            else if(Buffer->Data[Parser->I] == '"')
            {
                printf("\"\"");
                ++Parser->I;
            }
            else
            {
                printf("%c", Buffer->Data[Parser->I]);
                ++Parser->I;
            }
        }
    }
    printf("\",\n");
    return Range;
}

static void ParseAssign(buffer *Buffer, parser *Parser)
{
    if(Parser->InAssignment)
    {
        s32 I;
        for (I = 0; I <= IdentifierCacheCount; ++I)
        {
            printf("%c", IDENTIFIER_CACHE[I]);
        }
        printf(",");
    }
    else
    {
        Parser->InAssignment = 1;
    }
    printf("Assign,");
    ParseIdentifier(Buffer, Parser);
    /* TODO: figure out if we need ot handle space character between identifers, assignment
     * operator, and data. We don't currently handle that kind of spacing, so it would need to
     * be added. */
    if(Parser->I < Buffer->Size && Buffer->Data[Parser->I] == '=')
    {
        ++Parser->I;
        ParseData(Buffer, Parser);
    }
    else
    {
        printf("ParseAssign expected ';'\n");
        Parser->State = parser_state_Error;
    }
}

static void ParseICal(buffer *Buffer)
{
    /* NOTE: try to tokenize and parse all in one go for now */
    u8 Char;
    parser Parser = CreateParser();
    Parser.State = parser_state_Root;
    printf("Binding Name,Binding Kind,Assign Name,Value,\n");
    while(Parser.I < Buffer->Size)
    {
        SkipSpace(&Parser, Buffer);
        Char = Buffer->Data[Parser.I];
        switch(Parser.State)
        {
        case parser_state_Root:
            if(CHAR_IS_IDENT_START(Char))
            {
                ParseIdentifier(Buffer, &Parser);
            }
            else
            {
                Parser.State = parser_state_Error;
            }
            break;
        case parser_state_BindOrAssign:
            if(Char == ':')
            {
                printf("Bind,,");
                Parser.State = parser_state_Data;
                ++Parser.I;
            }
            else if(Char == ';')
            {
                /* printf("Assign,"); */
                Parser.State = parser_state_Assign;
                ++Parser.I;
            }
            break;
        case parser_state_Assign:
            ParseAssign(Buffer, &Parser);
            break;
        case parser_state_Data:
            ParseData(Buffer, &Parser);
            break;
        default:
            printf("ParseICal default error %d\n", Parser.State);
            Parser.State = parser_state_Error;
            return;
        }
    }
}
#endif

static void ExpectChar(parser *Parser, buffer *Buffer, u8 Char)
{

}

static void ParseName(parser *Parser, buffer *Buffer)
{

}

static void ParseParam(parser *Parser, buffer *Buffer)
{

}

static void ParseParams(parser *Parser, buffer *Buffer)
{
    /* ";" Param */
}

static void ParseValue(parser *Parser, buffer *Buffer)
{

}

static void ParseCRLF(parser *Parser, buffer *Buffer)
{

}

static void ParseContentLine(parser *Parser, buffer *Buffer)
{
    /* Name *Params ":" Value CRLF */
    ParseName(Parser, Buffer);
    ParseParams(Parser, Buffer);
    ExpectChar(Parser, Buffer, ':');
    ParseValue(Parser, Buffer);
    ParseCRLF(Parser, Buffer);
}

static void ParseICal(buffer *Buffer)
{
    parser Parser = CreateParser();
    while(Parser.I < Buffer->Size)
    {
        switch(Parser.State)
        {
        case parser_state_ContentLine:
            ParseContentLine(&Parser, Buffer);
            break;
        default:
            break;
        }
    }
}

static void TestParseICal()
{
    /* buffer *Buffer = ReadFileIntoBuffer("./__test.ics"); */
    buffer *Buffer = ReadFileIntoBuffer("./__test2.ics");
    /* buffer *Buffer = ReadFileIntoBuffer("./__test_Meeting_After_School.ics"); */
    ParseICal(Buffer);
    FreeBuffer(Buffer);
}


int main()
{
    TestParseICal();
}
