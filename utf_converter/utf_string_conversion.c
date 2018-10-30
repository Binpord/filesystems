#include "utf_string_conversion.h"

const size_t NPOS = (size_t)-1;

const size_t MAX_UTF8_LEN = 4;
const utf8_char_t UTF8_MASKS[] = {0x80, 0xE0, 0xF0, 0xF8};
const utf8_char_t UTF8_BITS[]  = {0x00, 0xC0, 0xE0, 0xF0};
const utf8_char_t UTF8_SUBSEQ_MASK = 0xC0;
const utf8_char_t UTF8_SUBSEQ_BITS = 0x80;

size_t utf8_char_len(const utf8_char_t ch) {
    for (size_t len = 0; len < MAX_UTF8_LEN; len++) {
        if ((ch & UTF8_MASKS[len]) == UTF8_BITS[len])
            return len + 1;
    }

    if (ch)
        errno = EILSEQ;
    return 0;
}

size_t check_subseq_chars(const utf8_char_t *src, size_t i, size_t len) {
    for (size_t ch_pos = i + 1; ch_pos < i + len; ch_pos++) {
        if ((src[ch_pos] & UTF8_SUBSEQ_MASK) != UTF8_SUBSEQ_BITS) {
            errno = EILSEQ;
            return NPOS;
        }
    }
    return i + len;
}

size_t get_utf32_char(const utf8_char_t *src, size_t i, utf32_char_t *dst, size_t j) {
    size_t len = utf8_char_len(src[i]);
    if (!len || check_subseq_chars(src, i, len) == NPOS) {
        return NPOS;
    } else if (dst) {
        dst[j] = (utf32_char_t)(src[i] & ~UTF8_MASKS[len - 1]);
        for (size_t k = i + 1; k < i + len; k++) {
            dst[j] = (dst[j] << 6) | (utf32_char_t)(src[k] & ~UTF8_SUBSEQ_MASK);
        }
    }
    return i + len;
}

size_t to_utf32(const utf8_char_t *src, utf32_char_t *dst) {
    size_t i = 0, j = 0;
    if (src) {
        while (src[i]) {
            i = get_utf32_char(src, i, dst, j++);
            if (i == NPOS)
                return NPOS;
        }
        if (dst) {
            dst[j] = 0;
        }
    }
    return j;
}

size_t utf32_char_len(const utf32_char_t ch) {
    if (ch < (1<<7)) {
        return 1;
    } else if (ch < (1<<11)) {
        return 2;
    } else if (ch < (1<<16)) {
        return 3;
    } else if (ch < (1<<21)) {
        return 4;
    }
    errno = EINVAL;
    return 0;
}

size_t get_utf8_char(const utf32_char_t *src, size_t i, utf8_char_t *dst, size_t j) {
    size_t len = utf32_char_len(src[i]);
    if (!len) {
        return NPOS;
    } else if (dst) {
        size_t subseq_chars = len - 1;
        dst[j] = (src[i] >> (6 * subseq_chars)) & 0x7F;
        dst[j] |= UTF8_BITS[len - 1];
        for (size_t c = 1; c < len; c++) {
            dst[j + c] = (src[i] >> (6 * (subseq_chars - c))) & 0x3F;
            dst[j + c] |= UTF8_SUBSEQ_BITS;
        }
    }
    return j + len;
}

size_t to_utf8(const utf32_char_t *src, utf8_char_t *dst) {
    size_t i = 0, j = 0;
    if (src) {
        while (src[i]) {
            j = get_utf8_char(src, i++, dst, j);
            if (j == NPOS)
                return NPOS;
        }
        if (dst) {
            dst[j] = 0;
        }
    }
    return j;
}

