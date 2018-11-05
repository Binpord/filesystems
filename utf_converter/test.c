#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utf_string_conversion.h"

#define print_utf(str, cnt)         \
    printf("{");                    \
    cnt = 0;                        \
    while (str[cnt])                \
        printf("%#x, ", str[cnt++]);\
    printf("}\n");

int main(int argc, char *argv[]) {
    utf32_char_t *utf32_str = NULL;
    utf8_char_t *utf8_str = NULL;
    int ret = 0;

    // "Foo © bar 𝌆 baz ☃ qux"
    utf8_char_t str[] = {0x46, 0x6F, 0x6F, 0x20, 0xC2, 0xA9, 0x20, 0x62, 0x61, 0x72, 0x20, 0xF0, 0x9D, 0x8C, 0x86, 0x20, 0x62, 0x61, 0x7A, 0x20, 0xE2, 0x98, 0x83, 0x20, 0x71, 0x75, 0x78, 0x00};
    int i = 0;

    printf("original string:\n");
    print_utf(str, i);

    int utf32_len = to_utf32(str, NULL);
    if (utf32_len < 0) {
        fprintf(stderr, "Error while getting utf32_str size: %s\n", e_to_str(utf32_len));
        ret = 1;
        goto cleanup;
    }

    utf32_str = (utf32_char_t *)malloc(utf32_len * sizeof(*utf32_str));
    int check = to_utf32(str, utf32_str);
    if (check < 0) {
        fprintf(stderr, "Error while converting str to utf32: %s\n", e_to_str(check));
        ret = 2;
        goto cleanup;
    }

    printf("utf32 converted string:\n");
    print_utf(utf32_str, i);

    int utf8_len = to_utf8(utf32_str, NULL);
    if (utf8_len < 0) {
        fprintf(stderr, "Error while getting twice converted str size: %s\n", e_to_str(utf8_len));
        ret = 3;
        goto cleanup;
    }

    utf8_str = (utf8_char_t *)malloc(utf8_len * sizeof(*utf8_str));
    check = to_utf8(utf32_str, utf8_str);
    if (check < 0) {
        fprintf(stderr, "Error while converting string to utf8: %s\n", e_to_str(check));
        ret = 4;
        goto cleanup;
    }

    printf("twice converted string:\n");
    print_utf(utf8_str, i);

cleanup:
    if (utf32_str)
        free(utf32_str);

    if (utf8_str)
        free(utf8_str);

    return ret;
}

