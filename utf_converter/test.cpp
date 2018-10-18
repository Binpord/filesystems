#include <iostream>
#include <vector>
#include "utf_string_converter.h"

template<class utf_string>
void print_utf_string(const utf_string& string) {
    std::cout << "{";
    for (auto c : string) {
        std::cout << "0x" << std::hex << (unsigned)c << ", ";
    }
    std::cout << "}" << std::endl;
}

int main(int argc, char *argv[])
{
    // "Foo © bar 𝌆 baz ☃ qux"
    utf8_string string = {0x46, 0x6F, 0x6F, 0x20, 0xC2, 0xA9, 0x20, 0x62, 0x61, 0x72, 0x20, 0xF0, 0x9D, 0x8C, 0x86, 0x20, 0x62, 0x61, 0x7A, 0x20, 0xE2, 0x98, 0x83, 0x20, 0x71, 0x75, 0x78};
    utf32_string converted_string = utf_string_converter::to_utf32(string);
    utf8_string back_converted_string = utf_string_converter::to_utf8(converted_string);

    std::cout << "original string was ";
    print_utf_string(string);

    std::cout << "utf32 converted string is ";
    print_utf_string(converted_string);

    std::cout << "twice converted string is ";
    print_utf_string(back_converted_string);

    return 0;
}

