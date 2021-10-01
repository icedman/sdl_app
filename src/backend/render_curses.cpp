#include "renderer.h"

#include <curses.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <codecvt>
#include <locale>

#include "app.h"
#include "operation.h"
#include "theme.h"
#include "utf8.h"

static int last_millis = 0;

#define CTRL_KEY(k) ((k)&0x1f)

enum KEY_ACTION {
    K_NULL = 0, /* NULL */
    K_ENTER = 13,
    K_TAB = 9,
    K_ESC = 27,
    K_BACKSPACE = 127,
    K_RESIZE = 150,

    /* The following are just soft codes, not really reported by the terminal directly. */
    K_ALT_ = 1000,
    K_CTRL_,
    K_CTRL_ALT_,
    K_CTRL_SHIFT_,
    K_CTRL_SHIFT_ALT_,
    K_CTRL_UP,
    K_CTRL_DOWN,
    K_CTRL_LEFT,
    K_CTRL_RIGHT,
    K_CTRL_HOME,
    K_CTRL_END,
    K_CTRL_SHIFT_UP,
    K_CTRL_SHIFT_DOWN,
    K_CTRL_SHIFT_LEFT,
    K_CTRL_SHIFT_RIGHT,
    K_CTRL_SHIFT_HOME,
    K_CTRL_SHIFT_END,
    K_CTRL_ALT_UP,
    K_CTRL_ALT_DOWN,
    K_CTRL_ALT_LEFT,
    K_CTRL_ALT_RIGHT,
    K_CTRL_ALT_HOME,
    K_CTRL_ALT_END,
    K_CTRL_SHIFT_ALT_LEFT,
    K_CTRL_SHIFT_ALT_RIGHT,
    K_CTRL_SHIFT_ALT_HOME,
    K_CTRL_SHIFT_ALT_END,

    K_ALT_UP,
    K_ALT_DOWN,
    K_ALT_LEFT,
    K_ALT_RIGHT,

    K_SHIFT_HOME,
    K_SHIFT_END,
    K_HOME_KEY,
    K_END_KEY,
    K_PAGE_UP,
    K_PAGE_DOWN,

    K_MOUSE
};

static Renderer theRenderer;
static bool _running = true;
static std::map<int, color_info_t> color_map;

typedef struct _color_pair {
    int idx;
    int fg;
    int bg;
};

static std::map<int, _color_pair> color_pairs;

struct screen_meta_t {
    char bg;
    char underline;
};

screen_meta_t* screen_meta = 0;
int bg_w = 0;
int bg_h = 0;

struct state {
    RenRect clip;
};
RenRect clip_rect;

static std::vector<state> state_stack;

int window_cols;
int window_rows;

static int pair_for_colors(int fg, int bg)
{
    for (auto p : color_pairs) {
        if (p.second.fg == fg && p.second.bg == bg) {
            return p.second.idx;
        }
    }
    int idx = color_pairs.size();
    init_pair(idx, fg, bg);
    color_pairs[idx] = { idx, fg, bg };
    return idx;
}

static int kbhit(int timeout = 500)
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = timeout;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

    return FD_ISSET(STDIN_FILENO, &fds);
}

static int readMoreEscapeSequence(int c, std::string& keySequence)
{
    char tmp[32];

    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z')) {
        sprintf(tmp, "alt+%c", c);
        keySequence = tmp;
        return K_ALT_;
    }

    if (c == '[' || c == ']' || c == '/') {
        sprintf(tmp, "alt+%c", c);
        keySequence = tmp;
        return K_ALT_;
    }

    if (c >= 'A' && c <= 'Z') {
        sprintf(tmp, "alt+shift+%c", (c + 'a' - 'A'));
        keySequence = tmp;
        return K_ALT_;
    }

    if ((c + 'a' - 1) >= 'a' && (c + 'a' - 1) <= 'z') {
        sprintf(tmp, "ctrl+alt+%c", (c + 'a' - 1));
        keySequence = tmp;
        return K_CTRL_ALT_;
    }

    return K_ESC;
}

