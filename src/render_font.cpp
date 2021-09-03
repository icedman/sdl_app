#include "render_font.h"
#include "renderer.h"

#include <cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#define MAX_GLYPHSET 256

typedef struct {
    RenImage* image;
    int cw;
    int ch;
} GlyphSet;

struct RenFont {
    int font_width;
    int font_height;
    bool firable;

    GlyphSet regular[MAX_GLYPHSET];
    GlyphSet bold[MAX_GLYPHSET];
    GlyphSet italic[MAX_GLYPHSET];
    
    PangoFontMap* font_map;
    PangoLayout* layout;
    PangoContext* context;

    std::string desc;
};

std::vector<RenFont*> fonts;

cairo_t*  ren_context();
cairo_t*  ren_image_context(RenImage* image);

static const char* utf8_to_codepoint(const char* p, unsigned* dst)
{
    unsigned res, n;
    switch (*p & 0xf0) {
    case 0xf0:
        res = *p & 0x07;
        n = 3;
        break;
    case 0xe0:
        res = *p & 0x0f;
        n = 2;
        break;
    case 0xd0:
    case 0xc0:
        res = *p & 0x1f;
        n = 1;
        break;
    default:
        res = *p;
        n = 0;
        break;
    }
    while (n--) {
        res = (res << 6) | (*(++p) & 0x3f);
    }
    *dst = res;
    return p + 1;
}


void _ren_get_font_extents(PangoLayout *layout, int *w, int *h, const char *text, int len)
{
    pango_layout_set_attributes(layout, nullptr);
    pango_layout_set_text(layout, text, len);
    pango_layout_get_pixel_size(layout, w, h);
}

RenFont* ren_create_font(char *fdsc)
{
    for(auto fnt : fonts) {
        if (fnt->desc == fdsc) {
            return fnt;
        }
    }

    RenFont *fnt = new RenFont();
    fnt->font_map = pango_cairo_font_map_get_default(); // pango-owned, don't delete
    fnt->context = pango_font_map_create_context(fnt->font_map);
    fnt->layout = pango_layout_new(fnt->context);
    fnt->desc = fdsc;
    fnt->firable = true;

    fnt->font_width = 0;
    fnt->font_height = 0;

    PangoFontDescription* font_desc = pango_font_description_from_string(fdsc);
    pango_layout_set_font_description(fnt->layout, nullptr);   // pango will silently not make a copy if
    // the new fontdesc matches the old one
    // whether by pointer or deep compare.
    // this makes sure the old one is deleted
    // in favor of a new copy
    pango_layout_set_font_description(fnt->layout, font_desc);
    pango_font_map_load_font(fnt->font_map, fnt->context, font_desc);  // most examples do this, but does not appear
    // to actually be necessary? There's not much
    // info out there getting detailed on Pango.

    pango_font_description_free(font_desc); 

    cairo_font_options_t *cairo_font_options = cairo_font_options_create();
    cairo_font_options_set_antialias(cairo_font_options, CAIRO_ANTIALIAS_SUBPIXEL);

    cairo_font_options_set_hint_style(cairo_font_options, CAIRO_HINT_STYLE_DEFAULT); // NONE DEFAULT SLIGHT MEDIUM FULL
    cairo_font_options_set_hint_metrics(cairo_font_options, CAIRO_HINT_METRICS_ON);  // ON OFF

    pango_cairo_context_set_font_options(fnt->context, cairo_font_options);

    const char text[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789";
    int len = strlen(text);\
    _ren_get_font_extents(fnt->layout, &fnt->font_width, &fnt->font_height, text, len);

    fnt->font_width = ((float)fnt->font_width/len) + 0.8f;

    printf(">>%d\n", fnt->font_width);
    if (!ren_get_default_font()) {
    	ren_set_default_font(fnt);
    }

    // generate glyphs

    for(int i=0;i<128;i++) {
    	int cw, ch;
    	int x = 0;
    	int y = 0;

        char _p[] = { (char)i, 0 };
        unsigned cp = 0;
        utf8_to_codepoint(_p, &cp);

    	_ren_get_font_extents(fnt->layout, &cw, &ch, _p, 1);
    	cw++;

    	fnt->regular[cp].image = ren_create_image(cw, ch);
    	fnt->regular[cp].cw = cw;
    	fnt->regular[cp].ch = ch;
    	ren_begin_frame(fnt->regular[cp].image);

        pango_layout_set_text(fnt->layout, _p, 1);
        cairo_set_source_rgb(ren_image_context(fnt->regular[cp].image), 1,1,1);
        cairo_move_to(ren_image_context(fnt->regular[cp].image), x, y);
        pango_cairo_show_layout(ren_image_context(fnt->regular[cp].image), fnt->layout);

    	ren_end_frame();

    	// printf("%d %d %d\n", i, cw, ch);
    }

    fonts.push_back(fnt);
    return fnt;
}

void ren_destroy_font(RenFont *font)
{
    std::vector<RenFont*>::iterator it = std::find(fonts.begin(), fonts.end(), font);
    if (it != fonts.end()) {
        fonts.erase(it);
    }
    delete font;
}

void ren_destroy_fonts()
{
    std::vector<RenFont*> _fonts = fonts;
    for(auto fnt : _fonts) {
        ren_destroy_font(fnt);
    }	
}

void ren_get_font_extents(RenFont *font, int *w, int *h, const char *text, int len, bool fixed)
{
    if (!font) {
        font = ren_get_default_font();
    }

    if (font->font_width) {
        *w = font->font_width * len;
        *h = font->font_height;
        return;
    }

    pango_layout_set_attributes(font->layout, nullptr);
    pango_layout_set_text(font->layout, text, len);
    pango_layout_get_pixel_size(font->layout, w, h);
}

int ren_draw_text(RenFont* font, const char* text, int x, int y, RenColor clr, bool bold, bool italic, bool fixed_width)
{
    if (!font) {
        font = ren_get_default_font();
    }
    int length = strlen(text);

    cairo_t *cairo_context = ren_context();

    for(int i=0; i<length; i++) {
        char _p[] = { (char)text[i], 0 };
        unsigned cp = 0;
        utf8_to_codepoint(_p, &cp);

        clr.a = 255;

        GlyphSet *set = &font->regular[cp]; 
		ren_draw_image(set->image, {
			x + (i*font->font_width) + (font->font_width/2) - (set->cw/2), y,
			set->cw, set->ch
		}, clr);
    }

    return x;
}
