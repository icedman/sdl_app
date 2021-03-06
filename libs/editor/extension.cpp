#include "extension.h"

#include <algorithm>
#include <string.h>

#include "theme.h"
#include "util.h"

static bool file_exists(const char* path)
{
    bool exists = false;
    FILE* fp = fopen(path, "r");
    if (fp) {
        exists = true;
        fclose(fp);
    }
    return exists;
}
void load_extensions(const std::string _path, std::vector<struct extension_t>& extensions)
{
    char* cpath = (char*)malloc(_path.length() + 1 * sizeof(char));
    strcpy(cpath, _path.c_str());
    expand_path((char**)(&cpath));

    const std::string path(cpath);
    free(cpath);

    // Json::Value contribs;
    log("loading extensions in %s %s", _path.c_str(), path.c_str());

    std::vector<std::string> filter = { "themes", "iconThemes", "languages" };

    for (const auto& extensionPath : enumerate_dir(path)) {
        std::string package = extensionPath + "/package.json";
        std::string packageNLS = extensionPath + "/package.nls.json";

        struct extension_t ex = {
            .path = extensionPath
        };

        ex.nls = parse::loadJson(packageNLS);
        ex.package = parse::loadJson(package);
        if (!ex.package.isObject()) {
            continue;
        }
        ex.name = ex.package["name"].asString();

        bool append = false;
        if (ex.package.isMember("contributes")) {
            std::vector<std::string> keys = ex.package["contributes"].getMemberNames();
            std::vector<std::string>::iterator c_it = keys.begin();
            while (c_it != keys.end()) {
                std::string name = *c_it;

                // if (filter.contains(name.c_str())) {
                if (std::find(filter.begin(), filter.end(), name) != filter.end()) {
                    append = true;

                    // Json::Value obj;
                    // obj["name"] = ex.package["name"];
                    // obj["package"] = package.toStdString();
                    // contribs[name].append(obj);

                    break;
                }

                c_it++;
            }
        }

        if (append) {
            if (ex.package["name"].asString() == "meson") {
                log(ex.package["name"].asString().c_str());
                log("extensions path %s", ex.path.c_str());
            }
            extensions.emplace_back(ex);
        }
    }

    // std::cout << contribs;
}

static bool load_language_configuration(const std::string path, language_info_ptr lang)
{
    Json::Value root = parse::loadJson(path);

    if (root.empty()) {
        log("unable to load configuration file %s", path.c_str());
        return false;
    }

    if (root.isMember("comments")) {
        Json::Value comments = root["comments"];

        if (comments.isMember("lineComment")) {
            lang->lineComment = comments["lineComment"].asString();
        }

        if (comments.isMember("blockComment")) {
            Json::Value blockComment = comments["blockComment"];
            if (blockComment.isArray() && blockComment.size() == 2) {
                std::string beginComment = comments["blockComment"][0].asString();
                std::string endComment = comments["blockComment"][1].asString();
                if (beginComment.length() && endComment.length()) {
                    lang->blockCommentStart = beginComment;
                    lang->blockCommentEnd = endComment;
                }
            }
        }
    }

    if (root.isMember("brackets")) {
        Json::Value brackets = root["brackets"];
        if (brackets.isArray()) {
            for (int i = 0; i < brackets.size(); i++) {
                Json::Value pair = brackets[i];
                if (pair.isArray() && pair.size() == 2) {
                    if (pair[0].isString() && pair[1].isString()) {
                        lang->bracketOpen.push_back(pair[0].asString());
                        lang->bracketClose.push_back(pair[1].asString());
                    }
                }
            }
            lang->brackets = lang->bracketOpen.size();
        }
    }

    if (root.isMember("autoClosingPairs")) {
        Json::Value pairs = root["autoClosingPairs"];
        if (pairs.isArray()) {
            for (int i = 0; i < pairs.size(); i++) {
                Json::Value pair = pairs[i];
                if (pair.isObject()) {
                    if (pair.isMember("open") && pair.isMember("close")) {
                        lang->pairOpen.push_back(pair["open"].asString());
                        lang->pairClose.push_back(pair["close"].asString());
                    }
                }
            }
            lang->pairs = lang->pairOpen.size();
        }
    }

    return true;
}

