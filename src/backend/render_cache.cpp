#include "render_cache.h"

#include <stdio.h>
#include <string.h>

// from lite editor
/* a cache over the software renderer -- all drawing operations are stored as
** commands when issued. At the end of the frame we write the commands to a grid
** of hash values, take the cells that have changed since the previous frame,
** merge them into dirty rectangles and redraw only those regions */

#define CELLS_X 80
#define CELLS_Y 50
#define CELL_SIZE 96
#define COMMAND_BUF_SIZE (1024 * 512)

enum {
    FREE_FONT,
    SET_CLIP,
    DRAW_IMAGE,
    DRAW_TEXT,
    DRAW_RECT,
    SAVE_STATE,
    RESTORE_STATE
};

typedef struct {
    int type, size;
    RenRect rect;
    RenColor color;
    void* font; // image / radius
    int bold;   // fill
    int italic; // stroke
    char text[0];
} Command;

struct RenCache {
    unsigned cells_buf1[CELLS_X * CELLS_Y];
    unsigned cells_buf2[CELLS_X * CELLS_Y];
    unsigned* cells_prev = 0;
    unsigned* cells = 0;
    RenRect rect_buf[CELLS_X * CELLS_Y / 2];
    char command_buf[COMMAND_BUF_SIZE];
    int command_buf_idx;
    RenRect target_rect;
    bool show_debug;

    RenCache() {
        cells_prev = cells_buf1;
        cells = cells_buf2;
    }
};

RenCache default_cache;
RenCache *cache = 0;

RenCache* ren_create_cache()
{
    return new RenCache();
}

void ren_destroy_cache(RenCache *c)
{
    delete c;
}

static inline int min(int a, int b) { return a < b ? a : b; }
static inline int max(int a, int b) { return a > b ? a : b; }

/* 32bit fnv-1a hash */
#define HASH_INITIAL 2166136261

static void hash(unsigned* h, const void* data, int size)
{
    const unsigned char* p = (const unsigned char*)data;
    while (size--) {
        *h = (*h ^ *p++) * 16777619;
    }
}

static inline int cell_idx(int x, int y)
{
    return x + y * CELLS_X;
}

static inline bool rects_overlap(RenRect a, RenRect b)
{
    return b.x + b.width >= a.x && b.x <= a.x + a.width
        && b.y + b.height >= a.y && b.y <= a.y + a.height;
}

static RenRect intersect_rects(RenRect a, RenRect b)
{
    int x1 = max(a.x, b.x);
    int y1 = max(a.y, b.y);
    int x2 = min(a.x + a.width, b.x + b.width);
    int y2 = min(a.y + a.height, b.y + b.height);
    return (RenRect){ x1, y1, max(0, x2 - x1), max(0, y2 - y1) };
}

static RenRect merge_rects(RenRect a, RenRect b)
{
    int x1 = min(a.x, b.x);
    int y1 = min(a.y, b.y);
    int x2 = max(a.x + a.width, b.x + b.width);
    int y2 = max(a.y + a.height, b.y + b.height);
    return (RenRect){ x1, y1, x2 - x1, y2 - y1 };
}

static Command* push_command(int type, int size)
{
    Command* cmd = (Command*)(cache->command_buf + cache->command_buf_idx);
    int n = cache->command_buf_idx + size;
    if (n > COMMAND_BUF_SIZE) {
        fprintf(stderr, "Warning: (" __FILE__ "): exhausted command buffer\n");
        return NULL;
    }
    cache->command_buf_idx = n;
    memset(cmd, 0, sizeof(Command));
    cmd->type = type;
    cmd->size = size;
    return cmd;
}

static bool next_command(Command** prev)
{
    if (*prev == NULL) {
        *prev = (Command*)cache->command_buf;
    } else {
        *prev = (Command*)(((char*)*prev) + (*prev)->size);
    }
    return *prev != ((Command*)(cache->command_buf + cache->command_buf_idx));
}

void rencache_show_debug(bool enable)
{
    cache->show_debug = enable;
}