static int readMoreMouseSequence(std::string& keySequence)
{
    keySequence = "mouse;";

    std::string m;
    char c;
    while (true) {
        read(STDIN_FILENO, &c, 1);
        if (!c)
            break;
        if (c == 'M')
            break;
        if (c == 'm')
            break;
        keySequence += c;
    }

    // if (c == 'M')
    keySequence += ";";
    keySequence += c;

    return 0;
}
static int readEscapeSequence(std::string& keySequence)
{
    keySequence = "";
    std::string sequence = "";

    char seq[4];
    int wait = 500;

    if (!kbhit(wait)) {
        return K_ESC;
    }
    read(STDIN_FILENO, &seq[0], 1);
    // seq[0] = getch();

    if (!kbhit(wait)) {
        return readMoreEscapeSequence(seq[0], keySequence);
    }
    read(STDIN_FILENO, &seq[1], 1);

    /* ESC [ sequences. */
    if (seq[0] == '[') {
        if (seq[1] >= '0' && seq[1] <= '9') {

            /* Extended escape, read additional byte. */
            if (!kbhit(wait)) {
                return K_ESC;
            }
            read(STDIN_FILENO, &seq[2], 1);

            if (seq[2] == '~') {
                switch (seq[1]) {
                case '3':
                    keySequence = "delete";
                    return KEY_DC;
                case '5':
                    keySequence = "pageup";
                    return K_PAGE_UP;
                case '6':
                    keySequence = "pagedown";
                    return K_PAGE_DOWN;
                }
            }

            if (seq[2] == ';') {
                if (!kbhit(wait)) {
                    return K_ESC;
                }
                read(STDIN_FILENO, &seq[0], 1);

                if (!kbhit(wait)) {
                    return K_ESC;
                }
                read(STDIN_FILENO, &seq[1], 1);

                sequence = "shift+";
                if (seq[0] == '2') {
                    app_t::log("shift+%d\n", seq[1]);
                    switch (seq[1]) {
                    case 'A':
                        keySequence = sequence + "up";
                        return KEY_SR;
                    case 'B':
                        keySequence = sequence + "down";
                        return KEY_SF;
                    case 'C':
                        keySequence = sequence + "right";
                        return KEY_SRIGHT;
                    case 'D':
                        keySequence = sequence + "left";
                        return KEY_SLEFT;
                    case 'H':
                        keySequence = sequence + "home";
                        return K_SHIFT_HOME;
                    case 'F':
                        keySequence = sequence + "end";
                        return K_SHIFT_END;
                    }
                }

                sequence = "alt+";
                if (seq[0] == '3') {
                    app_t::log("alt+%c\n", seq[1]);
                    switch (seq[1]) {
                    case 'A':
                        keySequence = sequence + "up";
                        return K_ALT_UP;
                    case 'B':
                        keySequence = sequence + "down";
                        return K_ALT_DOWN;
                    case 'C':
                        keySequence = sequence + "right";
                        return K_ALT_RIGHT;
                    case 'D':
                        keySequence = sequence + "left";
                        return K_ALT_LEFT;
                    }
                }

                sequence = "ctrl+";
                if (seq[0] == '5') {
                    app_t::log("ctrl+%d\n", seq[1]);
                    switch (seq[1]) {
                    case 'A':
                        keySequence = sequence + "up";
                        return K_CTRL_UP;
                    case 'B':
                        keySequence = sequence + "down";
                        return K_CTRL_DOWN;
                    case 'C':
                        keySequence = sequence + "right";
                        return K_CTRL_RIGHT;
                    case 'D':
                        keySequence = sequence + "left";
                        return K_CTRL_LEFT;
                    case 'H':
                        keySequence = sequence + "home";
                        return K_CTRL_HOME;
                    case 'F':
                        keySequence = sequence + "end";
                        return K_CTRL_END;
                    }
                }

                sequence = "ctrl+shift+";
                if (seq[0] == '6') {
                    app_t::log("ctrl+shift+%d\n", seq[1]);
                    switch (seq[1]) {
                    case 'A':
                        keySequence = sequence + "up";
                        return K_CTRL_SHIFT_UP;
                    case 'B':
                        keySequence = sequence + "down";
                        return K_CTRL_SHIFT_DOWN;
                    case 'C':
                        keySequence = sequence + "right";
                        return K_CTRL_SHIFT_RIGHT;
                    case 'D':
                        keySequence = sequence + "left";
                        return K_CTRL_SHIFT_LEFT;
                    case 'H':
                        keySequence = sequence + "home";
                        return K_CTRL_SHIFT_HOME;
                    case 'F':
                        keySequence = sequence + "end";
                        return K_CTRL_SHIFT_END;
                    }
                }

                sequence = "ctrl+alt+";
                if (seq[0] == '7') {
                    app_t::log("ctrl+alt+%d\n", seq[1]);
                    switch (seq[1]) {
                    case 'A':
                        keySequence = sequence + "up";
                        return K_CTRL_ALT_UP;
                    case 'B':
                        keySequence = sequence + "down";
                        return K_CTRL_ALT_DOWN;
                    case 'C':
                        keySequence = sequence + "right";
                        return K_CTRL_ALT_RIGHT;
                    case 'D':
                        keySequence = sequence + "left";
                        return K_CTRL_ALT_LEFT;
                    case 'H':
                        keySequence = sequence + "home";
                        return K_CTRL_ALT_HOME;
                    case 'F':
                        keySequence = sequence + "end";
                        return K_CTRL_ALT_END;
                    }
                }

                sequence = "ctrl+shift+alt+";
                if (seq[0] == '8') {
                    app_t::log("ctrl+shift+alt+%d\n", seq[1]);
                    switch (seq[1]) {
                    case 'A':
                        keySequence = sequence + "up";
                        return KEY_SR;
                    case 'B':
                        keySequence = sequence + "down";
                        return KEY_SF;
                    case 'C':
                        keySequence = sequence + "right";
                        return K_CTRL_SHIFT_ALT_RIGHT;
                    case 'D':
                        keySequence = sequence + "left";
                        return K_CTRL_SHIFT_ALT_LEFT;
                    case 'H':
                        keySequence = sequence + "home";
                        return K_CTRL_SHIFT_ALT_HOME;
                    case 'F':
                        keySequence = sequence + "end";
                        return K_CTRL_SHIFT_ALT_END;
                    }
                }

                return K_ESC;
            }
        }
    }

    if (seq[0] == '[' || seq[0] == 'O') {
        switch (seq[1]) {
        case 'A':
            keySequence = "up";
            return KEY_UP;
        case 'B':
            keySequence = "down";
            return KEY_DOWN;
        case 'C':
            keySequence = "right";
            return KEY_RIGHT;
        case 'D':
            keySequence = "left";
            return KEY_LEFT;
        case 'H':
            keySequence = "home";
            return K_HOME_KEY;
        case 'F':
            keySequence = "end";
            return K_END_KEY;
        case '<':
            readMoreMouseSequence(keySequence);
            return K_MOUSE;
        }
        app_t::log("escape+%c+%c\n", seq[0], seq[1]);
    }

    app_t::log("?+%c+%c\n", seq[0], seq[1]);

    return K_ESC;
}