language_info_ptr language_from_file(const std::string path, std::vector<struct extension_t>& extensions)
{
    static std::map<std::string, language_info_ptr> cache;
    language_info_ptr lang = std::make_shared<language_info_t>();

    std::set<char> delims = { '/' };
    std::vector<std::string> spath = split_path(path, delims);
    std::string fileName = spath.back();

    std::set<char> delims_file = { '.' };
    std::vector<std::string> sfile = split_path(fileName, delims_file);

    std::string suffix = ".";
    suffix += sfile.back();

    log("%s file: %s suffix: %s", path.c_str(), fileName.c_str(), suffix.c_str());

    auto it = cache.find(suffix);
    if (it != cache.end()) {
        return it->second;
    }

    // check cache
    struct extension_t resolvedExtension;
    std::string resolvedLanguage;
    Json::Value resolvedGrammars;
    Json::Value resolvedConfiguration;

    for (auto ext : extensions) {
        Json::Value contribs = ext.package["contributes"];
        if (!contribs.isMember("languages") || !contribs.isMember("grammars")) {
            continue;
        }
        Json::Value langs = contribs["languages"];
        for (int i = 0; i < langs.size(); i++) {
            Json::Value lang = langs[i];
            if (!lang.isMember("id")) {
                continue;
            }

            if (!lang.isMember("file")) {
            }

            bool found = false;
            if (lang.isMember("filenames")) {
                Json::Value fns = lang["filenames"];
                for (int j = 0; j < fns.size(); j++) {
                    Json::Value fn = fns[j];
                    if (fn.asString() == fileName) {
                        resolvedExtension = ext;
                        resolvedLanguage = lang["id"].asString();
                        resolvedGrammars = contribs["grammars"];
                        found = true;
                        break;
                    }
                }
            }

            if (!found && lang.isMember("extensions")) {
                Json::Value exts = lang["extensions"];
                for (int j = 0; j < exts.size(); j++) {
                    Json::Value ex = exts[j];

                    if (ex.asString() == suffix) {
                        resolvedExtension = ext;
                        resolvedLanguage = lang["id"].asString();
                        resolvedGrammars = contribs["grammars"];

                        // log("resolved %s", resolvedLanguage.c_str());
                        // log("resolved path %s", ext.path.c_str());
                        found = true;
                        break;
                    }
                }
            }

            if (found) {
                if (lang.isMember("configuration")) {
                    resolvedConfiguration = lang["configuration"];
                }
                break;
            }
        }

        if (!resolvedLanguage.empty())
            break;
    }

    std::string scopeName = "source.";
    scopeName += resolvedLanguage;
    log("scopeName: %s", scopeName.c_str());

    if (!resolvedLanguage.empty()) {
        for (int j = 0; j < 2; j++)
            for (int i = 0; i < resolvedGrammars.size(); i++) {
                Json::Value g = resolvedGrammars[i];
                bool foundGrammar = false;

                if (j == 0 && g.isMember("scopeName") && g["scopeName"].asString().compare(scopeName) == 0) {
                    foundGrammar = true;
                }

                if (j == 1 && g.isMember("language") && g["language"].asString().compare(resolvedLanguage) == 0) {
                    foundGrammar = true;
                }

                if (foundGrammar) {
                    std::string path = resolvedExtension.path + "/" + g["path"].asString();

                    // std::cout << path << std::endl;
                    log("grammar: %s", path.c_str());
                    log("extension: %s", resolvedExtension.path.c_str());

                    lang->grammar = parse::parse_grammar(parse::loadJson(path));
                    lang->id = resolvedLanguage;

                    // language configuration
                    if (!resolvedConfiguration.empty()) {
                        path = resolvedExtension.path + "/" + resolvedConfiguration.asString();
                    } else {
                        path = resolvedExtension.path + "/language-configuration.json";
                    }

                    load_language_configuration(path, lang);

                    log("language configuration: %s", path.c_str());
                    // std::cout << "langauge matched" << lang->id << std::endl;
                    // std::cout << path << std::endl;

                    // don't cache..? causes problem with highlighter thread
                    cache.emplace(suffix, lang);
                    return lang;
                }
            }
    }

    if (!lang->grammar) {
        Json::Value empty;
        empty["scopeName"] = suffix;
        lang->id = suffix;
        lang->grammar = parse::parse_grammar(empty);
    }

    // if (suffix != ".") {
    //     cache.emplace(suffix, lang);
    // }
    return lang;
}

