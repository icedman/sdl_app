#include "renderer.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cairo.h>
#include <librsvg/rsvg.h>

#include "app.h"

static int _state = 0;
static int keyMods = 0;

static SDL_Window* window;
static SDL_Surface* window_surface;

static RenRect* update_rects = 0;
static int update_rects_count = 0;

int items_drawn = 0;

static inline bool rects_overlap(RenRect a, RenRect b)
{
    return b.x + b.width >= a.x && b.x <= a.x + a.width
        && b.y + b.height >= a.y && b.y <= a.y + a.height;
}

struct RenImage {
    int width;
    int height;
    uint8_t* buffer;
    SDL_Surface* sdl_surface;
    cairo_surface_t* cairo_surface;
    cairo_t* cairo_context;
    cairo_pattern_t* pattern;
    std::string path;
};

static std::vector<RenImage*> images;

static RenImage* window_buffer = 0;
static RenImage* target_buffer = 0;
static cairo_t* cairo_context = 0;
static bool shouldEnd;

static RenFont* default_font = 0;

static int last_millis = 0;

static std::map<int, color_info_t> color_map;

cairo_t* ren_context()
{
    return cairo_context;
}

cairo_t* ren_image_context(RenImage* image)
{
    return image->cairo_context;
}

cairo_surface_t* ren_image_surface(RenImage* image)
{
    return image->cairo_surface;
}

cairo_pattern_t* ren_image_pattern(RenImage* image)
{
    return image->pattern;
}

void _destroy_cairo_context()
{
    if (!window_buffer) {
        return;
    }

    Renderer::instance()->destroy_image(window_buffer);
    window_buffer = 0;
}

cairo_t* _create_cairo_context(int width, int height)
{
    if (window_buffer) {
        _destroy_cairo_context();
    }
    window_buffer = Renderer::instance()->create_image(width, height);
    return window_buffer->cairo_context;
}

std::vector<RenImage*> context_stack;
void _set_context_from_stack()
{
    RenImage* target = context_stack.size() > 0 ? context_stack.back() : 0;
    if (target) {
        target_buffer = target;
        cairo_context = target->cairo_context;
    } else {
        target_buffer = window_buffer;
        cairo_context = window_buffer->cairo_context;
    }
}

void _blit_to_window()
{
    SDL_Surface* window_surface = SDL_GetWindowSurface(window);
    SDL_BlitSurface(target_buffer->sdl_surface, nullptr, window_surface, nullptr);

    if (update_rects_count) {
        SDL_UpdateWindowSurfaceRects(window, (SDL_Rect*)update_rects, update_rects_count);
        update_rects_count = 0;
    }

    SDL_UpdateWindowSurface(window);
}

static bool is_firable(char c)
{
    const char firs[] = "<>-=!:+|_&";
    for (int i = 0; firs[i] != 0; i++) {
        if (firs[i] == c)
            return true;
    }
    return false;
}

std::string to_mods(int mods)
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

static Renderer theRenderer;

Renderer* Renderer::instance()
{
    return &theRenderer;
}

void Renderer::init()
{
    printf("initialize\n");

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
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_OPENGL);

    _create_cairo_context(width, height);
    shouldEnd = false;

    // rencache_init();
    update_colors();
}

void Renderer::shutdown()
{
    // rencache_shutdown();

    destroy_images();
    destroy_fonts();

    SDL_Quit();
    printf("shutdown\n");
}

void Renderer::show_debug(bool enable)
{
}

void Renderer::quit()
{
    shouldEnd = true;
}

bool Renderer::is_running()
{
    return !shouldEnd;
}

void Renderer::get_window_size(int* w, int* h)
{
    *w = window_buffer->width;
    *h = window_buffer->height;
}

