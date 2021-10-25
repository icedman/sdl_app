#include "system.h"

#include <SDL2/SDL.h>
#include <chrono>

static system_t global_system;
static SDL_Window* window = 0;
static bool should_end = false;
static int idle_counter = 0;
static int caffeine_counter = 0;
static int mouse_x = -1;
static int mouse_y = -1;

static SDL_Cursor* cursor_arrow = 0;
static SDL_Cursor* cursor_resize_ew = 0;
static SDL_Cursor* cursor_resize_ns = 0;

#define MAX_UPDATE_RECTS 2048
#define FRAME_RATE 30
#define IDLE_FRAMES (FRAME_RATE * 2)
#define CAFFEINE_FRAMES (FRAME_RATE * 4)

extern SDL_Surface* ctx_sdl_surface(context_t* ctx);

void window_renderer_t::begin_frame()
{
    renderer_t::begin_frame();
}

void window_renderer_t::end_frame()
{
    SDL_Surface* window_surface = SDL_GetWindowSurface(window);

    if (enable_update_rects) {
        rect_t rects[MAX_UPDATE_RECTS];
        int rects_count = 0;
        for (int i = 0; i < update_rects_count; i++) {
            rect_t d = update_rects[i];

            if (d.x >= window_surface->w)
                continue;
            if (d.y >= window_surface->h)
                continue;
            if (d.x + d.w > window_surface->w) {
                d.w = window_surface->w - d.x;
            }
            if (d.y + d.h > window_surface->h) {
                d.h = window_surface->h - d.y;
            }

            rects[rects_count++] = d;
            if (rects_count >= MAX_UPDATE_RECTS) {
                break;
            }

            SDL_BlitSurface(ctx_sdl_surface(context.get()), (SDL_Rect*)&d, window_surface, (SDL_Rect*)&d);
        }
        SDL_UpdateWindowSurfaceRects(window, (SDL_Rect*)rects, rects_count);
    } else {
        SDL_BlitSurface(ctx_sdl_surface(context.get()), NULL, window_surface, NULL);
        SDL_UpdateWindowSurface(window);
    }

    renderer_t::end_frame();

    static bool firstShow = false;
    if (!firstShow) {
        SDL_ShowWindow(window);
        firstShow = true;
    }
}

int system_t::timer_t::now()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return millis;
}

void system_t::timer_t::begin()
{
    start_millis = now();
}

int system_t::timer_t::elapsed()
{
    return now() - start_millis;
}

system_t* system_t::instance()
{
    return &global_system;
}

bool system_t::init()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_EnableScreenSaver();
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    atexit(SDL_Quit);

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);

    int width = dm.w * 0.75;
    int height = dm.h * 0.75;

    window = SDL_CreateWindow(
        "", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    
    renderer.init(width, height);

    SDL_GetMouseState(&mouse_x, &mouse_y);

    stats.fps = 0;
    return true;
}

void system_t::shutdown()
{
    renderer.shutdown();
    SDL_Quit();
}

void system_t::quit()
{
    should_end = true;
}

int system_t::target_fps()
{
    return FRAME_RATE;
}

bool system_t::is_running()
{
    return !should_end;
}

bool system_t::is_idle()
{
    return idle_counter > IDLE_FRAMES;
}

void system_t::caffeinate()
{
    caffeine_counter = CAFFEINE_FRAMES;
    idle_counter = 0;
}

bool system_t::is_caffeinated()
{
    if (caffeine_counter > 0) {
        caffeine_counter--;
        return true;
    }
    return false;
}

void system_t::delay(int ms)
{
    struct timespec waittime;
    waittime.tv_sec = (ms / 1000);
    ms = ms % 1000;
    waittime.tv_nsec = ms * 1000 * 1000;
    nanosleep(&waittime, NULL);
}

std::string previousKeySequence;
int prevKeyTime;
int keyMods;

static inline std::string to_mods(int mods)
{
    std::string mod;
    int _mod = 0;
    if (mods & KMOD_CTRL) {
        mod = "ctrl";
        _mod = K_MOD_CTRL;
    }
    if (mods & KMOD_SHIFT) {
        if (mod.length())
            mod += "+";
        mod += "shift";
        _mod = K_MOD_SHIFT | _mod;
    }
    if (mods & KMOD_ALT) {
        if (mod.length())
            mod += "+";
        mod += "alt";
        _mod = K_MOD_ALT | _mod;
    }
    if (mods & KMOD_GUI) {
        if (mod.length())
            mod += "+";
        mod += "cmd";
        _mod = K_MOD_GUI | _mod;
    }
    keyMods = _mod;
    return mod;
}

