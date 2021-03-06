#ifndef FX_SYSTEM_H
#define FX_SYSTEM_H

#include "events.h"
#include "renderer.h"

enum cursor_e {
    ARROW,
    RESIZE_EW,
    RESIZE_NS
};

struct window_renderer_t : renderer_t {
    void begin_frame();
    void end_frame();
};

struct system_t {
    static system_t* instance();

    bool init();
    void shutdown();
    void quit();

    int poll_events(event_list* events = NULL, bool wait = false);

    int target_fps();
    bool is_running();
    bool is_idle();
    void delay(int ms);

    void caffeinate();
    bool is_caffeinated();

    void set_cursor(cursor_e cur);

    struct timer_t {
        void begin();
        int now();
        int elapsed();

        int start_millis;
    };

    struct stats {
        int fps;
    } stats;

    int key_mods();

    window_renderer_t renderer;
    timer_t timer;
};

#endif FX_SYSTEM_H