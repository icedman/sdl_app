#ifndef RENDERER_H
#define RENDERER_H

#include "renderer.h"
#include "theme.h"

#include "events.h"
#include <stdint.h>

#define K_MOD_SHIFT 1 << 1
#define K_MOD_CTRL 1 << 2
#define K_MOD_ALT 1 << 3
#define K_MOD_GUI 1 << 4

typedef struct RenImage RenImage;
typedef struct RenFont RenFont;
typedef struct RenCache RenCache;

typedef struct {
    uint8_t r, g, b, a;
} RenColor;
typedef struct {
    int x, y, width, height;
} RenRect;

struct Renderer {
    static Renderer* instance();

    void init();
    void shutdown();
    void show_debug(bool enable);

    void quit();
    bool is_running();
    void get_window_size(int* w, int* h);

    void listen_events(event_list* events);
    void throttle_up(int frames = 120);
    bool is_throttle_up();

    int key_mods();

    RenImage* create_image(int w, int h);
    RenImage* create_image_from_svg(char* filename, int w, int h);
    RenImage* create_image_from_png(char* filename, int w, int h);
    void destroy_image(RenImage* image);
    void destroy_images();
    void image_size(RenImage* image, int* w, int* h);
    void save_image(RenImage* image, char* filename);

    void register_font(char* path);
    RenFont* create_font(char* font_desc, char* alias = 0);
    RenFont* font(char* alias);
    void destroy_font(RenFont* font);
    void destroy_fonts();
    RenFont* get_default_font();
    void set_default_font(RenFont* font);
    void get_font_extents(RenFont* font, int* w, int* h, const char* text = 0, int len = 0);

    void update_rects(RenRect* rects, int count);
    void set_clip_rect(RenRect rect);
    void invalidate_rect(RenRect rect);
    void draw_image(RenImage* image, RenRect rect, RenColor clr = { 255, 255, 255, 255 });
    void draw_rect(RenRect rect, RenColor color, bool fill = true, int stroke = 1, int radius = 0);
    int draw_text(RenFont* font, const char* text, int x, int y, RenColor color, bool bold = false, bool italic = false);
    int draw_char(RenFont* font, char ch, int x, int y, RenColor color, bool bold = false, bool italic = false);
    void invalidate();
    void begin_frame(RenImage* image = 0, int w = 0, int h = 0, RenCache* cache = 0);
    void end_frame();
    void state_save();
    void state_restore();

    int draw_count();

    std::string get_clipboard();
    void set_clipboard(std::string text);

    bool is_terminal();

    color_info_t color_for_index(int index);
    void update_colors();
};

#endif // RENDERER_H