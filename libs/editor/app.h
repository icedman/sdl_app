#ifndef APP_H
#define APP_H

#include "editor.h"
#include "extension.h"

#include <string>

enum color_pair_e {
    NORMAL = 0,
    SELECTED
};

int pairForColor(int colorIdx, bool selected);

struct app_t {
    app_t();
    ~app_t();

    static app_t* instance();

    void configure(int argc, char** argv);
    void setupColors(bool trueColors);
    void shutdown();

    // view
    // void layout(int x, int y, int width, int height) override;
    // void preLayout() override;

    void refresh();
    bool isFresh();

    bool showStatusBar;
    bool showGutter;
    bool showTabbar;
    bool showSidebar;
    bool showMinimap;
    bool enablePopup;
    int tabSize;
    bool tabsToSpaces;
    bool lineWrap;
    bool matchBrackets;

    std::string font;
    int fontSize;

    extension_list extensions;
    theme_ptr theme;
    icon_theme_ptr icons;
    icon_theme_ptr icons_default;

    std::string inputFile;

    bool debug;

    // colors
    bool trueColors;
    std::string themeName;

    int fg;
    int bg;
    int bgApp;

    int selFg;
    int selBg;

    int treeFg;
    int treeBg;
    int treeHoverFg;
    int treeHoverBg;

    int tabFg;
    int tabBg;
    int tabHoverFg;
    int tabHoverBg;
    int tabActiveBorder;

    std::vector<std::string> excludeFiles;
    std::vector<std::string> excludeFolders;
    std::vector<std::string> binaryFiles;
    std::string scriptPath;

    editor_ptr findEditor(std::string path);
    editor_ptr openEditor(std::string path, bool check = true);
    editor_ptr newEditor();
    void closeEditor(editor_ptr editor);
    editor_ptr currentEditor;

    editor_list editors;

    bool end;
    int refreshCount;

    void* view;

    static void log(const char* format, ...);
};

int pairForColor(int colorIdx, bool selected);

#endif // APP_H
