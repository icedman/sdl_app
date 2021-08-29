#ifndef RENDERER_CACHE_H
#define RENDERER_CACHE_H

#include "renderer.h"

typedef struct RenCache RenCache;

RenCache* ren_create_cache();
void ren_destroy_cache(RenCache *cache);

void rencache_init();
void rencache_shutdown();
void rencache_show_debug(bool enable);
void rencache_free_font(RenFont* font);
void rencache_set_clip_rect(RenRect rect);
void rencache_draw_image(RenImage* image, RenRect rect);
void rencache_draw_rect(RenRect rect, RenColor color, bool fill = true, float l = 1.0f);
int rencache_draw_text(RenFont* font, const char* text, int x, int y, RenColor color, bool bold = false, bool italic = false, bool fixed = false);
void rencache_invalidate(void);
void rencache_begin_frame(int w, int h, RenCache *cache = 0);
void rencache_end_frame(void);

void rencache_state_save();
void rencache_state_restore();

#endif // RENDERER_CACHE_H