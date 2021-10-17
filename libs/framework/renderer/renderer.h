#ifndef RENDERER_H
#define RENDERER_H

#include "color.h"
#include "font.h"
#include "rect.h"

#include <string>
#include <vector>

struct context_t {
    int width;
    int height;
};

struct image_t : context_t {
    std::string path;
    int ref;
};

struct text_span_t {
    int start;
    int length;
    color_t fg;
    color_t bg;
    bool bold;
    bool italic;
    bool underline;
    int caret;

    bool equals(text_span_t& b)
    {
        return (fg.r == b.fg.r && fg.b == b.fg.b && fg.a == b.fg.a) && (bg.r == b.bg.r && bg.b == b.bg.b && bg.a == b.bg.a) && bold == b.bold && italic == b.italic && underline == b.underline;
    }
};

struct renderer_t {
    renderer_t();
    ~renderer_t();

    void init(int w, int h);
    void shutdown();

    int width();
    int height();

    image_t* create_image(int w, int h);
    image_t* create_image_from_svg(std::string path, int w, int h);
    image_t* create_image_from_png(std::string path);
    static void destroy_image(image_t*);

    font_t* create_font(std::string name, int size);
    static void destroy_font(font_t* font);
    void set_default_font(font_t* font);
    font_t* default_font();

    context_t* create_context(int w, int h);
    void destroy_context(context_t* context);

    void clear(color_t clr);
    void draw_rect(rect_t rect, color_t clr, bool fill = true, int stroke = 0, color_t stroke_clr = { 0, 0, 0, 0 }, int radius = 0);
    void draw_line(int x, int y, int x2, int y2, color_t clr, int stroke = 1);
    void draw_image(image_t* image, rect_t rect, color_t clr = { 0, 0, 0, 0 });
    void draw_image(image_t* image, int x, int y, color_t clr = { 0, 0, 0, 0 });
    int draw_text(font_t* font, char* text, int x, int y, color_t clr = { 0, 0, 0, 0 }, bool bold = false, bool italic = false, bool underline = false);

    void begin_text_span(std::vector<text_span_t> spans);
    void end_text_span();

    void push_state();
    void pop_state();
    void set_clip_rect(rect_t rect);
    void rotate(float deg);
    void translate(int x, int y);
    void scale(float sx, float sy = 0);

    virtual void begin_frame();
    virtual void end_frame();

    void set_update_rects(rect_t* rects, int count);
    int draw_count();

    context_t* context;
    int _draw_count;
    rect_t* update_rects;
    int update_rects_count;
    bool enable_update_rects;

    std::vector<text_span_t> text_spans;
    int text_span_idx;
};

text_span_t span_from_index(std::vector<text_span_t>& spans, int idx);

#endif // RENDERER_H