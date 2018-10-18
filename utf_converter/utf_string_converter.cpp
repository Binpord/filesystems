#include "utf_string_converter.h"
#include <stdexcept>
#include <string>
#include <functional>
#include <iostream>
#include <array>
#include <cassert>

namespace {
    const utf32_char UTF8_ONE_BYTE_CHAR_MASK = 0x80;
    const utf32_char UTF8_ONE_BYTE_CHAR_BITS = 0x00;
    const utf32_char UTF8_TWO_BYTE_FIRST_CHAR_MASK = 0xE0;
    const utf32_char UTF8_TWO_BYTE_FIRST_CHAR_BITS = 0xC0;
    const utf32_char UTF8_THREE_BYTE_FIRST_CHAR_MASK = 0xF0;
    const utf32_char UTF8_THREE_BYTE_FIRST_CHAR_BITS = 0xE0;
    const utf32_char UTF8_FOUR_BYTE_FIRST_CHAR_MASK = 0xF8;
    const utf32_char UTF8_FOUR_BYTE_FIRST_CHAR_BITS = 0xF0;
    const utf32_char UTF8_SUBSEQUENT_BYTE_MASK = 0xC0;
    const utf32_char UTF8_SUBSEQUENT_BYTE_BITS = 0x80;
    const size_t MAX_UTF8_CHARS_IN_UTF32_CHAR = 4;

    size_t get_utf8_char_len(const utf8_string::const_iterator& it) {
        if ((*it & UTF8_ONE_BYTE_CHAR_MASK) == UTF8_ONE_BYTE_CHAR_BITS)
            return 1;
        else if ((*it & UTF8_TWO_BYTE_FIRST_CHAR_MASK) == UTF8_TWO_BYTE_FIRST_CHAR_BITS)
            return 2;
        else if ((*it & UTF8_THREE_BYTE_FIRST_CHAR_MASK) == UTF8_THREE_BYTE_FIRST_CHAR_BITS)
            return 3;
        else if ((*it & UTF8_FOUR_BYTE_FIRST_CHAR_MASK) == UTF8_FOUR_BYTE_FIRST_CHAR_BITS)
            return 4;

        throw std::domain_error(std::string("invalid UTF-8 symbol detected ") + std::to_string(*it));
    }

    utf32_char get_utf32_char(utf8_string::const_iterator& it, const utf32_char mask, const size_t char_len) {
        utf32_char result = (utf32_char)(*it) & ~mask;
        const size_t amount_of_subsequent_bytes = char_len - 1;
        for (size_t i = amount_of_subsequent_bytes; i > 0; --i) {
            ++it;
            if (((*it) & UTF8_SUBSEQUENT_BYTE_MASK) != UTF8_SUBSEQUENT_BYTE_BITS) {
                throw std::domain_error(std::string("invalid UTF-8 symbol detected ") + std::to_string(*it));
            }
            result = (result << 6) | ((utf32_char)(*it) & ~UTF8_SUBSEQUENT_BYTE_MASK);
        }
        return result;
    }

    utf32_char get_utf32_char(utf8_string::const_iterator& it, const size_t char_len) {
        switch (char_len) {
        case 1:
            return get_utf32_char(it, UTF8_ONE_BYTE_CHAR_MASK, char_len);
        case 2:
            return get_utf32_char(it, UTF8_TWO_BYTE_FIRST_CHAR_MASK, char_len);
        case 3:
            return get_utf32_char(it, UTF8_THREE_BYTE_FIRST_CHAR_MASK, char_len);
        case 4:
            return get_utf32_char(it, UTF8_FOUR_BYTE_FIRST_CHAR_MASK, char_len);
        }

        throw std::domain_error(std::string("invalid UTF-8 symbol detected ") + std::to_string(*it));
    }

    utf32_char get_utf32_char(utf8_string::const_iterator& it) {
        size_t char_len = get_utf8_char_len(it);
        return get_utf32_char(it, char_len);
    }

    utf8_char get_utf8_char_from_utf32_char(utf32_string::const_iterator& it, size_t char_num, size_t char_len) {
        static const size_t AMOUNT_OF_BITS_IN_UTF32_CHAR_FOR_SUBSEQENT_UTF8_CHAR = 6;
        const utf8_char UTF8_CHAR_MASK = char_len == 1 ? 0x7F : 0x3F;
        size_t amount_of_bits_before_our_char = (char_num - 1) * AMOUNT_OF_BITS_IN_UTF32_CHAR_FOR_SUBSEQENT_UTF8_CHAR;
        return (utf8_char)(((*it) >> amount_of_bits_before_our_char) & UTF8_CHAR_MASK);
    }

