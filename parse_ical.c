/* https://www.rfc-editor.org/rfc/rfc3629.txt */
#include "parse_ical.h"

#define CHAR_IS_LOWER_CASE(char) (((char) >= 'a') && ((char) <= 'z'))
#define CHAR_IS_UPPER_CASE(char) (((char) >= 'A') && ((char) <= 'Z'))
#define CHAR_IS_ALPHA(char) (CHAR_IS_LOWER_CASE(char) || CHAR_IS_UPPER_CASE(char))
#define CHAR_IS_DIGIT(char) (((char) >= '0') && ((char) <= '9'))
#define CHAR_IS_ALPHANUM(char) (CHAR_IS_ALPHA(char) || CHAR_IS_DIGIT(char))
#define CHAR_IS_SPACE(char) (((char) == ' ') || ((char) == '\t'))
#define CHAR_IS_IANA_CHAR(char) (CHAR_IS_ALPHANUM(char) || ((char) == '"'))

#define CHAR_IS_NON_USASCII(char) (((char) == 0xE0) ||                  \
                                   ((char) == 0xED) ||                  \
                                   ((char) >= 0xC2 && (char) <= 0xDF) || \
                                   ((char) >= 0xE1 && (char) <= 0xEC) || \
                                   ((char) >= 0xEE && (char) <= 0xEF))


#define PEEK(buffer, parser) ((parser)->I+1 < (buffer)->Size ? (buffer)->Data[(parser)->I+1] : 0)
#define PEEK2(buffer, parser) ((parser)->I+2 < (buffer)->Size ? (buffer)->Data[(parser)->I+2] : 0)
#define PEEK3(buffer, parser) ((parser)->I+3 < (buffer)->Size ? (buffer)->Data[(parser)->I+3] : 0)

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
    fclose(File);
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

static void ExpectChar(parser *Parser, buffer *Buffer, u8 Char)
{
    if(Buffer->Data[Parser->I] == Char)
    {
        ++Parser->I;
    }
    else
    {
        printf("Expected char '%c'\n", Char);
        Parser->State = parser_state_Error;
    }
}

static void ParseIanaToken(parser *Parser, buffer *Buffer)
{
    /*
      IanaToken    = 1*IanaChar
      IanaChar     = Alpha / Digit / "-"
    */
    for(;;)
    {
        if(CHAR_IS_IANA_CHAR(Buffer->Data[Parser->I]))
        {
            ++Parser->I;
        }
        else
        {
            break;
        }
    }
}

static void ParseXName(parser *Parser, buffer *Buffer)
{
    /*
      XName        = "X-" [VendorId "-"] 1*IanaChar
      VendorId     = 3*AlphaNum

    */
    /* NOTE: assume we has already parsed "X-" */
    b32 VendorId1 = CHAR_IS_ALPHANUM(Buffer->Data[Parser->I]);
    b32 VendorId2 = CHAR_IS_ALPHANUM(PEEK(Buffer, Parser));
    b32 VendorId3 = CHAR_IS_ALPHANUM(PEEK2(Buffer, Parser));
    b32 VendorId4 = PEEK3(Buffer, Parser) == '-';
    b32 IsVendorId = VendorId1 | VendorId2 | VendorId3 | VendorId4;
    if(IsVendorId)
    {
        Parser->I += 4;
    }
    ParseIanaToken(Parser, Buffer);
}

static void ParseName(parser *Parser, buffer *Buffer)
{
    /* IanaToken / XName */
    u8 PeekChar = PEEK(Buffer, Parser);
    if(Buffer->Data[Parser->I] == 'X' && PeekChar == '-')
    {
        Parser->I += 2;
        ParseXName(Parser, Buffer);
    }
    else
    {
        ParseIanaToken(Parser, Buffer);
    }
}
static void ParseQuotedString(parser *Parser, buffer *Buffer)
{

}

static void ParamText(parser *Parser, buffer *Buffer)
{

}

static void ParseParamValue(parser *Parser, buffer *Buffer)
{
    /*
      ParamValue   = ParamText / QuotedString
      ParamText    = *SafeChar
      SafeChar     = WSP / %x21 / %x23-2B / %x2D-39 / %x3C-7E / NonUsAscii
      QuotedString = "\"" *QsafeChar "\""
    */
    if(Buffer->Data[Parser->I] == '"')
    {
        ParseQuotedString(Parser, Buffer);
    }
    else
    {
        ParamText(Parser, Buffer);
    }
}