void rencache_free_font(RenFont* font)
{
    Command* cmd = push_command(FREE_FONT, sizeof(Command));
    if (cmd) {
        cmd->font = font;
    }
}

void rencache_set_clip_rect(RenRect rect)
{
    // printf("%d %d %d %d\n", rect.x, rect.y, rect.width, rect.height);
    Command* cmd = push_command(SET_CLIP, sizeof(Command));
    if (cmd) {
        cmd->rect = intersect_rects(rect, cache->target_rect);
    }
}

void rencache_state_save()
{
    Command* cmd = push_command(SAVE_STATE, sizeof(Command));
    if (cmd) {
        cmd->rect = { 0, 0, 0, 0 };
    }
}

void rencache_state_restore()
{
    Command* cmd = push_command(RESTORE_STATE, sizeof(Command));
    if (cmd) {
        cmd->rect = { 0, 0, 0, 0 };
    }
}

void rencache_draw_image(RenImage* image, RenRect rect, RenColor color)
{
    if (!rects_overlap(cache->target_rect, rect)) {
        return;
    }
    Command* cmd = push_command(DRAW_IMAGE, sizeof(Command));
    if (cmd) {
        cmd->rect = rect;
        cmd->color = color;
        cmd->font = (void*)image;
    }
}

void rencache_invalidate_rect(RenRect rect)
{
    rencache_draw_rect(rect, {
        (uint8_t)rand(), (uint8_t)rand(), (uint8_t)rand(),0
    }, false, 0);
}

void rencache_draw_rect(RenRect rect, RenColor color, bool fill, int stroke, int radius)
{
    if (!rects_overlap(cache->target_rect, rect)) {
        return;
    }
    Command* cmd = push_command(DRAW_RECT, sizeof(Command));
    if (cmd) {
        cmd->rect = rect;
        cmd->color = color;
        cmd->bold = fill;
        cmd->italic = ((stroke & 0xf) << 0xf) | radius;
    }
}

int rencache_draw_text(RenFont* font, const char* text, int x, int y, RenColor color, bool bold, bool italic, bool fixed)
{
    int len = strlen(text);
    if (len == 0) {
        return 0;
    }
    int fw, fh;
    ren_get_font_extents(font, &fw, &fh, text, strlen(text));
    RenRect rect;
    rect.x = x;
    rect.y = y;
    rect.width = fw;
    rect.height = fh;

    if (rects_overlap(cache->target_rect, rect)) {
        int sz = len + 1;
        Command* cmd = push_command(DRAW_TEXT, sizeof(Command) + sz);
        if (cmd) {
            memcpy(cmd->text, text, sz);
            cmd->color = color;
            cmd->font = (void*)font;
            cmd->rect = rect;
            cmd->bold = bold;
            cmd->italic = italic;
        }
    }

    return x + rect.width;
}

void rencache_invalidate(void)
{
    memset(cache->cells_prev, 0xff, sizeof(cache->cells_buf1));
}

void rencache_begin_frame(int w, int h, RenCache *target)
{
    if (target) {
        cache = target;
    } else {
        cache = &default_cache;
    }

    /* reset all cells if the screen width/height has changed */
    // int w, h;
    // ren_get_size(&w, &h);

    if (cache->target_rect.width != w || h != cache->target_rect.height) {
        cache->target_rect.width = w;
        cache->target_rect.height = h;
        rencache_invalidate();
    }

    ren_state_save();
}

static void update_overlapping_cells(RenRect r, unsigned h)
{
    int x1 = r.x / CELL_SIZE;
    int y1 = r.y / CELL_SIZE;
    int x2 = (r.x + r.width) / CELL_SIZE;
    int y2 = (r.y + r.height) / CELL_SIZE;

    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            int idx = cell_idx(x, y);
            hash(&(cache->cells[idx]), &h, sizeof(h));
        }
    }
}

static void push_rect(RenRect r, int* count)
{
    /* try to merge with existing rectangle */
    for (int i = *count - 1; i >= 0; i--) {
        RenRect* rp = &cache->rect_buf[i];
        if (rects_overlap(*rp, r)) {
            *rp = merge_rects(*rp, r);
            return;
        }
    }
    /* couldn't merge with previous rectangle: push */
    cache->rect_buf[(*count)++] = r;
}

