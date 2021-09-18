#include "renderer.h"

#include <curses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app.h"
#include "theme.h"
#include "operation.h"

#include "../libs/editor/util.h"

#define SELECTED_OFFSET 500
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

    K_SHIFT_HOME,
    K_SHIFT_END,
    K_HOME_KEY,
    K_END_KEY,
    K_PAGE_UP,
    K_PAGE_DOWN
};

static Renderer theRenderer;
static bool _running = true;
static std::map<int, int> color_map;

struct state {
    RenRect clip;
};
RenRect clip_rect;

static std::vector<state> state_stack;

static void update_colors()
{
    color_map.clear();

    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    init_pair(color_pair_e::NORMAL, app->fg, app->bg);
    init_pair(color_pair_e::SELECTED, app->selFg, app->selBg);

    int idx = 32;

    auto it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        color_map[it->first] = idx;
        init_pair(idx++, it->first, app->bg);
        it++;
    }

    it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        color_map[it->first + SELECTED_OFFSET] = idx;
        init_pair(idx++, it->first, app->selBg);
        if (it->first == app->selBg) {
            color_map[it->first + SELECTED_OFFSET] = idx + 1;
        }
        it++;
    }
}

int kbhit(int timeout = 500)
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

    log("escape+%d a:%d A:%d 0:%d 9:%d\n", c, 'a', 'A', '0', '9');

    return K_ESC;
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
                    // log("shift+%d\n", seq[1]);
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

                sequence = "ctrl+";
                if (seq[0] == '5') {
                    // log("ctrl+%d\n", seq[1]);
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
                    // log("ctrl+shift+%d\n", seq[1]);
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
                    // log("ctrl+alt+%d\n", seq[1]);
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
                    // log("ctrl+shift+alt+%d\n", seq[1]);
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
        }
        log("escape+%c+%c\n", seq[0], seq[1]);
    }

    /* ESC O sequences. */
    // else if (seq[0] == 'O') {
    //     log("escape+O+%d\n", seq[1]);
    //     switch (seq[1]) {
    //     case 'H':
    //         return K_HOME_KEY;
    //     case 'F':
    //         return K_END_KEY;
    //     }
    // }

    return K_ESC;
}

int readKey(std::string& keySequence)
{
    if (kbhit(100) != 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 0) {

            if (c == K_ESC) {
                return readEscapeSequence(keySequence);
            }

            switch (c) {
            case K_TAB:
                keySequence = "tab";
                return K_TAB;
            case K_ENTER:
                keySequence = "enter";
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

                log("ctrl+%d\n", c);
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
    log("init\n");

    setlocale(LC_ALL, "");

    initscr();
    raw();
    keypad(stdscr, true);
    noecho();
    nodelay(stdscr, true);

    use_default_colors();
    start_color();

    if (has_colors() && can_change_color()) {
        color_info_t::set_term_color_count(256);
    } else {
        color_info_t::set_term_color_count(8);
    }

    curs_set(0);
    clear();

    _running = true;
}

void Renderer::shutdown()
{
	endwin();

	log("shutdown\n");
}

void Renderer::show_debug(bool enable)
{}

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

    log("window: %d %d", *w, *h);
}

std::string previousKeySequence;

void Renderer::listen_events(event_list* events)
{
    std::string expandedSequence;

	int ch = -1;
    std::string keySequence;
    while (true) {
        ch = readKey(keySequence);

        if (previousKeySequence.length() && keySequence.length()) {
            expandedSequence = previousKeySequence + "+" + keySequence;
        }

        if (ch != -1) {
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

    // event!
    log("event! %s", keySequence.c_str());
    if (keySequence == "ctrl+q") {
        _running = false;
    }
}

void Renderer::listen_quick(int frames)
{
}

bool Renderer::listen_is_quick()
{
    return false;
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

RenImage* Renderer::create_image_from_png(char *filename, int w, int h)
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

void Renderer::set_clip_rect(RenRect rect)
{
    clip_rect = rect;
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

void Renderer::draw_rect(RenRect rect, RenColor clr, bool fill, int stroke, int radius)
{
}

int Renderer::draw_text(RenFont* font, const char* text, int x, int y, RenColor clr, bool bold, bool italic)
{
    int l = strlen(text);
    if (l == 0) return 0;

    move(y, x);

    if (x >= clip_rect.x + clip_rect.width) return 0;
    if (y >= clip_rect.y + clip_rect.height) return 0;

    if (x + l >= clip_rect.x + clip_rect.width) {
        l = clip_rect.width - x - 1;
        if (l < 0) return 0;
        std::string t = text;
        t.substr(0, l);
        addstr(t.c_str());
        return 0;
    }

    addstr(text);

    log(">%d %d %s", x, y, text);
    return 0;
}

void Renderer::begin_frame(RenImage *image, int w, int h, RenCache* cache)
{
	if (!color_map.size()) {
		update_colors();
	}
}

void Renderer::end_frame()
{
	refresh();
}

void Renderer::state_save()
{
    state_stack.push_back({
        clip: clip_rect
    });
}

void Renderer::state_restore()
{
    if (state_stack.size() == 0) return;
    clip_rect = state_stack.back().clip;
    state_stack.pop_back();
}

std::string Renderer::get_clipboard()
{
    return "";
}

void Renderer::set_clipboard(std::string text)
{
}

bool Renderer::is_terminal()
{
    return true;
}