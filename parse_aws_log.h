#include <stdlib.h>
#include <stdio.h>

typedef int32_t s32;

typedef uint8_t u8;

typedef struct
{
    s32 count;
    s32 index;
    u8 *data;
} Buffer;

Buffer *read_file_into_buffer(char *file_path);
void parse_aws_log(char *log_file_path);

Buffer *read_file_into_buffer(char *file_path)
{
    Buffer *buffer = 0;
    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        printf("Error opening file %s\n", file_path);
        return buffer;
    }
    buffer = malloc(sizeof(Buffer));
    fseek(file, 0, SEEK_END);
    buffer->count = ftell(file);
    buffer->data = malloc(buffer->count);
    fseek(file, 0, SEEK_SET);
    fread(buffer->data, 1, buffer->count, file);
    fclose(file);
    printf("file_size %d\n", buffer->count);
    return buffer;
}

/*
  https://www.w3.org/TR/NOTE-datetime

  Year:
  YYYY (eg 1997)
  Year and month:
  YYYY-MM (eg 1997-07)
  Complete date:
  YYYY-MM-DD (eg 1997-07-16)
  Complete date plus hours and minutes:
  YYYY-MM-DDThh:mmTZD (eg 1997-07-16T19:20+01:00)
  Complete date plus hours, minutes and seconds:
  YYYY-MM-DDThh:mm:ssTZD (eg 1997-07-16T19:20:30+01:00)
  Complete date plus hours, minutes, seconds and a decimal fraction of a second
  YYYY-MM-DDThh:mm:ss.sTZD (eg 1997-07-16T19:20:30.45+01:00)

  YYYY = four-digit year
  MM   = two-digit month (01=January, etc.)
  DD   = two-digit day of month (01 through 31)
  hh   = two digits of hour (00 through 23) (am/pm NOT allowed)
  mm   = two digits of minute (00 through 59)
  ss   = two digits of second (00 through 59)
  s    = one or more digits representing a decimal fraction of a second
  TZD  = time zone designator (Z or +hh:mm or -hh:mm)
*/
static void parse_date_time(Buffer *buffer)
{
    /* TODO: implement */
}

void parse_aws_log(char *log_file_path)
{
    Buffer *buffer = read_file_into_buffer(log_file_path);
    if (!buffer)
    {
        return;
    }
    s32 i;
    for (i = 0; i < buffer->count; ++i)
    {
    }
}
