#ifndef UTF8_H
#define UTF8_H

#include <string>

const char* utf8_to_codepoint(const char* p, unsigned* dst);
int codepoint_to_utf8(uint32_t utf, char* out);
std::string wstring_to_utf8string(std::wstring text);

#endif // UTF8_H