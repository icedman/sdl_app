#include "image.h"
#include "damage.h"
#include "system.h"
#include "utf8.h"

#include <iostream>
#include <sstream>
#include <string>

void image_view_t::render(renderer_t* renderer)
{
    if (!image) {
        return;
    }

    float sz = 0.75f;
    layout_item_ptr item = layout();
    system_t::instance()->renderer.draw_image(image.get(),
        { item->render_rect.x + image->width * (1.0f - sz), item->render_rect.y, image->width * sz, image->height * sz },
        { 255, 255, 255, 0 });
}

void image_view_t::load_image(std::string path, int w, int h)
{
    layout()->width = w;
    layout()->height = h;
    image = system_t::instance()->renderer.create_image_from_svg(path, w, h);
}

std::string to_utf8(uint32_t cp)
{
    // https://stackoverflow.com/questions/28534221/c-convert-asii-escaped-unicode-string-into-utf8-string/47734595.

    std::string result;

    int count;
    if (cp <= 0x007F)
        count = 1;
    else if (cp <= 0x07FF)
        count = 2;
    else if (cp <= 0xFFFF)
        count = 3;
    else if (cp <= 0x10FFFF)
        count = 4;
    else
        return result; // or throw an exception

    result.resize(count);

    if (count > 1) {
        for (int i = count - 1; i > 0; --i) {
            result[i] = (char)(0x80 | (cp & 0x3F));
            cp >>= 6;
        }

        for (int i = 0; i < count; ++i)
            cp |= (1 << (7 - i));
    }

    result[0] = (char)cp;

    return result;
}

std::string wstring_convert(std::string str)
{
    std::string::size_type startIdx = 0;
    do {
        startIdx = str.find("x\\", startIdx);
        if (startIdx == std::string::npos)
            break;
        std::string::size_type endIdx = str.length();
        // str.find_first_not_of("0123456789abcdefABCDEF", startIdx+2);
        if (endIdx == std::string::npos)
            break;
        std::string tmpStr = str.substr(startIdx + 2, endIdx - (startIdx + 2));
        std::istringstream iss(tmpStr);

        uint32_t cp;
        if (iss >> std::hex >> cp) {
            std::string utf8 = to_utf8(cp);
            str.replace(startIdx, 2 + tmpStr.length(), utf8);
            startIdx += utf8.length();
        } else
            startIdx += 2;
    } while (true);

    return str;
}

void icon_view_t::render(renderer_t* renderer)
{
    if (image) {
        image_view_t::render(renderer);
        return;
    }

    if (character.length()) {
        layout_item_ptr lo = layout();
        renderer->draw_text(icon_font.get(), (char*)character.c_str(), lo->render_rect.x + icon_font->width/2, lo->render_rect.y, {255,255,255});
    }
}

void icon_view_t::load_icon(std::string path, std::string c)
{
    if (!system_t::instance()->renderer.register_font(path)) return;

    icon_font = system_t::instance()->renderer.create_font("seti", 12);
    image = nullptr;

    int w = 24;
    int h = 24;
    layout()->width = 400;
    layout()->height = h;

    c = "x" + c;
    character = wstring_convert(c);
}