static int readKey(std::string& keySequence)
{
    if (kbhit(100) != 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 0) {

            if (c == K_ESC) {
                return readEscapeSequence(keySequence);
            }

            // app_t::log("%d\n", c);

            switch (c) {
            case K_TAB:
                keySequence = "tab";
                return K_TAB;
            case K_ENTER:
                keySequence = "return";
                return c;
            case K_BACKSPACE:
            case KEY_BACKSPACE:
                keySequence = "backspace";
                return c;
            case K_RESIZE:
            case KEY_RESIZE:
                keySequence = "resize";
                return c;
            }

            if (CTRL_KEY(c) == c) {
                keySequence = "ctrl+";
                c = 'a' + (c - 1);
                if (c >= 'a' && c <= 'z') {
                    keySequence += c;
                    return c;
                } else {
                    keySequence += '?';
                }

                app_t::log("ctrl+%d\n", c);
                return c;
            }

            return c;
        }
    }
    return -1;
}

Renderer* Renderer::instance()
{
    return &theRenderer;
}

void Renderer::init()
{
    app_t::log("init\n");

    setlocale(LC_ALL, "");

    initscr();

    raw();
    keypad(stdscr, true);
    noecho();
    nodelay(stdscr, true);

    // mousemask(ALL_MOUSE_EVENTS, NULL);

    use_default_colors();
    start_color();

    if (has_colors() && can_change_color()) {
        color_info_t::set_term_color_count(256);
    } else {
        color_info_t::set_term_color_count(8);
    }

    curs_set(0);
    clear();

    update_colors();

    _running = true;

    throttle_up_events();
}

