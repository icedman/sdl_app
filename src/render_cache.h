#ifndef RENDERER_CACHE_H
#define RENDERER_CACHE_H

#include "renderer.h"

typedef struct RenCache RenCache;

void rencache_show_debug(bool enable);
void rencache_free_font(RenFont* font);
void rencache_set_clip_rect(RenRect rect);
void rencache_draw_image(RenImage* image, RenRect rect);
void rencache_draw_rect(RenRect rect, RenColor color, bool fill = true, float l = 1.0f);
int rencache_draw_text(RenFont* font, const char* text, int x, int y, RenColor color, bool bold = false, bool italic = false, bool fixed = false);
void rencache_invalidate(void);
void rencache_begin_frame(RenCache *cache = 0);
void rencache_end_frame(void);

#endif // RENDERER_CACHE_H