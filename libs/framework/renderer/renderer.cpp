#include "renderer.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cairo.h>

#ifndef M_PI
#define M_PI 3.14159f
#endif

#ifdef ENABLE_SVG
#include <librsvg-2.0/librsvg/rsvg.h>
#endif

font_ptr pango_font_create(std::string name, int size);
bool pango_register_font(char* text);

font_ptr asteroid_font_create(std::string name, int size);

struct cairo_context_t : image_t {
    cairo_context_t();
    ~cairo_context_t();

    uint8_t* buffer;
    SDL_Surface* sdl_surface;
    cairo_surface_t* cairo_surface;
    cairo_t* cairo_context;
    cairo_pattern_t* pattern;
};

cairo_context_t::cairo_context_t()
{
}

cairo_context_t::~cairo_context_t()
{
    cairo_pattern_destroy(pattern);
    cairo_surface_destroy(cairo_surface);
    cairo_destroy(cairo_context);
    free(buffer);
    SDL_FreeSurface(sdl_surface);
}

SDL_Surface* ctx_sdl_surface(context_t* ctx)
{
    cairo_context_t* c = (cairo_context_t*)ctx;
    return c->sdl_surface;
}

cairo_t* ctx_cairo_context(context_t* ctx)
{
    cairo_context_t* c = (cairo_context_t*)ctx;
    return c->cairo_context;
}

cairo_surface_t* ctx_cairo_surface(context_t* ctx)
{
    cairo_context_t* c = (cairo_context_t*)ctx;
    return c->cairo_surface;
}

cairo_pattern_t* ctx_cairo_pattern(context_t* ctx)
{
    cairo_context_t* c = (cairo_context_t*)ctx;
    return c->pattern;
}

text_span_t span_from_index(std::vector<text_span_t>& spans, int idx, bool bg)
{
    text_span_t res = { 0, 0, { 255, 255, 255, 0 }, { 0, 0, 0, 0 }, false, false, false, 0 };
    for (auto s : spans) {
        if (idx >= s.start && idx < s.start + s.length) {

            if (bg) {
                if (s.caret == 0 && !color_is_set(s.bg)) {
                    continue;
                }
                if (color_is_set(s.bg)) {
                    res.bg = s.bg;
                }
                if (s.caret) {
                    res.caret = s.caret;
                }
            } else {
                if (s.caret != 0 || color_is_set(s.bg)) {
                    continue;
                }
            }

            res.start = s.start;
            res.length = s.length;
            res.bold |= s.bold;
            res.italic |= s.italic;
            res.underline |= s.underline;
            if (color_is_set(s.fg)) {
                res.fg = s.fg;
            }
        }
    }
    return res;
}

renderer_t::renderer_t()
    : _draw_count(0)
    , update_rects(0)
    , update_rects_count(0)
    , enable_update_rects(false)
{
    foreground = { 255, 255, 255 };
    background = { 50, 50, 50 };
}

renderer_t::~renderer_t()
{
}

void renderer_t::init(int w, int h)
{
    context = create_context(w, h);
}

int renderer_t::width()
{
    if (context)
        return context->width;
    return 0;
}

int renderer_t::height()
{
    if (context)
        return context->height;
    return 0;
}

void renderer_t::shutdown()
{
    context = nullptr;
    _default_font = nullptr;

#ifdef WATCH_LEAKS
    if (image_cache.size() || font_cache.size()) {
        printf("freeing %d images, %d fonts\n", image_cache.size(), font_cache.size());
    }
#endif

    image_cache.clear();
    font_cache.clear();
}

context_ptr renderer_t::create_context(int w, int h)
{
    return create_image(w, h);
}