void rencache_end_frame(void)
{
    /* update cells from commands */
    Command* cmd = NULL;
    RenRect cr = cache->target_rect;
    while (next_command(&cmd)) {
        if (cmd->type == SET_CLIP) {
            cr = cmd->rect;
            continue;
        }
        RenRect r = intersect_rects(cmd->rect, cr);
        if (r.width == 0 || r.height == 0) {
            continue;
        }
        unsigned h = HASH_INITIAL;
        hash(&h, cmd, cmd->size);
        update_overlapping_cells(r, h);
    }

    /* push rects for all cells changed from last frame, reset cells */
    int rect_count = 0;
    int max_x = cache->target_rect.width / CELL_SIZE + 1;
    int max_y = cache->target_rect.height / CELL_SIZE + 1;
    for (int y = 0; y < max_y; y++) {
        for (int x = 0; x < max_x; x++) {
            /* compare previous and current cell for change */
            int idx = cell_idx(x, y);
            if (cache->cells[idx] != cache->cells_prev[idx]) {
                push_rect((RenRect){ x, y, 1, 1 }, &rect_count);
            }
            cache->cells_prev[idx] = HASH_INITIAL;
        }
    }

    /* expand rects from cells to pixels */
    for (int i = 0; i < rect_count; i++) {
        RenRect* r = &cache->rect_buf[i];
        r->x *= CELL_SIZE;
        r->y *= CELL_SIZE;
        r->width *= CELL_SIZE;
        r->height *= CELL_SIZE;
        *r = intersect_rects(*r, cache->target_rect);
    }

    /* redraw updated regions */
    bool has_free_commands = false;
    for (int i = 0; i < rect_count; i++) {
        /* draw */
        RenRect r = cache->rect_buf[i];
        ren_set_clip_rect(r);

        cmd = NULL;
        while (next_command(&cmd)) {
            switch (cmd->type) {
            case FREE_FONT:
                has_free_commands = true;
                break;
            case SET_CLIP:
                ren_set_clip_rect(intersect_rects(cmd->rect, r));
                break;
            case DRAW_IMAGE:
                ren_draw_image((RenImage*)cmd->font, cmd->rect, cmd->color);
            case DRAW_RECT: {
                bool fill = cmd->bold;
                int stroke = cmd->italic >> 0xf;
                int radius = cmd->italic & 0xf;
                if (cmd->bold || cmd->italic) {
                    ren_draw_rect(cmd->rect, cmd->color, fill, stroke, radius);
                }
                break;
            }
            case DRAW_TEXT:
                ren_draw_text((RenFont*)cmd->font, cmd->text, cmd->rect.x, cmd->rect.y, cmd->color, cmd->bold, cmd->italic);
                break;
            case SAVE_STATE:
                ren_state_save();
                break;
            case RESTORE_STATE:
                ren_state_restore();
                break;
            }
        }

        if (cache->show_debug) {
            // RenColor color = { (uint8_t)rand(), (uint8_t)rand(), (uint8_t)rand(), 50 };
            RenColor color = { 255,255,0,50 };
            ren_draw_rect(r, color, false, 4.0f);
        }
    }

    /* update dirty rects */
    if (rect_count > 0) {
        ren_update_rects(cache->rect_buf, rect_count);
    }

    /* free fonts */
    if (has_free_commands) {
        cmd = NULL;
        while (next_command(&cmd)) {
            if (cmd->type == FREE_FONT) {
                ren_destroy_font((RenFont*)cmd->font);
            }
        }
    }

    /* swap cell buffer and reset */
    unsigned* tmp = cache->cells;
    cache->cells = cache->cells_prev;
    cache->cells_prev = tmp;
    cache->command_buf_idx = 0;

    ren_state_restore();
}

void rencache_init()
{
    cache = &default_cache;
}

void rencache_shutdown()
{}