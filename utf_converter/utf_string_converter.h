#ifndef UTF_STRING_CONVERTER_H
#define UTF_STRING_CONVERTER_H

#include <cstdint>
#include <vector>

typedef uint8_t utf8_char;
typedef std::vector<utf8_char> utf8_string;
typedef uint32_t utf32_char;
typedef std::vector<utf32_char> utf32_string;

namespace utf_string_converter {
    utf32_string to_utf32(const utf8_string& src);
    utf8_string to_utf8(const utf32_string& src);
}

#endif /* ifndef UTF_STRING_CONVERTER_H */