image_ptr renderer_t::create_image(int w, int h)
{
    cairo_context_t* img = new cairo_context_t();
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
    img->buffer = (uint8_t*)malloc(stride * h);
    memset(img->buffer, 0, stride * h);
    img->width = w;
    img->height = h;
    img->sdl_surface = SDL_CreateRGBSurfaceFrom(img->buffer, w, h, 32, stride, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
    img->cairo_surface = cairo_image_surface_create_for_data(img->buffer, CAIRO_FORMAT_ARGB32, w, h, stride);
    img->cairo_context = cairo_create(img->cairo_surface);
    img->pattern = cairo_pattern_create_for_surface(img->cairo_surface);
    return image_ptr(img);
}

image_ptr renderer_t::create_image_from_svg(std::string path, int w, int h, std::string alias)
{
    for (auto f : image_cache) {
        if (f->path == path || (f->alias == alias && alias.length())) {
            return f;
        }
    }

    image_ptr img = create_image(w, h);
    img->path = path;
    img->alias = alias;

#ifdef ENABLE_SVG
    RsvgHandle* svg = rsvg_handle_new_from_file(path.c_str(), 0);
    if (svg) {

        double width;
        double height;
        rsvg_handle_get_intrinsic_size_in_pixels(svg, &width, &height);

        RsvgRectangle box;
        rsvg_handle_get_intrinsic_dimensions(svg, // RsvgHandle *handle,
            NULL, //gboolean *out_has_width,
            NULL, //RsvgLength *out_width,
            NULL, //gboolean *out_has_height,
            NULL, //RsvgLength *out_height,
            NULL, //gboolean *out_has_viewbox,
            &box); // RsvgRectangle *out_viewbox);

        if (width == 0) {
            width = box.width;
        }
        if (height == 0) {
            height = box.height;
        }

        cairo_t* ctx = ((cairo_context_t*)img.get())->cairo_context;

        float sx = (float)img->width / width;
        float sy = (float)img->height / height;
        float sz = (sx < sy) ? sx : sy;

        cairo_save(ctx);
        cairo_scale(ctx, sz, sz);

        // printf("%d %d %d %d\n", (int)box.width, (int)box.height, img->width, img->height);

        rsvg_handle_render_cairo(svg, ctx);
        rsvg_handle_free(svg);

        cairo_restore(ctx);
    }
#endif

    image_cache.push_back(img);
    return img;
}

image_ptr renderer_t::create_image_from_svg_data(std::string data, int w, int h, std::string alias)
{
    for (auto f : image_cache) {
        if (f->alias == alias && alias.length()) {
            return f;
        }
    }

    image_ptr img = create_image(w, h);
    img->path = alias;
    img->alias = alias;

#ifdef ENABLE_SVG
    RsvgHandle* svg = rsvg_handle_new_from_data((const uint8_t*)data.c_str(), data.length(), NULL);
    if (svg) {

        double width;
        double height;
        rsvg_handle_get_intrinsic_size_in_pixels(svg, &width, &height);

        RsvgRectangle box;
        rsvg_handle_get_intrinsic_dimensions(svg, // RsvgHandle *handle,
            NULL, //gboolean *out_has_width,
            NULL, //RsvgLength *out_width,
            NULL, //gboolean *out_has_height,
            NULL, //RsvgLength *out_height,
            NULL, //gboolean *out_has_viewbox,
            &box); // RsvgRectangle *out_viewbox);

        if (width == 0) {
            width = box.width;
        }
        if (height == 0) {
            height = box.height;
        }

        cairo_t* ctx = ((cairo_context_t*)img.get())->cairo_context;

        float sx = (float)img->width / width;
        float sy = (float)img->height / height;
        float sz = (sx < sy) ? sx : sy;
        cairo_save(ctx);
        cairo_scale(ctx, sz, sz);

        // printf("%d %d %d %d\n", (int)box.width, (int)box.height, img->width, img->height);

        rsvg_handle_render_cairo(svg, ctx);
        rsvg_handle_free(svg);

        cairo_restore(ctx);
    }
#endif

    image_cache.push_back(img);
    return img;
}

image_ptr renderer_t::create_image_from_png(std::string path, std::string alias)
{
    for (auto f : image_cache) {
        if (f->path == path || (f->alias == alias && alias.length())) {
            return f;
        }
    }

    cairo_surface_t* image = cairo_image_surface_create_from_png(path.c_str());
    if (!image) {
        return NULL;
    }

    image_ptr img = create_image(cairo_image_surface_get_width(image), cairo_image_surface_get_height(image));
    img->path = path;
    img->alias = alias;

    color_t clr = { 255, 0, 255 };
    cairo_t* cairo_context = ((cairo_context_t*)img.get())->cairo_context;
    cairo_set_source_surface(cairo_context, image, 0, 0);
    cairo_paint(cairo_context);

    cairo_surface_destroy(image);

    image_cache.push_back(img);
    return img;
}

image_ptr renderer_t::image(std::string alias)
{
    for (auto f : image_cache) {
        if (f->alias == alias) {
            return f;
        }
    }
    return nullptr;
}

bool renderer_t::register_font(std::string path)
{
    pango_register_font((char*)path.c_str());
    return true;
}

font_ptr renderer_t::create_font(std::string name, int size, std::string alias)
{
    for (auto f : font_cache) {
        if (f->name == name && (int)f->size == size) {
            return f;
        }
    }

    font_ptr fnt = nullptr;
    if (name == "asteroids") {
        fnt = asteroid_font_create(name, size);
    }
    if (!fnt) {
        fnt = pango_font_create(name, size);
    }
    fnt->alias = alias;
    font_cache.push_back(fnt);

    if (!_default_font) {
        _default_font = fnt;
    }

    return fnt;
}

void renderer_t::set_default_font(font_ptr font)
{
    _default_font = font;
}

font_ptr renderer_t::default_font()
{
    return _default_font;
}

font_ptr renderer_t::font(std::string alias)
{
    for (auto f : font_cache) {
        if (f->alias == alias) {
            return f;
        }
    }
    return nullptr;
}

void renderer_t::clear(color_t clr)
{
    if (!context)
        return;
    draw_rect({ 0, 0, context->width, context->height }, clr);
}

void renderer_t::draw_rect(rect_t rect, color_t clr, bool fill, int stroke, color_t stroke_clr, int radius)
{
    _draw_count++;

    if (!context)
        return;
    cairo_t* cairo_context = ((cairo_context_t*)context.get())->cairo_context;

    if (!fill && !color_is_set(stroke_clr)) {
        stroke_clr = clr;
    }

    double border = (double)stroke / 2;
    if (clr.a > 0) {
        cairo_set_source_rgba(cairo_context, clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f, clr.a / 255.0f);
    } else {
        cairo_set_source_rgb(cairo_context, clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f);
    }

    if (radius == 0) {
        cairo_rectangle(cairo_context, rect.x, rect.y, rect.w, rect.h);
    } else {
        // path
        double aspect = 1.0f;
        double rad = radius / aspect;
        double degrees = M_PI / 180.0f;

        int x = rect.x;
        int y = rect.y;
        int width = rect.w;
        int height = rect.h;
        cairo_new_sub_path(cairo_context);
        cairo_arc(cairo_context, x + width - rad, y + rad, rad, -90 * degrees, 0 * degrees);
        cairo_arc(cairo_context, x + width - rad, y + height - rad, rad, 0 * degrees, 90 * degrees);
        cairo_arc(cairo_context, x + rad, y + height - rad, rad, 90 * degrees, 180 * degrees);
        cairo_arc(cairo_context, x + rad, y + rad, rad, 180 * degrees, 270 * degrees);
        cairo_close_path(cairo_context);
    }

    if (fill) {
        cairo_fill(cairo_context);
    } else {
        cairo_set_line_width(cairo_context, border);
        cairo_stroke(cairo_context);
    }

    if (fill && stroke && color_is_set(stroke_clr)) {
        draw_rect(rect, stroke_clr, false, stroke, {}, radius);
    }
}

void renderer_t::draw_line(int x, int y, int x2, int y2, color_t clr, int stroke)
{
    _draw_count++;

    if (!context)
        return;
    cairo_t* cairo_context = ((cairo_context_t*)context.get())->cairo_context;

    if (clr.a > 0) {
        cairo_set_source_rgba(cairo_context, clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f, clr.a / 255.0f);
    } else {
        cairo_set_source_rgb(cairo_context, clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f);
    }

    double border = (double)stroke / 2;
    cairo_set_line_width(cairo_context, border);
    cairo_move_to(cairo_context, x, y);
    cairo_line_to(cairo_context, x2, y2);
    cairo_stroke(cairo_context);
}

void renderer_t::draw_image(image_t* image, rect_t rect, color_t clr)
{
    _draw_count++;

    if (!context)
        return;
    cairo_t* cairo_context = ((cairo_context_t*)context.get())->cairo_context;
    cairo_context_t* img = (cairo_context_t*)image;

    cairo_save(cairo_context);
    cairo_translate(cairo_context, rect.x, rect.y);
    cairo_scale(cairo_context,
        (double)rect.w / image->width,
        (double)rect.h / image->height);

    if (!color_is_set(clr)) {
        cairo_set_source_rgba(cairo_context, clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f, 1.0f);
        cairo_mask(cairo_context, img->pattern);
    } else {
        cairo_set_source_surface(cairo_context, img->cairo_surface, 0, 0);
        cairo_paint(cairo_context);

        /*
        cairo_set_source(cairo_context, img->pattern);
        cairo_pattern_set_extend(cairo_get_source(cairo_context), CAIRO_EXTEND_NONE);
        cairo_rectangle(cairo_context, 0, 0, image->width, image->height);
        cairo_fill(cairo_context);
        */
    }
    cairo_restore(cairo_context);
}

void renderer_t::draw_image(image_t* image, int x, int y, color_t clr)
{
    _draw_count++;

    rect_t rect = { x, y, image->width, image->height };
    draw_image(image, rect, clr);
}

int renderer_t::draw_text(font_t* font, char* text, int x, int y, color_t clr, bool bold, bool italic, bool underline)
{
    // _draw_count++;
    if (!font) {
        font = default_font().get();
    }

    if (!color_is_set(clr)) {
        clr = foreground;
    }

    return font->draw_text(this, font, text, x, y, clr, bold, italic, underline);
}

void renderer_t::begin_text_span(std::vector<text_span_t> spans)
{
    text_span_idx = 0;
    text_spans = spans;
}

void renderer_t::end_text_span()
{
    text_spans.clear();
}

void renderer_t::begin_frame()
{
    _draw_count = 0;
}

void renderer_t::end_frame()
{
    update_rects = 0;
    update_rects_count = 0;
    enable_update_rects = false;
}

void renderer_t::push_state()
{
    if (!context)
        return;
    cairo_t* cairo_context = ((cairo_context_t*)context.get())->cairo_context;
    cairo_save(cairo_context);
}

void renderer_t::pop_state()
{
    if (!context)
        return;
    cairo_t* cairo_context = ((cairo_context_t*)context.get())->cairo_context;
    cairo_restore(cairo_context);
}

void renderer_t::set_clip_rect(rect_t rect)
{
    if (!context)
        return;
    cairo_t* cairo_context = ((cairo_context_t*)context.get())->cairo_context;

    cairo_rectangle(cairo_context, rect.x, rect.y, rect.w, rect.h);
    cairo_clip(cairo_context);
}

void renderer_t::rotate(float deg)
{
    if (!context)
        return;
    cairo_t* cairo_context = ((cairo_context_t*)context.get())->cairo_context;
    cairo_rotate(cairo_context, deg * M_PI / 180.0f);
}

void renderer_t::translate(int x, int y)
{
    if (!context)
        return;
    cairo_t* cairo_context = ((cairo_context_t*)context.get())->cairo_context;
    cairo_translate(cairo_context, x, y);
}

void renderer_t::scale(float sx, float sy)
{
    if (!context)
        return;
    cairo_t* cairo_context = ((cairo_context_t*)context.get())->cairo_context;

    if (sy == 0) {
        sy = sx;
    }
    cairo_scale(cairo_context,
        (double)sx,
        (double)sy);
}

void renderer_t::set_update_rects(rect_t* rects, int count)
{
    update_rects = rects;
    update_rects_count = count;
    enable_update_rects = true;
}

int renderer_t::draw_count()
{
    return _draw_count;
}
