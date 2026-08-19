#include <sys/socket.h>

#include <string.h>

#include "web_utils.h"

int web_send_all(int fd, const void *data, size_t length)
{
    const char *cursor = data;
    while (length > 0)
    {
        ssize_t sent = send(fd, cursor, length, 0);
        if (sent <= 0)
            return -1;
        cursor += sent;
        length -= (size_t)sent;
    }
    return 0;
}

size_t web_json_append(char *out, size_t at, size_t capacity, const char *text)
{
    const unsigned char *cursor = (const unsigned char *)text;

    while (*cursor && at + 2 < capacity)
    {
        if (*cursor == '"' || *cursor == '\\')
        {
            out[at++] = '\\';
            out[at++] = (char)*cursor++;
        }
        else if (*cursor == '\n' || *cursor == '\r')
        {
            out[at++] = '\\';
            out[at++] = *cursor++ == '\n' ? 'n' : 'r';
        }
        else if (*cursor < 0x20)
        {
            cursor++;
        }
        else
        {
            out[at++] = (char)*cursor++;
        }
    }
    out[at] = '\0';
    return at;
}

size_t web_json_append_bytes(char *out, size_t at, size_t capacity, const unsigned char *data,
                             size_t length)
{
    size_t i = 0;
    while (i < length && at + 2 < capacity)
    {
        unsigned char value = data[i];
        if (value == '"' || value == '\\')
        {
            out[at++] = '\\';
            out[at++] = (char)value;
            i++;
        }
        else if (value == '\n' || value == '\r' || value == '\t')
        {
            out[at++] = '\\';
            out[at++] = value == '\n' ? 'n' : value == '\r' ? 'r' : 't';
            i++;
        }
        else if (value < 0x20)
        {
            out[at++] = '?';
            i++;
        }
        else if (value < 0x80)
        {
            out[at++] = (char)value;
            i++;
        }
        else
        {
            size_t sequence_length = 0;
            if (value >= 0xc2 && value <= 0xdf && i + 1 < length && data[i + 1] >= 0x80 &&
                data[i + 1] <= 0xbf)
                sequence_length = 2;
            else if (value >= 0xe0 && value <= 0xef && i + 2 < length && data[i + 1] >= 0x80 &&
                     data[i + 1] <= 0xbf && data[i + 2] >= 0x80 && data[i + 2] <= 0xbf)
                sequence_length = 3;
            else if (value >= 0xf0 && value <= 0xf4 && i + 3 < length && data[i + 1] >= 0x80 &&
                     data[i + 1] <= 0xbf && data[i + 2] >= 0x80 && data[i + 2] <= 0xbf &&
                     data[i + 3] >= 0x80 && data[i + 3] <= 0xbf)
                sequence_length = 4;
            if (sequence_length != 0 && at + sequence_length < capacity)
            {
                memcpy(out + at, data + i, sequence_length);
                at += sequence_length;
                i += sequence_length;
            }
            else
            {
                out[at++] = '?';
                i++;
            }
        }
    }
    out[at] = '\0';
    return at;
}
