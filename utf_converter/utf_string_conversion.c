#include "utf_string_conversion.h"

static const int MAX_UTF8_LEN = 4;
static const utf8_char_t UTF8_MASKS[] = {0x80, 0xE0, 0xF0, 0xF8};
static const utf8_char_t UTF8_BITS[]  = {0x00, 0xC0, 0xE0, 0xF0};
static const utf8_char_t UTF8_SUBSEQUENT_MASK = 0xC0;
static const utf8_char_t UTF8_SUBSEQUENT_BITS = 0x80;

static int utf8_char_len(const utf8_char_t ch) {
    if (!ch)
        return 0;

    for (int len = 0; len < MAX_UTF8_LEN; len++) {
        if ((ch & UTF8_MASKS[len]) == UTF8_BITS[len])
            return len + 1;
    }
    return E_IL_UTF8_CH;
}

static int check_subsequent_chars(const utf8_char_t *src, int len) {
    for (int ch_pos = 1; ch_pos < len; ch_pos++) {
        if ((src[ch_pos] & UTF8_SUBSEQUENT_MASK) != UTF8_SUBSEQUENT_BITS) {
            return E_IL_UTF8_SEQ;
        }
    }
    return len;
}

static int get_utf32_char(const utf8_char_t *src, utf32_char_t *dst, int j) {
    int len = utf8_char_len(*src);
    len = check_subsequent_chars(src, len);
    if (len > 0 && dst) {
        dst[j] = (utf32_char_t)(*src & ~UTF8_MASKS[len - 1]);
        for (int i = 1; i < len; i++) {
            dst[j] = (dst[j] << 6) | (utf32_char_t)(src[i] & ~UTF8_SUBSEQUENT_MASK);
        }
    }
    return len;
}

int to_utf32(const utf8_char_t *src, utf32_char_t *dst) {
    if (!src) {
        return E_NO_PTR;
    }

    int i = 0, j = 0;
    while (src[i]) {
        int len = get_utf32_char(&src[i], dst, j++);
        if (len < 0)
            return len;
        else
            i += len;
    }
    if (dst) {
        dst[j] = 0;
    }
    return j;
}

static int utf32_char_len(const utf32_char_t ch) {
    if (ch < (1<<7)) {
        return 1;
    } else if (ch < (1<<11)) {
        return 2;
    } else if (ch < (1<<16)) {
        return 3;
    } else if (ch < (1<<21)) {
        return 4;
    }
    return E_IL_UTF32_CH;
}

static int get_utf8_char(const utf32_char_t *src, utf8_char_t *dst, int j) {
    int len = utf32_char_len(*src);
    if (len > 0 && dst) {
        int subsequent_chars = len - 1;
        dst[j] = (*src >> (6 * subsequent_chars)) & 0x7F;
        dst[j] |= UTF8_BITS[len - 1];
        for (int i = 1; i < len; i++) {
            dst[j + i] = (*src >> (6 * (subsequent_chars - i))) & 0x3F;
            dst[j + i] |= UTF8_SUBSEQUENT_BITS;
        }
    }
    return len;
}

int to_utf8(const utf32_char_t *src, utf8_char_t *dst) {
    if (!src) {
        return E_NO_PTR;
    }

    int i = 0, j = 0;
    while (src[i]) {
        int len = get_utf8_char(&src[i++], dst, j);
        if (len < 0)
            return len;
        else
            j += len;
    }
    if (dst) {
        dst[j] = 0;
    }
    return j;
}

const char *e_to_str(int e) {
    static const char E_IL_UTF8_CH_STR[] = "illegal UTF8 first char";
    static const char E_IL_UTF8_SEQ_STR[] = "illegal UTF8 subsequent char";
    static const char E_IL_UTF32_CH_STR[] = "illegal UTF32 char";
    static const char E_NO_PTR_STR[] = "no pointer provided";
    static const char DEFAULT_STR[] = "unknown error";
    switch (e) {
        case E_IL_UTF8_CH:
            return E_IL_UTF8_CH_STR;
        case E_IL_UTF8_SEQ:
            return E_IL_UTF8_SEQ_STR;
        case E_IL_UTF32_CH:
            return E_IL_UTF32_CH_STR;
        case E_NO_PTR:
            return E_NO_PTR_STR;
        default:
            return DEFAULT_STR;
    }
}

