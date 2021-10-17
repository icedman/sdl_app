#ifndef EVENT_H
#define EVENT_H

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>

#define K_MOD_SHIFT 1 << 1
#define K_MOD_CTRL 1 << 2
#define K_MOD_ALT 1 << 3
#define K_MOD_GUI 1 << 4

enum event_type_e {
    EVT_UNKNOWN = 0,
    EVT_ALL = 1,

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

    /* ui events */
    EVT_SCROLL,
    EVT_SCROLLBAR_MOVE,
    EVT_ITEM_SELECT,

    EVT_STAGE_IN,
    EVT_STAGE_OUT,
    EVT_HOVER_IN,
    EVT_HOVER_OUT,
    EVT_FOCUS_IN,
    EVT_FOCUS_OUT
};

struct event_object_t {
};

struct event_t {
    int type;
    int x;
    int y;
    int w;
    int h;
    int sx;
    int sy;
    int key;
    int mod;
    int button;
    int clicks;
    std::string text;
    std::string extra;
    event_object_t* source;
    bool cancelled;
};

typedef std::vector<event_t> event_list;
typedef std::function<bool(event_t&)> event_callback_t;
typedef std::map<int, std::vector<event_callback_t>> event_callback_list;

struct events_manager_t {
    static events_manager_t* instance();

    int on(event_type_e event_type, event_callback_t callback);
    void dispatch_event(event_t& event);
    void dispatch_events(event_list& events);
    event_callback_list callbacks;
};

#endif // EVENT_H