    template<typename T>
    size_t get_last_set_bit_pos(T num) {
        static const size_t AMOUNT_OF_BITS_IN_BYTE = 8;
        const size_t amount_of_bits_in_num = sizeof(num) * AMOUNT_OF_BITS_IN_BYTE;
        for (size_t last_set_bit_pos = 0; last_set_bit_pos <= amount_of_bits_in_num; last_set_bit_pos++) {
            if ((num >> last_set_bit_pos) == 0) {
                return last_set_bit_pos;
            }
        }

        throw std::length_error("amount of byte seems to be other than 8");
    }

    size_t get_utf8_char_len(utf32_string::const_iterator& it) {
        static const std::array<size_t, MAX_UTF8_CHARS_IN_UTF32_CHAR> max_amount_of_significant_bits_in_utf8_char = {7 , 11, 16, 21};
        size_t amount_of_significant_bits = get_last_set_bit_pos(*it);
        for (size_t amount_of_subsequent_chars = 0; amount_of_subsequent_chars < max_amount_of_significant_bits_in_utf8_char.size(); amount_of_subsequent_chars++) {
            if (amount_of_significant_bits <= max_amount_of_significant_bits_in_utf8_char[amount_of_subsequent_chars]) {
                return amount_of_subsequent_chars + 1;
            }
        }

        throw std::domain_error(std::string("invalid UTF-32 symbol detected ") + std::to_string(*it));
    }

    utf8_string get_utf8_char(utf32_string::const_iterator& it, const utf32_char bits, const size_t char_len) {
        utf8_string result(char_len);

        utf8_char first_char = get_utf8_char_from_utf32_char(it, char_len, char_len);
        if ((utf32_char)first_char & bits) {
            throw std::domain_error(std::string("invalid UTF-32 symbol detected ") + std::to_string(*it));
        }
        result[0] = first_char | bits;

        for (size_t subsequent_char_count = 1; subsequent_char_count < char_len; subsequent_char_count++) {
            size_t subsequent_char_num = char_len - subsequent_char_count;
            utf8_char subsequent_char = get_utf8_char_from_utf32_char(it, subsequent_char_num, char_len);
            if ((utf32_char)subsequent_char & UTF8_SUBSEQUENT_BYTE_BITS) {
                throw std::domain_error(std::string("invalid UTF-32 symbol detected ") + std::to_string(*it));
            }
            result[subsequent_char_count] = subsequent_char | UTF8_SUBSEQUENT_BYTE_BITS;
        }

        return result;
    }

    utf8_string get_utf8_char(utf32_string::const_iterator& it, const size_t char_len) {
        switch (char_len) {
        case 1:
            return get_utf8_char(it, UTF8_ONE_BYTE_CHAR_BITS, char_len);
        case 2:
            return get_utf8_char(it, UTF8_TWO_BYTE_FIRST_CHAR_BITS, char_len);
        case 3:
            return get_utf8_char(it, UTF8_THREE_BYTE_FIRST_CHAR_BITS, char_len);
        case 4:
            return get_utf8_char(it, UTF8_FOUR_BYTE_FIRST_CHAR_BITS, char_len);
        }

        throw std::domain_error(std::string("invalid UTF-32 symbol detected ") + std::to_string(*it));
    }

    utf8_string get_utf8_char(utf32_string::const_iterator& it) {
        const size_t char_len = get_utf8_char_len(it);
        return get_utf8_char(it, char_len);
    }
} // namespace

namespace utf_string_converter {
    utf32_string to_utf32(const utf8_string& src) {
        utf32_string res;
        for (auto it = src.begin(); it != src.end(); ++it) {
            res.push_back(get_utf32_char(it));
        }
        return res;
    }

    utf8_string to_utf8(const utf32_string& src) {
        utf8_string res;
        for (auto it = src.begin(); it != src.end(); ++it) {
            auto utf8_ch = get_utf8_char(it);
            res.insert(res.end(), utf8_ch.begin(), utf8_ch.end());
        }
        return res;
    }
} // namespace utf_string_converter

