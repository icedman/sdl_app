#ifndef RENDERER_H
#define RENDERER_H

#include "events.h"
#include <stdint.h>

struct nk_context;

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
void ren_get_size(int *w, int *h);

void ren_update_rects(RenRect* rects, int count);
void ren_set_clip_rect(RenRect rect);

void ren_listen_events(event_list* events);

void ren_draw_rect(RenRect rect, RenColor clr = { 255, 255, 255 }, bool fill = true, float l = 1.0f);
int ren_draw_text(RenFont* font, const char* text, int x, int y, RenColor color, bool bold = false, bool italic = false, bool fixed = false);

RenImage* ren_create_image(int w, int h);
void ren_destroy_image(RenImage *image);

RenFont* ren_create_font(char *font_desc);
void ren_destroy_font(RenFont *font);
RenFont* ren_get_default_font();
void ren_set_default_font(RenFont *font);
void ren_get_font_extents(RenFont* font, int *w, int *h, const char *text = 0, int len = 0, bool fixed = false);

#endif // RENDERER_H