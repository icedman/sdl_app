#ifndef EVENT_H
#define EVENT_H

#include <vector>
#include <string>

enum event_type_e {
    EVT_UNKNOWN = 0,
    EVT_KEY_DOWN,
    EVT_KEY_UP,
    EVT_KEY_TEXT,
    EVT_KEY_SHORTCUT,
    EVT_MOUSE_DOWN,
    EVT_MOUSE_UP,
    EVT_MOUSE_MOTION,
    EVT_WINDOW_RESIZE
};

struct event_t {
    event_type_e type;
    int x;
    int y;
    int w;
    int h;
    int key;
    int mod;
    int button;
    std::string text;
};

typedef std::vector<event_t> event_list;

#endif // EVENT_H