icon_theme_ptr icon_theme_from_name(const std::string path, std::vector<struct extension_t>& extensions)
{
    icon_theme_ptr icons = std::make_shared<icon_theme_t>();

    std::string theme_path = path;
    std::string icons_path;
    bool found = false;
    for (auto ext : extensions) {
        Json::Value contribs = ext.package["contributes"];
        if (!contribs.isMember("iconThemes")) {
            continue;
        }

        Json::Value themes = contribs["iconThemes"];
        for (int i = 0; i < themes.size(); i++) {
            Json::Value theme = themes[i];
            if (theme["id"].asString() == theme_path || theme["label"].asString() == theme_path) {
                theme_path = ext.path + "/" + theme["path"].asString();
                icons_path = ext.path + "/icons/";
                icons->path = ext.path;
                found = true;
                break;
            }
        }

        if (found) {
            break;
        }
    }

    if (!found) {
        return icons;
    }

    Json::Value json = parse::loadJson(theme_path);
    icons->icons_path = icons_path;

    if (json.isMember("fonts")) {
        Json::Value fonts = json["fonts"];
        Json::Value font = fonts[0];
        Json::Value family = font["id"];
        Json::Value src = font["src"][0];
        Json::Value src_path = src["path"];
        std::string real_font_path = icons_path + src_path.asString();

        // QFontDatabase::addApplicationFont(real_font_path.c_str());
        // icons->font.setFamily("monospace");
        // icons->font.setFamily(family.asString().c_str());
        // icons->font.setPointSize(16);
        // icons->font.setFixedPitch(true);
    }

    icons->definition = json;
    return icons;
}

theme_ptr theme_from_name(const std::string path, std::vector<struct extension_t>& extensions, const std::string uiTheme)
{
    std::string theme_path = path;
    bool found = false;
    for (auto ext : extensions) {
        Json::Value contribs = ext.package["contributes"];
        if (!contribs.isMember("themes")) {
            continue;
        }

        Json::Value themes = contribs["themes"];
        for (int i = 0; i < themes.size(); i++) {
            Json::Value theme = themes[i];

            std::string theme_ui;
            if (theme.isMember("uiTheme")) {
                theme_ui = theme["uiTheme"].asString();
            }

            // log("theme compare %s %s\n", theme_ui.c_str(), theme["label"].asString().c_str());

            if (theme["id"].asString() == theme_path || theme["label"].asString() == theme_path) {
                theme_path = ext.path + "/" + theme["path"].asString();
                // std::cout << ext.path << "..." << std::endl;
                // std::cout << theme_path << std::endl;

                if (theme.isMember("uiTheme") && uiTheme != "" && theme["uiTheme"].asString() != uiTheme) {
                    continue;
                }

                log("theme:%s %s\n", ext.path.c_str(), theme_path.c_str());
                found = true;
                break;
            }
        }

        if (found) {
            break;
        }
    }

    Json::Value json = parse::loadJson(theme_path);
    theme_ptr theme = parse_theme(json);
    return theme;
}

