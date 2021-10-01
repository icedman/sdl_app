#ifndef UTF8_H
#define UTF8_H

#include <string>

const char* utf8_to_codepoint(const char* p, unsigned* dst);
int codepoint_to_utf8(uint32_t utf, char* out);
std::string wstring_to_utf8string(std::wstring text);

std::string utf8_substr(std::string &text, size_t start, size_t len);
size_t utf8_length(std::string &text);

#endif // UTF8_H
