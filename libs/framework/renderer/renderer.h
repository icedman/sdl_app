#ifndef RENDERER_H
#define RENDERER_H

#include "color.h"
#include "font.h"
#include "rect.h"

#include <memory>
#include <string>
#include <vector>

struct image_t {
    int width;
    int height;
    std::string path;
};

typedef image_t context_t;
typedef std::shared_ptr<image_t> image_ptr;
typedef std::shared_ptr<image_t> context_ptr;

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

    image_ptr create_image(int w, int h);
    image_ptr create_image_from_svg(std::string path, int w, int h);
    image_ptr create_image_from_png(std::string path);

    font_ptr create_font(std::string name, int size, std::string alias = "");
    void set_default_font(font_ptr font);
    font_ptr default_font();
    font_ptr font(std::string alias);

    context_ptr create_context(int w, int h);

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

    context_ptr context;

    int _draw_count;
    rect_t* update_rects;
    int update_rects_count;
    bool enable_update_rects;

    color_t foreground;
    color_t background;

    std::vector<text_span_t> text_spans;
    int text_span_idx;

    std::vector<image_ptr> image_cache;
    std::vector<font_ptr> font_cache;
    font_ptr _default_font;
};

text_span_t span_from_index(std::vector<text_span_t>& spans, int idx, bool bg = false);

#endif // RENDERER_H