void Renderer::shutdown()
{
    endwin();
    app_t::log("shutdown\n");
}

void Renderer::show_debug(bool enable)
{
}

void Renderer::quit()
{
    _running = false;
}

bool Renderer::is_running()
{
    return _running;
}

void Renderer::get_window_size(int* w, int* h)
{
    static struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *w = ws.ws_col;
    *h = ws.ws_row;

    window_cols = ws.ws_col;
    window_rows = ws.ws_row;

    // app_t::log("window: %d %d", *w, *h);
}

std::string previousKeySequence;

void Renderer::listen_events(event_list* events)
{
    events->clear();

    std::string expandedSequence;
    std::string keySequence;

    int ch = -1;
    while (true) {
        ch = readKey(keySequence);
        if (previousKeySequence.length() && keySequence.length()) {
            expandedSequence = previousKeySequence + "+" + keySequence;
        }

        if (ch != -1 || is_throttle_up_events()) {
            break;
        }
    }

    if (ch == -1)
        return;

    previousKeySequence = keySequence;
    if (expandedSequence.length()) {
        if (operationFromKeys(expandedSequence) != UNKNOWN) {
            keySequence = expandedSequence;
            previousKeySequence = "";
        }
        expandedSequence = "";
    }

    if (keySequence == "ctrl+q") {
        _running = false;
    }

    if (keySequence.length() > 1) {

        if (keySequence[0] == 'm' && keySequence[1] == 'o') {
            std::vector<std::string> strings;
            std::istringstream f(keySequence);
            std::string s;
            while (getline(f, s, ';')) {
                strings.push_back(s);
            }
            int x = std::stoi(strings[2]);
            int y = std::stoi(strings[3]);

            if (strings[4] == "m")
                return;

            events->push_back({
                type : EVT_MOUSE_DOWN,
                x : x,
                y : y,
                button : 0,
                clicks : 1
            });

            events->push_back({
                type : EVT_MOUSE_UP,
                x : x,
                y : y,
                button : 0,
                clicks : 1
            });

            events->push_back({
                type : EVT_MOUSE_CLICK,
                x : x,
                y : y,
                button : 0,
                clicks : 1
            });

            // app_t::log("%s", keySequence.c_str());
            return;
        }

        events->push_back({
            type : EVT_KEY_SEQUENCE,
            text : keySequence
        });
        return;
    }

    if (!isprint(ch)) {
        switch (ch) {
        case K_ESC:
            events->push_back({
                type : EVT_KEY_SEQUENCE,
                text : "escape"
            });
            break;
        }
        return;
    }

    std::string c;
    c += ch;

    app_t::log("text %s", c.c_str());

    events->push_back({
        type : EVT_KEY_TEXT,
        text : c
    });
}

void Renderer::wake()
{
    throttle_up_events(4);
}

int Renderer::key_mods()
{
    return 0;
}

RenImage* Renderer::create_image(int w, int h)
{
    return 0;
}

RenImage* Renderer::create_image_from_svg(char* filename, int w, int h)
{
    return 0;
}

RenImage* Renderer::create_image_from_png(char* filename, int w, int h)
{
    return 0;
}

void Renderer::destroy_image(RenImage* image)
{
}

void Renderer::destroy_images()
{
}

void Renderer::image_size(RenImage* image, int* w, int* h)
{
    *w = 1;
    *h = 1;
}

void Renderer::save_image(RenImage* image, char* filename)
{
}

void Renderer::register_font(char* path)
{
}

RenFont* Renderer::create_font(char* font_desc, char* alias)
{
    return 0;
}

RenFont* Renderer::font(char* alias)
{
    return 0;
}

void Renderer::destroy_font(RenFont* font)
{
}

void Renderer::destroy_fonts()
{
}

RenFont* Renderer::get_default_font()
{
    return 0;
}

void Renderer::set_default_font(RenFont* font)
{
}

void Renderer::get_font_extents(RenFont* font, int* w, int* h, const char* text, int len)
{
    *w = 1;
    *h = 1;
}

