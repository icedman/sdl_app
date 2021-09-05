#ifndef RENDERER_CACHE_H
#define RENDERER_CACHE_H

#include "renderer.h"

#define ENABLE_RENDER_CACHE

typedef struct RenCache RenCache;

RenCache* ren_create_cache();
void ren_destroy_cache(RenCache *cache);

void rencache_init();
void rencache_shutdown();
void rencache_show_debug(bool enable);
void rencache_free_font(RenFont* font);
void rencache_set_clip_rect(RenRect rect);
void rencache_invalidate_rect(RenRect rect);
void rencache_draw_image(RenImage* image, RenRect rect, RenColor clr = { 255,255,255,255 });
void rencache_draw_rect(RenRect rect, RenColor color, bool fill = true, int stroke = 1, int radius = 0);
 int rencache_draw_text(RenFont* font, const char* text, int x, int y, RenColor color, bool bold = false, bool italic = false, bool fixed = false);
void rencache_invalidate(void);
void rencache_begin_frame(int w, int h, RenCache *cache = 0);
void rencache_end_frame(void);

void rencache_state_save();
void rencache_state_restore();

#ifdef ENABLE_RENDER_CACHE
#define draw_rect rencache_draw_rect
#define draw_text rencache_draw_text
#define draw_image rencache_draw_image
#define set_clip_rect rencache_set_clip_rect
#define state_save rencache_state_save
#define state_restore rencache_state_restore
#define begin_frame(w, h) ren_begin_frame(); rencache_begin_frame(w, h);
#define end_frame() rencache_end_frame(); ren_end_frame();
#else
#define draw_rect ren_draw_rect
#define draw_text ren_draw_text
#define draw_image ren_draw_image
#define set_clip_rect ren_set_clip_rect
#define state_save ren_state_save
#define state_restore ren_state_restore
#define begin_frame(w, h) ren_begin_frame();
#define end_frame() ren_end_frame();
#endif

#endif // RENDERER_CACHE_H