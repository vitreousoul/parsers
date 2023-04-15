/* https://www.rfc-editor.org/rfc/rfc3629.txt */
#include "parse_ical.h"

#define CHAR_IS_LOWER_CASE(char) (((char) >= 'a') && ((char) <= 'z'))
#define CHAR_IS_UPPER_CASE(char) (((char) >= 'A') && ((char) <= 'Z'))
#define CHAR_IS_ALPHA(char) (CHAR_IS_LOWER_CASE(char) || CHAR_IS_UPPER_CASE(char))
#define CHAR_IS_DIGIT(char) (((char) >= '0') && ((char) <= '9'))
#define CHAR_IS_ALPHANUM(char) (CHAR_IS_ALPHA(char) || CHAR_IS_DIGIT(char))
#define CHAR_IS_SPACE(char) (((char) == ' ') || ((char) == '\t'))
#define CHAR_IS_IANA_CHAR(char) (CHAR_IS_ALPHANUM(char) || ((char) == '-'))
#define CHAR_IS_NON_USASCII(char) (((char) == 0xE0) ||                  \
                                   ((char) == 0xED) ||                  \
                                   ((char) >= 0xC2 && (char) <= 0xDF) || \
                                   ((char) >= 0xE1 && (char) <= 0xEC) || \
                                   ((char) >= 0xEE && (char) <= 0xEF))
#define CHAR_IS_Q_SAFE_STRING(char) (CHAR_IS_SPACE(char) ||             \
                                     (char) >= 0x21 ||                  \
                                     ((char) >= 0x23 && (char) <= 0x7E) || \
                                     CHAR_IS_NON_USASCII(char))

#define CHAR_IS_SAFE_CHAR(char) (CHAR_IS_SPACE(char) ||                 \
                                 (char) >= 0x21 ||                      \
                                 ((char) >= 0x23 && (char) <= 0x2B) ||  \
                                 ((char) >= 0x2D && (char) <= 0x39) ||  \
                                 ((char) >= 0x3C && (char) <= 0x7E) ||  \
                                 CHAR_IS_NON_USASCII(char))

#define CHAR_IS_VALUE(char) (CHAR_IS_SPACE(char) ||                 \
                                 ((char) >= 0x21 && (char) <= 0x7E) ||  \
                                 CHAR_IS_NON_USASCII(char))

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
        printf("Expected char (%d)\n", Char);
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
            printf("%c", Buffer->Data[Parser->I]);
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
    printf("X-");
    b32 VendorId1 = CHAR_IS_ALPHANUM(Buffer->Data[Parser->I]);
    b32 VendorId2 = CHAR_IS_ALPHANUM(PEEK(Buffer, Parser));
    b32 VendorId3 = CHAR_IS_ALPHANUM(PEEK2(Buffer, Parser));
    b32 VendorId4 = PEEK3(Buffer, Parser) == '-';
    b32 IsVendorId = VendorId1 && VendorId2 && VendorId3 && VendorId4;
    if(IsVendorId)
    {
        printf("%c%c%c%c", Buffer->Data[Parser->I], Buffer->Data[Parser->I+1], Buffer->Data[Parser->I+2], Buffer->Data[Parser->I+3]);
        Parser->I += 4;
    }
    ParseIanaToken(Parser, Buffer);
    printf("\n");
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
    printf("\n");
}
static void ParseQuotedString(parser *Parser, buffer *Buffer)
{
    /*
      QuotedString = "\"" *QsafeChar "\""
      QsafeChar    = WSP / %x21 / %x23-7E / NonUsAscii
    */
    ExpectChar(Parser, Buffer, '"');
    for(;;)
    {
        if(CHAR_IS_Q_SAFE_STRING(Buffer->Data[Parser->I]))
        {
            ++Parser->I;
        }
        else
        {
            break;
        }
    }
    ExpectChar(Parser, Buffer, '"');
}