void Renderer::listen_events(event_list* events)
{
    {
        events->clear();

        SDL_Event e;

        // todo ... 
        if (is_throttle_up_events()) {
            if (!SDL_PollEvent(&e)) {
                return;
            }
        } else {
            if (!SDL_WaitEvent(&e)) {
                return;
            }
        }

        // throttle_up_events(12);

        switch (e.type) {
        case SDL_QUIT:
            shouldEnd = true;
            return;

        case SDL_MOUSEBUTTONDOWN:
            events->push_back({
                type : EVT_MOUSE_DOWN,
                x : e.button.x,
                y : e.button.y,
                button : e.button.button,
                clicks : e.button.clicks
            });
            return;

        case SDL_MOUSEBUTTONUP:
            events->push_back({
                type : EVT_MOUSE_UP,
                x : e.button.x,
                y : e.button.y,
                button : e.button.button
            });
            return;

        case SDL_MOUSEMOTION:
            events->push_back({
                type : EVT_MOUSE_MOTION,
                x : e.motion.x,
                y : e.motion.y,
                button : e.button.button
            });
            return;

        case SDL_MOUSEWHEEL:
            events->push_back({
                type : EVT_MOUSE_WHEEL,
                x : e.wheel.x,
                y : e.wheel.y
            });

            throttle_up_events(24);
            return;

        case SDL_KEYUP:
            to_mods(e.key.keysym.mod);
            events->push_back({
                type : EVT_KEY_UP,
                key : e.key.keysym.sym,
                mod : keyMods
            });

            return;

        case SDL_KEYDOWN: {
            std::string keySequence = SDL_GetKeyName(e.key.keysym.sym);
            std::string mod = to_mods(e.key.keysym.mod);

            // printf("%s : %s\n",
            //      SDL_GetScancodeName(e.key.keysym.scancode),
            //      SDL_GetKeyName(e.key.keysym.sym));

            std::transform(keySequence.begin(), keySequence.end(), keySequence.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (keySequence.length() && mod.length()) {
                keySequence = mod + "+" + keySequence;
            }

            if (keySequence.length() > 1) {
                events->push_back({
                    type : EVT_KEY_SEQUENCE,
                    text : keySequence
                });

                if (keySequence == "ctrl+q") {
                    quit();
                }
                return;
            }

            events->push_back({
                type : EVT_KEY_DOWN,
                key : e.key.keysym.sym,
                mod : keyMods
            });

            return;
        }
        case SDL_TEXTINPUT:
            events->push_back({
                type : EVT_KEY_TEXT,
                text : e.text.text
            });
            return;

        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                if (e.window.data1 && e.window.data2) {
                    event_t evt = {
                        type : EVT_WINDOW_RESIZE,
                        w : e.window.data1,
                        h : e.window.data2
                    };
                    window_buffer->width = evt.w;
                    window_buffer->height = evt.h;
                    _create_cairo_context(evt.w, evt.h);
                    events->push_back(evt);
                }
            }
            if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                SDL_FlushEvent(SDL_KEYDOWN);
            }
            return;
        }
    }
}

void Renderer::wake()
{
    if (is_throttle_up_events()) {
        return;
    }

    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.code = 0;
    event.user.data1 = 0;
    event.user.data2 = 0;
    SDL_PushEvent(&event);

    throttle_up_events(12);
}

int Renderer::key_mods()
{
    return keyMods;
}