int system_t::poll_events(event_list* events, bool wait)
{
    SDL_Event e;
    if (wait) {
        SDL_WaitEvent(&e);
    } else {
        if (!SDL_PollEvent(&e)) {
            idle_counter++;
            return 0;
        }
    }

    switch (e.type) {
    case SDL_QUIT:
        should_end = true;
        return 1;

    case SDL_MOUSEBUTTONDOWN:
        events->push_back({
            type : EVT_MOUSE_DOWN,
            x : e.button.x,
            y : e.button.y,
            button : e.button.button,
            clicks : e.button.clicks
        });
        idle_counter = 0;

        caffeinate();
        return 1;

    case SDL_MOUSEBUTTONUP:
        events->push_back({
            type : EVT_MOUSE_UP,
            x : e.button.x,
            y : e.button.y,
            button : e.button.button
        });
        idle_counter = 0;

        caffeinate();
        return 1;

    case SDL_MOUSEMOTION:
        events->push_back({
            type : EVT_MOUSE_MOTION,
            x : e.motion.x,
            y : e.motion.y,
            button : e.button.button
        });

        // idle_counter = 0.75f * IDLE_FRAMES;

        mouse_x = e.motion.x;
        mouse_y = e.motion.y;

        if (e.button.button) {
            caffeinate();
        }
        return 1;

    case SDL_MOUSEWHEEL:
        events->push_back({
            type : EVT_MOUSE_WHEEL,
            x : mouse_x,
            y : mouse_y,
            sx : e.wheel.x,
            sy : e.wheel.y
        });

        idle_counter = 0;
        caffeinate();
        return 1;

    case SDL_KEYUP:
        to_mods(e.key.keysym.mod);
        events->push_back({
            type : EVT_KEY_UP,
            key : e.key.keysym.sym,
            mod : keyMods
        });
        idle_counter = 0;

        caffeinate();
        return 1;

    case SDL_KEYDOWN: {

        caffeinate();

        std::string keySequence = SDL_GetKeyName(e.key.keysym.sym);
        std::string expandedSequence;
        std::string mod = to_mods(e.key.keysym.mod);

        // printf("%s : %s\n",
        //      SDL_GetScancodeName(e.key.keysym.scancode),
        //      SDL_GetKeyName(e.key.keysym.sym));

        std::transform(keySequence.begin(), keySequence.end(), keySequence.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (keySequence.find("left ") == 0 || keySequence.find("right ") == 0) {
            keySequence = "";
        }

        if (keySequence.length() && mod.length()) {
            keySequence = mod + "+" + keySequence;
        }

        int time_now = timer.now();
        if (previousKeySequence.length() && keySequence.length()) {
            if (time_now - prevKeyTime < 500) {
                expandedSequence = previousKeySequence + "+" + keySequence;
            }
            previousKeySequence = "";
            prevKeyTime = 0;
        }

        if (keySequence.length() > 1) {
            if (mod.length()) {
                previousKeySequence = keySequence;
                prevKeyTime = time_now;
            }
            events->push_back({
                type : EVT_KEY_SEQUENCE,
                text : keySequence,
                extra : expandedSequence
            });

            // printf("%s\n", expandedSequence.c_str());
            if (keySequence == "ctrl+q") {
                quit();
            }
            return 1;
        }

        events->push_back({
            type : EVT_KEY_DOWN,
            key : e.key.keysym.sym,
            mod : keyMods
        });

        idle_counter = 0;
        return 1;
    }

    case SDL_TEXTINPUT:
        events->push_back({
            type : EVT_KEY_TEXT,
            text : e.text.text
        });

        idle_counter = 0;

        caffeinate();
        return 1;

    case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
            if (e.window.data1 && e.window.data2) {

                event_t evt = {
                    type : EVT_WINDOW_RESIZE,
                    w : e.window.data1,
                    h : e.window.data2
                };

                renderer.init(evt.w, evt.h);
                events->push_back(evt);
            }

            caffeinate();
        }
        if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
            SDL_FlushEvent(SDL_KEYDOWN);
        }
        return 1;
    }

    return 0;
}

int system_t::key_mods()
{
    return keyMods;
}

void system_t::set_cursor(cursor_e cur)
{
    SDL_Cursor* cursor = 0;
    switch (cur) {
    case cursor_e::ARROW:
        if (!cursor_arrow) {
            cursor_arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        }
        cursor = cursor_arrow;
        break;
    case cursor_e::RESIZE_EW:
        if (!cursor_resize_ew) {
            cursor_resize_ew = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
        }
        cursor = cursor_resize_ew;
        break;
    case cursor_e::RESIZE_NS:
        if (!cursor_resize_ns) {
            cursor_resize_ns = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
        }
        cursor = cursor_resize_ns;
        break;
    }

    if (cursor) {
        SDL_SetCursor(cursor);
    }
}