void Renderer::update_rects(RenRect* rects, int count)
{
}

static inline int min(int a, int b) { return a < b ? a : b; }
static inline int max(int a, int b) { return a > b ? a : b; }

static RenRect intersect_rects(RenRect a, RenRect b)
{
    int x1 = max(a.x, b.x);
    int y1 = max(a.y, b.y);
    int x2 = min(a.x + a.width, b.x + b.width);
    int y2 = min(a.y + a.height, b.y + b.height);
    return (RenRect){ x1, y1, max(0, x2 - x1), max(0, y2 - y1) };
}

static RenRect merge_rects(RenRect a, RenRect b)
{
    int x1 = min(a.x, b.x);
    int y1 = min(a.y, b.y);
    int x2 = max(a.x + a.width, b.x + b.width);
    int y2 = max(a.y + a.height, b.y + b.height);
    return (RenRect){ x1, y1, x2 - x1, y2 - y1 };
}

void Renderer::set_clip_rect(RenRect rect)
{
    clip_rect = intersect_rects(rect, clip_rect);
}

void Renderer::invalidate_rect(RenRect rect)
{
    // cache
}

void Renderer::invalidate()
{
    // cache
}

void Renderer::draw_image(RenImage* image, RenRect rect, RenColor clr)
{
}

// todo UTF8 is clipped?
static bool _is_clipped(RenRect cr, int x, int y)
{
    if (!(x >= cr.x && x < cr.x + cr.width))
        return true;
    if (!(y >= cr.y && y < cr.y + cr.height))
        return true;
    return false;
}

static bool is_clipped(int x, int y)
{
    if (_is_clipped(clip_rect, x, y))
        return true;
    for (auto s : state_stack) {
        if (_is_clipped(s.clip, x, y))
            return true;
    }
    return false;
}

void Renderer::draw_underline(RenRect rect, RenColor color)
{
    screen_meta[rect.x + rect.y * bg_w].underline = 1;
}

void Renderer::draw_rect(RenRect rect, RenColor clr, bool fill, int stroke, int radius)
{
    if (!fill)
        return;

    int clr_index = clr.a;

    for (int y = 0; y < rect.height; y++) {

        for (int x = 0; x < rect.width; x++) {
            if (is_clipped(rect.x + x, rect.y + y))
                continue;

            move(rect.y + y, rect.x + x);
            int pair = pair_for_colors(-1, clr_index);
            attron(COLOR_PAIR(pair));
            addch(' ');
            attroff(COLOR_PAIR(pair));
            screen_meta[rect.x + x + (rect.y + y) * bg_w].bg = clr_index;
        }
    }
}

int Renderer::draw_wtext(RenFont* font, const wchar_t* text, int x, int y, RenColor clr, bool bold, bool italic, bool underline)
{
    int clr_index = clr.a ? clr.a : -1;

    if (bold) {
        attron(A_BOLD);
    }

    wchar_t* p = (wchar_t*)text;

    int pair = 0;
    int i = 0;
    while (*p) {
        if (is_clipped(x + i, y) || (*p >= 0xD800 && *p <= 0xD8FF)) {
            p++;
            i++;
            continue;
        }

        move(y, x + i);
        int bg = screen_meta[x + i + y * bg_w].bg;
        int ul = screen_meta[x + i + y * bg_w].underline;
        pair = pair_for_colors(clr_index, bg && bg != clr_index ? bg : -1);
        if (bg == clr_index) {
            attron(A_REVERSE);
        }
        if (ul || underline) {
            attron(A_UNDERLINE);
        }
        attron(COLOR_PAIR(pair));

        wchar_t s[2] = { *p, 0 };
        if (s[0] != L'\n' && s[0] != L'\t') {
            addwstr((wchar_t*)s);
        }

        attroff(A_UNDERLINE);
        attroff(A_REVERSE);
        attroff(COLOR_PAIR(pair));

        p++;
        i++;
    }

    attroff(A_UNDERLINE);
    attroff(A_BOLD);

    // app_t::log(">%d %d %s", clr.a, pair, text);
    return 0;
}

int Renderer::draw_text(RenFont* font, const char* text, int x, int y, RenColor clr, bool bold, bool italic, bool underline)
{
    std::wstring w = utf8string_to_wstring(text);
    draw_wtext(font, w.c_str(), x, y, clr, bold, italic, underline);
}

