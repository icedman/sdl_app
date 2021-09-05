#ifndef RENDERER_H
#define RENDERER_H

#include "events.h"
#include <stdint.h>

#define K_MOD_SHIFT 1<<1
#define K_MOD_CTRL  1<<2
#define K_MOD_ALT   1<<3

typedef struct RenImage RenImage;
typedef struct RenFont RenFont;

typedef struct {
    uint8_t r, g, b, a;
} RenColor;
typedef struct {
    int x, y, width, height;
} RenRect;

void ren_init();
void ren_shutdown();
void ren_begin_frame(RenImage *target = 0);
void ren_end_frame();

void ren_quit();
bool ren_is_running();
void ren_get_window_size(int *w, int *h);

void ren_update_rects(RenRect* rects, int count);
void ren_set_clip_rect(RenRect rect);

void ren_listen_events(event_list* events);
void ren_listen_quick(int frames = 120);
bool ren_listen_is_quick();

RenImage* ren_create_image(int w, int h);
RenImage* ren_create_image_from_svg(char *filename, int w, int h);
// RenImage* ren_create_image_from_png(char *filename, int w, int h);
void ren_destroy_image(RenImage *image);
void ren_destroy_images();
void ren_image_size(RenImage *image, int *w, int *h);
void ren_save_image(RenImage *image, char *filename);

void ren_state_save();
void ren_state_restore();

RenFont* ren_create_font(char *font_desc, char *alias = 0);
RenFont* ren_font(char* alias);
void ren_destroy_font(RenFont *font);
void ren_destroy_fonts();
RenFont* ren_get_default_font();
void ren_set_default_font(RenFont *font);
void ren_get_font_extents(RenFont* font, int *w, int *h, const char *text = 0, int len = 0, bool fixed = false);

void ren_draw_image(RenImage *image, RenRect rect, RenColor clr = { 255,255,255,255 });
void ren_draw_rect(RenRect rect, RenColor clr = { 255, 255, 255 }, bool fill = true, float l = 1.0f);
int ren_draw_text(RenFont* font, const char* text, int x, int y, RenColor color, bool bold = false, bool italic = false, bool fixed = false);

std::string ren_get_clipboard();
void ren_set_clipboard(std::string text);

#endif // RENDERER_H