static void ParamText(parser *Parser, buffer *Buffer)
{
    /*
      ParamText    = *SafeChar
      SafeChar     = WSP / %x21 / %x23-2B / %x2D-39 / %x3C-7E / NonUsAscii
    */
    for(;;)
    {
        if(CHAR_IS_SAFE_CHAR(Buffer->Data[Parser->I]))
        {
            ++Parser->I;
        }
        else
        {
            break;
        }
    }
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
        if(CHAR_IS_VALUE(Buffer->Data[Parser->I]))
        {
            ++Parser->I;
        }
        else
        {
            break;
        }
    }
}

static void ParseCRLF(parser *Parser, buffer *Buffer)
{
    u8 Char = Buffer->Data[Parser->I];
    if(Char == char_code_CR)
    {
        ExpectChar(Parser, Buffer, char_code_CR);
        ExpectChar(Parser, Buffer, char_code_LF);
    }
    else if(Char == '\n')
    {
        ++Parser->I;
    }
    else
    {
        printf("[ Error ] expected newline sequence\n");
    }
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

#define ERROR_BACK_BUFFER_COUNT 64
static void ParseICal(buffer *Buffer)
{
    b32 Running = 1;
    parser Parser = CreateParser();
    char ErrorChars[ERROR_BACK_BUFFER_COUNT];
    s32 ErrorIndex;
    Parser.State = parser_state_ContentLine;
    while(Running && Parser.I < Buffer->Size)
    {
        switch(Parser.State)
        {
        case parser_state_ContentLine:
            ParseContentLine(&Parser, Buffer);
            break;
        case parser_state_Error:
            ErrorIndex = Parser.I - ERROR_BACK_BUFFER_COUNT > 0 ? Parser.I - ERROR_BACK_BUFFER_COUNT : 0;
            memcpy(ErrorChars, &Buffer->Data[Parser.I-ERROR_BACK_BUFFER_COUNT], Parser.I - ErrorIndex);
            ErrorChars[ERROR_BACK_BUFFER_COUNT-1] = 0;
            printf("%s\n", ErrorChars);
            Running = 0;
            break;
        default:
            Running = 0;
            break;
        }
    }
}

static void RemoveLineContinuations(buffer *Buffer)
{
    if(!Buffer) return;
    printf("RemoveLineContinuations\n");
    s32 BufferIndex, WriteIndex = 0, ContinuationCount = 0;
    for(BufferIndex = 0; BufferIndex < Buffer->Size; ++BufferIndex)
    {
        s32 RemainingCharCount = (Buffer->Size - 1) - BufferIndex;
        if(RemainingCharCount >= 3)
        {
            u8 Char1 = Buffer->Data[BufferIndex];
            u8 Char2 = Buffer->Data[BufferIndex+1];
            u8 Char3 = Buffer->Data[BufferIndex+2];
            b32 NewlineSpace = Char1 == char_code_Newline && CHAR_IS_SPACE(Char2);
            b32 CRLFSpace = Char1 == char_code_CR && Char2 == char_code_LF && CHAR_IS_SPACE(Char3);
            if(NewlineSpace || CRLFSpace)
            {
                BufferIndex += 3;
                ++ContinuationCount;
            }
        }
        Buffer->Data[WriteIndex] = Buffer->Data[BufferIndex];
        ++WriteIndex;
    }
    Buffer->Size -= 3 * ContinuationCount;
}

static void TestParseICal()
{
    char *FilePath = "./__test2.ics";
    buffer *Buffer = ReadFileIntoBuffer(FilePath);
    if(Buffer)
    {
        printf("BufferSize %d\n", Buffer->Size);
        RemoveLineContinuations(Buffer);
        printf("BufferSize %d\n", Buffer->Size);
        /* for(I = 0; I < Buffer->Size; ++I) printf("%c", Buffer->Data[I]); */
        /* printf("\n"); */
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
