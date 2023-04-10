#include "parse_html.h"

#define UTF8_ALPHABET_COUNT (1 << 8)
s32 TRANSITION_TABLE[HTML_STATE_COUNT][UTF8_ALPHABET_COUNT];

#define TAG_NAME_CHAR_COUNT 53
u8 TAG_NAME_CHAR[TAG_NAME_CHAR_COUNT] = {
    'a','b','c','d','e','f','g',
    'h','i','j','k','l','m','n','o','p',
    'q','r','s','t','u','v',
    'w','x','y','z',
    'A','B','C','D','E','F','G',
    'H','I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V',
    'W','X','Y','Z',
    '-',
};

u8 CommentHeadSequence[] = "--";
u8 CommentTailSequence[] = "-->";

html_transition_entry Transitions[] = {
    /* html_state_Root */
    {html_state_Root,html_transition_kind_Single,'<',0,0,0,html_state_TagHead},
    /* html_state_TagHead */
    {html_state_TagHead,html_transition_kind_Single,'!',0,0,0,html_state_CommentHead},
    {html_state_TagHead,html_transition_kind_Set,0,0,TAG_NAME_CHAR_COUNT,TAG_NAME_CHAR,html_state_TagHeadName},
    /* html_state_TagHeadName */
    /* NOTE: we probably don't need transitions to html_state_Error since the value is 0 and the table is zeroed */
    /* {html_state_TagHeadName,html_transition_kind_Range,0,UTF8_ALPHABET_COUNT-1,0,0,html_state_Error}, */
    {html_state_TagHeadName,html_transition_kind_Single,'>',0,0,0,html_state_Root},
    {html_state_TagHeadName,html_transition_kind_Set,0,0,TAG_NAME_CHAR_COUNT,TAG_NAME_CHAR,html_state_TagHeadName},
    /* html_state_CommentHead */
    {html_state_CommentHead,html_transition_kind_Sequence,0,0,2,CommentHeadSequence,html_state_CommentBody},
    /* html_state_CommentBody */
    {html_state_CommentBody,html_transition_kind_NotSequence,0,0,3,CommentTailSequence,html_state_Root},
};

static buffer *ReadFileIntoBuffer(char *FilePath)
{
    FILE *File = fopen(FilePath, "rb");
    s32 FileSize;
    buffer *Result;
    u8 *Data;
    if(!File)
    {
        return 0;
    }
    fseek(File, 0, SEEK_END);
    FileSize = ftell(File);
    Data = malloc(FileSize);
    Result = malloc(sizeof(buffer));
    Result->Count = FileSize;
    Result->Data = Data;
    fseek(File, 0, SEEK_SET);
    fread(Result->Data, sizeof(*Result->Data), FileSize, File);
    fclose(File);
    return Result;
}

static char *DebugPrintHtmlState(html_state State)
{
    switch (State)
    {
    case html_state_Error: return "html_state_Error";
    case html_state_Success: return "html_state_Success";
    case html_state_Root: return "html_state_Root";
    case html_state_TagHead: return "html_state_TagHead";
    case html_state_TagHeadName: return "html_state_TagHeadName";
    case html_state_TagHeadContent: return "html_state_TagHeadContent";
    case html_state_CommentHead: return "html_state_CommentHead";
    case html_state_CommentBody: return "html_state_CommentBody";
    default: return "<generated-state>";
    }
}

static void PopulateTransitionTable()
{
    u32 I, J, K;
    u8 C;
    u32 TransitionCount = ArrayCount(Transitions);
    s32 PreviousNextState, SequenceIndex = HTML_STATE_COUNT - 1;
    for (I = 0; I < TransitionCount; ++I)
    {
        html_transition_entry Transition = Transitions[I];
        switch (Transition.Kind)
        {
        case html_transition_kind_Single:
            TRANSITION_TABLE[Transition.CurrentState][Transition.CharA] = Transition.NextState;
            break;
        case html_transition_kind_Set:
            for (J = 0; J < Transition.Count; ++J)
            {
                TRANSITION_TABLE[Transition.CurrentState][Transition.Set[J]] = Transition.NextState;
            }
            break;
        case html_transition_kind_Range:
            for (C = Transition.CharA; C < Transition.CharB; ++C)
            {
                TRANSITION_TABLE[Transition.CurrentState][C] = Transition.NextState;
            }
            break;
        case html_transition_kind_Sequence:
            J = 0;
            PreviousNextState = 0;
            do
            {
                s32 NotFirst = J > 0;
                s32 NotLast = J < Transition.Count - 1;
                s32 CurrentState = Transition.CurrentState;
                if (NotFirst)
                {
                    CurrentState = PreviousNextState;
                }
                s32 NextState = Transition.NextState;
                if (NotLast)
                {
                    NextState = SequenceIndex;
                    PreviousNextState = NextState;
                    --SequenceIndex;
                }

                printf("%s %c %s\n", DebugPrintHtmlState(CurrentState), Transition.Set[J], DebugPrintHtmlState(NextState));
                TRANSITION_TABLE[CurrentState][Transition.Set[J]] = NextState;
                ++J;
            } while(J < Transition.Count);
            break;
        case html_transition_kind_NotSequence:
            J = 0;
            PreviousNextState = 0;
            for (K = 0; K < UTF8_ALPHABET_COUNT; ++K)
            {
                TRANSITION_TABLE[Transition.CurrentState][K] = Transition.CurrentState;
            }
            do
            {
                s32 NotFirst = J > 0;
                s32 NotLast = J < Transition.Count;
                s32 CurrentState = Transition.CurrentState;
                if (NotFirst)
                {
                    CurrentState = PreviousNextState;
                }
                s32 NextState = Transition.NextState;
                if (NotLast)
                {
                    NextState = SequenceIndex;
                    PreviousNextState = NextState;
                    --SequenceIndex;
                }
                TRANSITION_TABLE[CurrentState][Transition.Set[J]] = NextState;
                ++J;
            } while(J >= 0 && J < Transition.Count);
            break;
        }
    }
    printf("SequenceIndex %d\n", SequenceIndex);
}

s32 main()
{
    s32 I, Result = 0;
    buffer *Buffer = ReadFileIntoBuffer("__test.html");
    html_state State = html_state_Root;
    PopulateTransitionTable();
    for (I = 0; I < Buffer->Count; ++I)
    {
        State = TRANSITION_TABLE[State][Buffer->Data[I]];
        printf("%s %c\n", DebugPrintHtmlState(State), Buffer->Data[I]);
        if (State == html_state_Success || State == html_state_Error)
        {
            break;
        }
    }
    free(Buffer->Data);
    free(Buffer);
    printf("sizeof(TRANSITION_TABLE) %lu\n", sizeof(TRANSITION_TABLE));
    return Result;
}
