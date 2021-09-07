#ifndef EVENT_H
#define EVENT_H

#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <functional>

enum event_type_e {
    EVT_UNKNOWN = 0,
    EVT_KEY_DOWN,
    EVT_KEY_UP,
    EVT_KEY_TEXT,
    EVT_KEY_SEQUENCE,
    EVT_MOUSE_DOWN,
    EVT_MOUSE_UP,
    EVT_MOUSE_MOTION,
    EVT_MOUSE_WHEEL,
    EVT_WINDOW_RESIZE,

    /* synthetic events */
    EVT_MOUSE_CLICK, 
    EVT_MOUSE_DRAG_START, 
    EVT_MOUSE_DRAG, 
    EVT_MOUSE_DRAG_END,
    EVT_SCROLLBAR_MOVE
};

struct event_object_t {
};

struct event_t {
    int type;
    int x;
    int y;
    int w;
    int h;
    int key;
    int mod;
    int button;
    int clicks;
    std::string text;
    event_object_t *source;
    event_object_t *target;
    bool cancelled;
};

typedef std::vector<event_t> event_list;

typedef std::function<bool(event_t&)> event_callback_t;
typedef std::map<int, std::vector<event_callback_t>> event_callback_list;

#endif // EVENT_H