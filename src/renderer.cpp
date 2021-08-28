#include "renderer.h"

#include <SDL2/SDL.h>
#include <cairo.h>
#include <pango/pangocairo.h>

static SDL_Window* window;
SDL_Surface* window_surface;

static struct {
    int left, top, right, bottom;
} clip;

struct RenImage {
    int width;
    int height;
    uint8_t* buffer;
    SDL_Surface* sdl_surface;
    cairo_surface_t* cairo_surface;
    cairo_t *cairo_context;
};

struct RenFont {
    int font_width;
    int font_height;
    PangoFontMap* font_map;
    PangoLayout* layout;
    PangoContext* context;
};

RenImage* window_buffer = 0;
RenImage* target_buffer = 0;
cairo_t* cairo_context = 0;
bool shouldEnd;

RenFont *default_font = 0;

RenImage* ren_create_image(int w, int h)
{
    RenImage* img = new RenImage();
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
    img->buffer =(uint8_t*) malloc(stride*h);
    img->width = w;
    img->height = h;
    img->sdl_surface = SDL_CreateRGBSurfaceFrom(img->buffer, w, h, 32, stride, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
    img->cairo_surface = cairo_image_surface_create_for_data(img->buffer, CAIRO_FORMAT_ARGB32, w, h, stride);
    img->cairo_context = cairo_create(img->cairo_surface);
    return img;
}

void ren_free_image(RenImage *img)
{
    SDL_FreeSurface(img->sdl_surface);
    cairo_surface_destroy(img->cairo_surface);
    cairo_destroy(img->cairo_context);
    free(img->buffer);
    delete img;
}

RenFont* ren_create_font(char *fdsc)
{
    RenFont *fnt = new RenFont();
    fnt->font_map = pango_cairo_font_map_get_default(); // pango-owned, don't delete
    fnt->context = pango_font_map_create_context(fnt->font_map);
    fnt->layout = pango_layout_new(fnt->context);

    PangoFontDescription* font_desc = pango_font_description_from_string(fdsc);
    pango_layout_set_font_description(fnt->layout, nullptr);   // pango will silently not make a copy if
    // the new fontdesc matches the old one
    // whether by pointer or deep compare.
    // this makes sure the old one is deleted
    // in favor of a new copy
    pango_layout_set_font_description(fnt->layout, font_desc);
    pango_font_map_load_font(fnt->font_map, fnt->context, font_desc);  // most examples do this, but does not appear
    // to actually be necessary? There's not much
    // info out there getting detailed on Pango.

    pango_font_description_free(font_desc); 

    const char text[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789";
    int len = strlen(text);
    int w, h;
    ren_get_font_extents(fnt, &w, &h, text, len);

    fnt->font_width = (w/len);
    fnt->font_height = h;

    default_font = fnt;
    return fnt;
}

void ren_destroy_font(RenFont *font)
{
    // pango_layout_free(fnt->layout);??
    // pango_font_map_context_free(fnt->context);??
    delete font;
}

void _destroy_cairo_context()
{
    if (!window_buffer) {
        return;
    }

    ren_free_image(window_buffer);
    window_buffer = 0;
}

cairo_t* _create_cairo_context(int width, int height)
{
    if (window_buffer) {
        _destroy_cairo_context();
    }
    window_buffer = ren_create_image(width, height);
    return window_buffer->cairo_context;
}

void ren_get_font_extents(RenFont *font, int *w, int *h, const char *text, int len, bool fixed)
{
    if (!font) {
        font = default_font;
    }

    if (fixed) {
        *w = font->font_width * len;
        *h = font->font_height;
        return;
    }
    pango_layout_set_attributes(font->layout, nullptr);
    pango_layout_set_text(font->layout, text, len);
    pango_layout_get_pixel_size(font->layout, w, h);
}

void ren_set_default_font(RenFont *font) {
    default_font = font;
}

RenFont* ren_get_default_font()
{
    return default_font;
}

void ren_get_size(int *w, int *h)
{
    *w = target_buffer->width;
    *h = target_buffer->height;
}

void ren_init()
{
    printf("initialize\n");

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_EnableScreenSaver();
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    // SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    atexit(SDL_Quit);

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);

    int width = dm.w * 0.75;
    int height = dm.h * 0.75;

    window = SDL_CreateWindow(
                 "", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
                 SDL_WINDOW_RESIZABLE);

    _create_cairo_context(width, height);
    shouldEnd = false;
}

void ren_shutdown()
{
    SDL_Quit();
    printf("shutdown\n");
}

void ren_begin_frame(RenImage *target)
{
    if (target) {
        target_buffer = target;
        cairo_context = target->cairo_context;
    } else {
        target_buffer = window_buffer;
        cairo_context = window_buffer->cairo_context;
    }
    // cairo_set_source_rgb(cairo_context, 0.5f, 0.5f, 0.5f);
    // cairo_paint(cairo_context);
}

void ren_end_frame()
{
    SDL_Surface* window_surface = SDL_GetWindowSurface(window);
    SDL_BlitSurface(target_buffer->sdl_surface, nullptr, window_surface, nullptr);
    SDL_UpdateWindowSurface(window);
}

void ren_quit()
{
    shouldEnd = true;
}

bool ren_is_running()
{
    return !shouldEnd;
}

void ren_draw_rect(RenRect rect, RenColor clr, bool fill, float l)
{
    cairo_set_source_rgb(cairo_context, clr.r/255.0f, clr.g/255.0f, clr.b/255.0f);
    cairo_rectangle(cairo_context, rect.x, rect.y, rect.width, rect.height);
    if (fill) {
        cairo_fill(cairo_context);
    } else {
        cairo_set_line_width(cairo_context, l);
        cairo_stroke(cairo_context);
    }
}

int ren_draw_text(RenFont* font, const char* text, int x, int y, RenColor clr, bool bold, bool italic, bool fixed_width)
{
    if (!font) {
        font = default_font;
    }
    int length = strlen(text);
    cairo_set_source_rgb(cairo_context, clr.r/255.0f, clr.g/255.0f, clr.b/255.0f);

    if (!fixed_width) {
        pango_layout_set_text(font->layout, text, length);
        cairo_move_to(cairo_context, x, y);
        pango_cairo_show_layout(cairo_context, font->layout);
        return 0;
    }

    char tmp[2];
    tmp[2] = 0;
    for(int i=0; i<length; i++) {
        tmp[0] = text[i];
        pango_layout_set_text(font->layout, tmp, 1);
        cairo_move_to(cairo_context, x, y);
        pango_cairo_show_layout(cairo_context, font->layout);
        x += font->font_width;
    }

    return x;
}

void ren_update_rects(RenRect* rects, int count)
{
    SDL_UpdateWindowSurfaceRects(window, (SDL_Rect*)rects, count);
}

void ren_set_clip_rect(RenRect rect)
{
    clip.left = rect.x;
    clip.top = rect.y;
    clip.right = rect.x + rect.width;
    clip.bottom = rect.y + rect.height;
}

void ren_listen_events(event_list* events)
{
    events->clear();

    SDL_Event e;

    if (!SDL_WaitEvent(&e)) {
        SDL_Delay(50);
        return;
    }

    switch (e.type) {
    case SDL_QUIT:
        shouldEnd = true;
        return;

    case SDL_MOUSEBUTTONDOWN:
        events->push_back({
            type: EVT_MOUSE_DOWN,
            x: e.button.x,
            y: e.button.y,
            button: e.button.button
        });
        return;

    case SDL_MOUSEBUTTONUP:
        events->push_back({
            type: EVT_MOUSE_UP,
            x: e.button.x,
            y: e.button.y,
            button: e.button.button
        });
        return;

    case SDL_MOUSEMOTION:
        events->push_back({
            type: EVT_MOUSE_MOTION,
            x: e.motion.x,
            y: e.motion.y,
            button: e.button.button
        });
        return;

    case SDL_KEYUP:
        events->push_back({
            type: EVT_KEY_UP,
            key: e.key.keysym.sym,
            mod: e.key.keysym.mod
        });
        return;

    case SDL_KEYDOWN:
        events->push_back({
            type: EVT_KEY_DOWN,
            key: e.key.keysym.sym,
            mod: e.key.keysym.mod
        });
        shouldEnd = true;
        return;

    case SDL_TEXTINPUT:
        events->push_back({
            type: EVT_KEY_TEXT,
            text: e.text.text
        });
        return;

    case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
            if (e.window.data1 && e.window.data2) {
                event_t evt = {
                    type: EVT_WINDOW_RESIZE,
                    w: e.window.data1,
                    h: e.window.data2
                };
                window_buffer->width = evt.w;
                window_buffer->height = evt.h;
                _create_cairo_context(evt.w, evt.h);
                events->push_back(evt);
            }
        }
        return;
    }

}