static void ParseParamRest(parser *Parser, buffer *Buffer)
{
    /* "," ParamValue */
    for(;;)
    {
        if(Buffer->Data[Parser->I] == ',')
        {
            ++Parser->I;
            ParseParamValue(Parser, Buffer);
        }
        else
        {
            break;
        }
    }
}

static void ParseParam(parser *Parser, buffer *Buffer)
{
    /* Name "=" ParamValue *ParamRest */
    ParseName(Parser, Buffer);
    ExpectChar(Parser, Buffer, '=');
    ParseParamValue(Parser, Buffer);
    ParseParamRest(Parser, Buffer);
}

static void ParseParams(parser *Parser, buffer *Buffer)
{
    /* *(";" Param) */
    for(;;)
    {
        if(Buffer->Data[Parser->I] == ';')
        {
            ++Parser->I;
            ParseParam(Parser, Buffer);
        }
        else
        {
            break;
        }
    }
}

static void ParseNonUsAscii(parser *Parser, buffer *Buffer)
{
    /* TODO: this might need to be rolled into other functions... */
    /*
      NonUsAscii  = UTF8-2 / UTF8-3 / UTF8-4

      UTF8-2      = %xC2-DF UTF8-tail

      UTF8-3      = %xE0 %xA0-BF UTF8-tail / %xE1-EC 2( UTF8-tail ) /
      %xED %x80-9F UTF8-tail / %xEE-EF 2( UTF8-tail )

      FIRST(NonUsAscii) = %xC2-DF / %xE0 / %xE1-EC / %xED / %xEE-EF
    */
    u8 Char = Buffer->Data[Parser->I];
    b32 IsC2ToDF = Char >= 0xC2 && Char <= 0xDF;
    b32 IsE0 = Char == 0xE0;
    b32 IsE1ToEC = Char >= 0xE1 && Char <= 0xEC;
    b32 IsED = Char == 0xED;
    b32 IsEEToEF = Char >= 0xEE && Char <= 0xEF;
    if(IsC2ToDF | IsE0 | IsE1ToEC | IsED | IsEEToEF)
    {
        ++Parser->I;
    }
    else
    {
    }
}

static void ParseValue(parser *Parser, buffer *Buffer)
{
    /*
      Value        = *ValueChar
      ValueChar    = WSP / %x21-7E / NonUsAscii
      FIRST(ValueChar) = ' ' / '\t' / %x21-7E / FIRST(NonUsAscii)
      FIRST(NonUsAscii) = %xC2-DF / %xE0 / %xE1-EC / %xED / %xEE-EF
    */
    for(;;)
    {
        u8 Char = Buffer->Data[Parser->I];
        b32 IsSpace = CHAR_IS_SPACE(Char);
        b32 IsNonUsAscii = CHAR_IS_NON_USASCII(Char);
    }
}

static void ParseCRLF(parser *Parser, buffer *Buffer)
{
    ExpectChar(Parser, Buffer, char_code_CR);
    ExpectChar(Parser, Buffer, char_code_LF);
}

static void ParseContentLine(parser *Parser, buffer *Buffer)
{
    /* Name Params ":" Value CRLF */
    ParseName(Parser, Buffer);
    ParseParams(Parser, Buffer);
    ExpectChar(Parser, Buffer, ':');
    ParseValue(Parser, Buffer);
    ParseCRLF(Parser, Buffer);
}

static void ParseICal(buffer *Buffer)
{
    b32 Running = 1;
    parser Parser = CreateParser();
    Parser.State = parser_state_ContentLine;
    while(Running && Parser.I < Buffer->Size)
    {
        printf("[ ParserState ] %s\n", DebugParserState(Parser.State));
        switch(Parser.State)
        {
        case parser_state_ContentLine:
            ParseContentLine(&Parser, Buffer);
            break;
        case parser_state_Error:
            Running = 0;
            break;
        default:
            Running = 0;
            break;
        }
    }
}

static void TestParseICal()
{
    char *FilePath = "./__test.ics";
    buffer *Buffer = ReadFileIntoBuffer(FilePath);
    if(Buffer)
    {
        ParseICal(Buffer);
        FreeBuffer(Buffer);
    }
    else
    {
        printf("File \"%s\" not found\n", FilePath);
    }
}


int main()
{
    TestParseICal();
}
