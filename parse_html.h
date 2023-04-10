#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int32_t s32;

typedef uint8_t u8;
typedef uint32_t u32;

#define ArrayCount(a) (sizeof(a) / sizeof(a[0]))

typedef struct
{
    s32 Count;
    u8 *Data;
} buffer;

/* NOTE: ensure that HTML_STATE_COUNT is equal to the number of values in the html_state enum */
typedef enum
{
    html_state_Error = 0,
    html_state_Success,
    html_state_Root,
    html_state_TagHead,
    html_state_TagHeadName,
    html_state_TagHeadContent,
    html_state_CommentHead,
    html_state_CommentBody,
} html_state;
/* TODO: loop through transition array and determine HTML_STATE_COUNT */
#define HTML_STATE_COUNT 16 /* NOTE: this also includes all states from transition sequences */

typedef enum
{
    html_transition_kind_Single,
    html_transition_kind_Set,
    html_transition_kind_Range,
    html_transition_kind_Sequence,
    html_transition_kind_NotSequence,
} html_transition_kind;

#define RANGE_COUNT 2
typedef struct
{
    s32 CurrentState;
    html_transition_kind Kind;
    u8 CharA;
    u8 CharB;
    u32 Count;
    u8 *Set;
    s32 NextState;
} html_transition_entry;