int Renderer::draw_char(RenFont* font, char ch, int x, int y, RenColor clr, bool bold, bool italic, bool underline)
{
    int clr_index = clr.a ? clr.a : -1;

    int _ch = ch;
    int _wch = 0;
    switch (ch) {
    case '#':
        _ch = ACS_CKBOARD;
        break;
    case 'x':
        _ch = ACS_DIAMOND;
        break;
    case '|':
        _ch = '|';
        _wch = L'▐';
        break;
    case '-':
        _ch = ACS_HLINE;
        _wch = L'▂';
        break;
        // case 'u':
        //     _ch = ACS_TTEE;
        //     break;
        // case 'd':
        //     _ch = ACS_BTEE;
        //     break;
        // case 'l':
        //     _ch = ACS_LTEE;
        //     break;
        // case 'r':
        //     _ch = ACS_RTEE;
        break;
    case 'u':
        _ch = ACS_UARROW;
        break;
    case 'd':
        _ch = ACS_DARROW;
        break;
    case 'l':
        _ch = ACS_LARROW;
        break;
    case 'r':
        _ch = ACS_RARROW;
        break;
    }

    move(y, x);

    if (bold) {
        attron(A_BOLD);
    }

    int pair = 0;
    if (is_clipped(x, y))
        return 0;

    int bg = screen_meta[x + y * bg_w].bg;
    pair = pair_for_colors(clr_index, bg && bg != clr_index ? bg : -1);
    if (bg == clr_index) {
        attron(A_REVERSE);
    }

    attron(COLOR_PAIR(pair));

    if (_wch != 0) {
        wchar_t _ws[2] = { _wch, 0 };
        addwstr((wchar_t*)_ws);
    } else {
        addch(_ch);
    }

    attroff(COLOR_PAIR(pair));

    attroff(A_REVERSE);
    attroff(A_UNDERLINE);
    attroff(A_BOLD);

    return 1;
}

void Renderer::begin_frame(RenImage* image, int w, int h, RenCache* cache)
{
    if (!color_map.size()) {
        update_colors();
    }

    if (screen_meta == 0 || bg_w != window_cols || bg_h != window_rows) {
        if (screen_meta)
            free(screen_meta);
        bg_w = window_cols;
        bg_h = window_rows;
        screen_meta = (screen_meta_t*)calloc(bg_w * bg_h, sizeof(screen_meta_t));
    }

    memset(screen_meta, 0, bg_w * bg_h * sizeof(screen_meta_t));

    clip_rect = { 0, 0, window_cols, window_rows };

    erase();
    state_save();
}

void Renderer::end_frame()
{
    refresh();

    state_restore();
}

void Renderer::state_save()
{
    state_stack.push_back({
        clip : clip_rect
    });
}

void Renderer::state_restore()
{
    if (state_stack.size() == 0)
        return;
    clip_rect = state_stack.back().clip;
    state_stack.pop_back();
}

int Renderer::draw_count()
{
    return 0;
}

int Renderer::ticks()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    auto elapsed = last_millis > 0 ? millis - last_millis : 1;
    if (elapsed > 0) {
        last_millis = millis;
    }
    return elapsed;
}

void Renderer::delay(int d)
{
}

std::string _clipText;
std::string Renderer::get_clipboard()
{
    return _clipText;
}

void Renderer::set_clipboard(std::string text)
{
    _clipText = text;
}

bool Renderer::is_terminal()
{
    return true;
}

color_info_t Renderer::color_for_index(int index)
{
    return color_map[index];
}

void Renderer::update_colors()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    pair_for_colors(-1, -1);

    auto it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        color_info_t fg = it->second;
        fg.red = fg.red <= 1 ? fg.red * 255 : fg.red;
        fg.green = fg.green <= 1 ? fg.green * 255 : fg.green;
        fg.blue = fg.blue <= 1 ? fg.blue * 255 : fg.blue;
        fg.index = it->second.index;
        color_map[it->second.index] = fg;

        int p = pair_for_colors(fg.index, -1);
        app_t::log("%d {%f %f %f} %d\n", fg.index, fg.red, fg.green, fg.blue, p);

        it++;
    }
}