icon_t icon_for_file(icon_theme_ptr icons, std::string filename, std::vector<struct extension_t>& _extensions)
{
    icon_t res;
    res.path = "";
    if (!icons) {
        return res;
    }

    std::set<char> delims = { '.' };
    std::vector<std::string> spath = split_path(filename, delims);

    std::string _suffix = spath.back();
    std::string cacheId = _suffix;

    static std::map<std::string, icon_t> cache;

    Json::Value definitions = icons->definition["iconDefinitions"];
    Json::Value fonts = icons->definition["fonts"];

    if (definitions.isMember(_suffix)) {
        Json::Value iconDef = definitions[_suffix];

        if (iconDef.isMember("iconPath")) {
            res.path = icons->icons_path + "/" + iconDef["iconPath"].asString();
            res.svg = true;

            cache.emplace(_suffix, res);
            return res;
        }
    }

    std::string iconName;

    // printf("finding icon %s\n", filename.c_str());

    auto it = cache.find(filename);
    if (it != cache.end()) {
        return it->second;
    }

    Json::Value fileNames = icons->definition["fileNames"];
    if (!iconName.length() && fileNames.isMember(filename)) {
        iconName = fileNames[filename].asString();
        cacheId = filename;
        printf("fileNames %s\n", iconName.c_str());
    }

    if (!iconName.length()) {
        it = cache.find(_suffix);
        if (it != cache.end()) {
            return it->second;
        }
    }

    Json::Value extensions = icons->definition["fileExtensions"];
    if (!iconName.length() && extensions.isMember(_suffix)) {
        iconName = extensions[_suffix].asString();
        cacheId = _suffix;
        printf("extensions %s\n", iconName.c_str());
    }

    if (!iconName.length()) {
        Json::Value languageIds = icons->definition["languageIds"];
        std::string _fileName = "file." + _suffix;
        language_info_ptr lang = language_from_file(_fileName.c_str(), _extensions);
        if (lang) {
            if (languageIds.isMember(lang->id)) {
                iconName = languageIds[lang->id].asString();
            }
        }

        if (!iconName.length()) {
            if (languageIds.isMember(_suffix)) {
                iconName = languageIds[_suffix].asString();
            }
        }
    }

    if (!iconName.length()) {
        if (file_exists(std::string(icons->icons_path + "/file.svg").c_str())) {
            res.path = icons->icons_path + "/file.svg";
            res.svg = true;
            return res;
        }
    }

    if (definitions.isMember(iconName)) {
        Json::Value iconDef = definitions[iconName];

        if (iconDef.isMember("iconPath")) {
            res.path = icons->icons_path + "/" + iconDef["iconPath"].asString();
            if (file_exists(res.path.c_str())) {
                res.svg = true;
                cache.emplace(cacheId, res);
                return res;
            }
        }

        if (iconDef.isMember("fontCharacter")) {
            res.character = iconDef["fontCharacter"].asString();
            if (fonts.size()) {
                Json::Value src = fonts[0]["src"];
                if (src.size()) {
                    Json::Value path = src[0]["path"];
                    res.path = icons->icons_path + "/" + path.asString();
                    res.svg = false;
                    cache.emplace(cacheId, res);
                }
            }
        }

        return res;
    }

    printf("not found %s\n", filename.c_str());
    return res;
}

icon_t icon_for_folder(icon_theme_ptr icons, std::string folder, std::vector<struct extension_t>& _extensions)
{
    icon_t res;
    return res;
}

bool theme_is_dark(theme_ptr theme)
{
    color_info_t clr;
    theme->theme_color("editor.background", clr);
    return color_is_dark(clr);
}

bool color_is_dark(color_info_t& color)
{
    return 0.30 * color.red + 0.59 * color.green + 0.11 * color.blue < 0.5;
}
