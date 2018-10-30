#ifndef UTF_STRING_CONVERSION_H
#define UTF_STRING_CONVERSION_H

#include <stdint.h>
#include <stddef.h>
#include <errno.h>

typedef uint8_t utf8_char_t;
typedef uint32_t utf32_char_t;

extern const size_t NPOS;

size_t to_utf32(const utf8_char_t *src, utf32_char_t *dst);
size_t to_utf8(const utf32_char_t *src, utf8_char_t *dst);

#endif /* ifndef UTF_STRING_CONVERSION_H */