RenImage* Renderer::create_image(int w, int h)
{
    RenImage* img = new RenImage();
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
    img->buffer = (uint8_t*)malloc(stride * h);
    memset(img->buffer, 0, stride * h);
    img->width = w;
    img->height = h;
    img->sdl_surface = SDL_CreateRGBSurfaceFrom(img->buffer, w, h, 32, stride, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
    img->cairo_surface = cairo_image_surface_create_for_data(img->buffer, CAIRO_FORMAT_ARGB32, w, h, stride);
    img->cairo_context = cairo_create(img->cairo_surface);
    img->pattern = cairo_pattern_create_for_surface(img->cairo_surface);
    images.push_back(img);
    return img;
}

RenImage* Renderer::create_image_from_svg(char* filename, int w, int h)
{
    for (auto img : images) {
        if (img->path == filename) {
            return img;
        }
    }

    RenImage* img = create_image(w, h);
    img->path = filename;

    RsvgHandle* svg = rsvg_handle_new_from_file(filename, 0);
    if (svg) {
        rsvg_handle_render_cairo(svg, img->cairo_context);
        rsvg_handle_free(svg);
    }
    return img;
}

RenImage* Renderer::create_image_from_png(char* filename, int w, int h)
{
    return 0;
}

void Renderer::destroy_image(RenImage* img)
{
    std::vector<RenImage*>::iterator it = std::find(images.begin(), images.end(), img);
    if (it != images.end()) {
        images.erase(it);
        cairo_pattern_destroy(img->pattern);
        cairo_surface_destroy(img->cairo_surface);
        cairo_destroy(img->cairo_context);
        free(img->buffer);
        SDL_FreeSurface(img->sdl_surface);
        delete img;
    }
}

void Renderer::destroy_images()
{
    std::vector<RenImage*> _images = images;
    for (auto img : _images) {
        destroy_image(img);
    }
}

void Renderer::image_size(RenImage* image, int* w, int* h)
{
    *w = image->width;
    *h = image->height;
}

void Renderer::save_image(RenImage* image, char* filename)
{
    cairo_surface_write_to_png(image->cairo_surface, filename);
}

RenFont* Renderer::get_default_font()
{
    return default_font;
}

void Renderer::set_default_font(RenFont* font)
{
    default_font = font;
}

void Renderer::update_rects(RenRect* rects, int count)
{
    ::update_rects = rects;
    update_rects_count = count;
}

void Renderer::set_clip_rect(RenRect rect)
{
    cairo_rectangle(cairo_context, rect.x, rect.y, rect.width, rect.height);
    cairo_clip(cairo_context);
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
    items_drawn++;
    cairo_save(cairo_context);
    cairo_translate(cairo_context, rect.x, rect.y);
    cairo_scale(cairo_context,
        (double)rect.width / image->width,
        (double)rect.height / image->height);

    if (clr.a == 0) {
        cairo_set_source_rgba(cairo_context, clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f, 1.0f);
        cairo_mask(cairo_context, image->pattern);
    } else {
        cairo_set_source_surface(cairo_context, image->cairo_surface, 0, 0);
        cairo_paint(cairo_context);

        /*
        cairo_set_source(cairo_context, image->pattern);
        cairo_pattern_set_extend(cairo_get_source(cairo_context), CAIRO_EXTEND_NONE);
        cairo_rectangle(cairo_context, 0, 0, image->width, image->height);
        cairo_fill(cairo_context);
        */
    }
    cairo_restore(cairo_context);
}

void Renderer::draw_underline(RenRect rect, RenColor color)
{
    rect.y += rect.height - 1;
    rect.height = 1;
    draw_rect(rect, color, true, 1.0f);
}

void Renderer::draw_rect(RenRect rect, RenColor clr, bool fill, int stroke, int rad)
{
    items_drawn++;
    double border = (double)stroke / 2;
    if (clr.a > 0) {
        cairo_set_source_rgba(cairo_context, clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f, clr.a / 255.0f);
    } else {
        cairo_set_source_rgb(cairo_context, clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f);
    }

    if (rad == 0) {
        cairo_rectangle(cairo_context, rect.x, rect.y, rect.width, rect.height);
    } else {
        // path
        double aspect = 1.0f;
        double radius = rad / aspect;
        double degrees = M_PI / 180.0;

        int x = rect.x;
        int y = rect.y;
        int width = rect.width;
        int height = rect.height;
        cairo_new_sub_path(cairo_context);
        cairo_arc(cairo_context, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
        cairo_arc(cairo_context, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
        cairo_arc(cairo_context, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
        cairo_arc(cairo_context, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
        cairo_close_path(cairo_context);
    }

    if (fill) {
        cairo_fill(cairo_context);
    } else {
        cairo_set_line_width(cairo_context, border);
        cairo_stroke(cairo_context);
    }
}

int Renderer::draw_char(RenFont* font, char ch, int x, int y, RenColor color, bool bold, bool italic, bool underline)
{
    return 0;
}

void Renderer::begin_frame(RenImage* image, int w, int h, RenCache* cache)
{
    if (!color_map.size()) {
        update_colors();
    }

    context_stack.push_back(image);
    _set_context_from_stack();
    items_drawn = 0;
    // cairo_set_antialias(cairo_context, CAIRO_ANTIALIAS_BEST);
}

void Renderer::end_frame()
{
    _set_context_from_stack();

    if (target_buffer == window_buffer) {
        if (_state != 0) {
            printf("warning: states stack at %d\n", _state);
        }
        _blit_to_window();
    }

    context_stack.pop_back();
    _set_context_from_stack();
}

void Renderer::state_save()
{
    cairo_save(cairo_context);
    _state++;
}

void Renderer::state_restore()
{
    cairo_restore(cairo_context);
    _state--;
}

int Renderer::draw_count()
{
    return items_drawn;
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
    SDL_Delay(d);
}

std::string Renderer::get_clipboard()
{
    if (!SDL_HasClipboardText()) {
        return "";
    }
    std::string res = SDL_GetClipboardText();
    ;
    return res;
}

void Renderer::set_clipboard(std::string text)
{
    SDL_SetClipboardText(text.c_str());
}

bool Renderer::is_terminal()
{
    return false;
}

color_info_t Renderer::color_for_index(int index)
{
    return color_map[index];
}

void Renderer::update_colors()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    auto it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        color_info_t fg = it->second;
        fg.red = fg.red <= 1 ? fg.red * 255 : fg.red;
        fg.green = fg.green <= 1 ? fg.green * 255 : fg.green;
        fg.blue = fg.blue <= 1 ? fg.blue * 255 : fg.blue;
        color_map[it->second.index] = fg;
        it++;
    }
}
