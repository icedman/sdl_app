#include "icons.h"
#include "system.h"

#include <fstream>
#include <iostream>
#include <json/json.h>
#include <sstream>
#include <string>

static icons_factory_t global_icons;

static inline std::string to_utf8(uint32_t cp)
{
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

static inline std::string wstring_convert(std::string str)
{
    std::string::size_type startIdx = 0;
    do {
        startIdx = str.find("x\\", startIdx);
        if (startIdx == std::string::npos)
            break;
        std::string::size_type endIdx = str.length();
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

icons_factory_t* icons_factory_t::instance()
{
    return &global_icons;
}

static inline Json::Value load_json(std::string filename)
{
    Json::Value root;
    std::ifstream ifs;
    ifs.open(filename);

    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;
    if (!parseFromStream(builder, ifs, &root, &errs)) {
        // error!
    }

    return root;
}

void icons_factory_t::load_icons(std::string path, int w, int h)
{
    Json::Value json = load_json(path);
    Json::Value svg = json["svg"];
    Json::Value defs = svg["defs"];
    Json::Value symbols = defs["symbol"];

    renderer_t* renderer = &system_t::instance()->renderer;

    for (int i = 0; i < symbols.size(); i++) {
        Json::Value sym = symbols[i];
        Json::Value id = sym["id"];
        Json::Value box = sym["viewBox"];
        Json::Value path = sym["path"];

        std::ostringstream ss;
        ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
        ss << "viewBox=\"" << box.asString() << "\">";
        if (path.isArray()) {
            for (int j = 0; j < path.size(); j++) {
                Json::Value d = path[j]["d"];
                ss << "<path d=\"";
                ss << d.asString();
                ss << "\"></path>";
            }
        } else {
            Json::Value d = path["d"];
            ss << "<path fill=\"rgb(255,255,255)\" d=\"";
            ss << d.asString();
            ss << "\"></path>";
        }
        ss << "</svg>\n";

        renderer->create_image_from_svg_data(ss.str(), w, h, id.asString());
    }
}

image_ptr icons_factory_t::icon(std::string name)
{
    return system_t::instance()->renderer.image(name);
}
