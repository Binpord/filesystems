#ifndef UTF_STRING_CONVERSION_H
#define UTF_STRING_CONVERSION_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t utf8_char_t;
typedef uint32_t utf32_char_t;

enum {
    E_IL_UTF8_CH = -1,
    E_IL_UTF8_SEQ = -2,
    E_IL_UTF32_CH = -3,
    E_NO_PTR = -4,
};

int to_utf32(const utf8_char_t *src, utf32_char_t *dst);
int to_utf8(const utf32_char_t *src, utf8_char_t *dst);
const char *e_to_str(int error);

#endif /* ifndef UTF_STRING_CONVERSION_H */
