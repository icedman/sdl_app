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
    GlyphSet italic[MAX_GLYPHSET];
    GlyphSet bold[MAX_GLYPHSET];

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
    PangoFontMap* font_map = pango_cairo_font_map_get_default(); // pango-owned, don't delete
    PangoContext* context = pango_font_map_create_context(font_map);
    PangoLayout* layout = pango_layout_new(context);
    fnt->desc = fdsc;
    fnt->firable = true;

    fnt->font_width = 0;
    fnt->font_height = 0;

    for(int i=0;i<3;i++) {

    	std::string desc = fdsc;

	    GlyphSet *set = fnt->regular;
	    switch(i) {
    	case 0:
	    	set = fnt->regular;
	    	break;
	    case 1:
	    	set = fnt->italic;
	    	desc += " italic";
	    	break;
	    case 2:
	    	set = fnt->bold;
	    	desc += " bold";
	    	break;
	    }


	    PangoFontDescription* font_desc = pango_font_description_from_string(desc.c_str());
	    pango_layout_set_font_description(layout, nullptr);
	    pango_layout_set_font_description(layout, font_desc);
	    pango_font_map_load_font(font_map, context, font_desc);

	    pango_font_description_free(font_desc); 

	    cairo_font_options_t *cairo_font_options = cairo_font_options_create();
	    cairo_font_options_set_antialias(cairo_font_options, CAIRO_ANTIALIAS_SUBPIXEL);

	    cairo_font_options_set_hint_style(cairo_font_options, CAIRO_HINT_STYLE_DEFAULT); // NONE DEFAULT SLIGHT MEDIUM FULL
	    cairo_font_options_set_hint_metrics(cairo_font_options, CAIRO_HINT_METRICS_ON);  // ON OFF

	    pango_cairo_context_set_font_options(context, cairo_font_options);

	    if (i == 0) {
		    const char text[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789";
		    int len = strlen(text);
		    _ren_get_font_extents(layout, &fnt->font_width, &fnt->font_height, text, len);
		    fnt->font_width = ((float)fnt->font_width/len);
		    printf(">>%d\n", fnt->font_width);
	    }

	    // generate glyphs
	    for(int i=0;i<128;i++) {
	    	int cw, ch;
	    	int x = 0;
	    	int y = 0;

	        char _p[] = { (char)i, 0 };
	        unsigned cp = 0;
	        utf8_to_codepoint(_p, &cp);

	    	_ren_get_font_extents(layout, &cw, &ch, _p, 1);
	    	cw++;

	    	set[cp].image = ren_create_image(cw, ch);
	    	set[cp].cw = cw;
	    	set[cp].ch = ch;
	    	ren_begin_frame(set[cp].image);

	        pango_layout_set_text(layout, _p, 1);
	        cairo_set_source_rgb(ren_image_context(set[cp].image), 1,1,1);
	        cairo_move_to(ren_image_context(set[cp].image), x, y);
	        pango_cairo_show_layout(ren_image_context(set[cp].image), layout);

	    	ren_end_frame();
	    }
	}

    fonts.push_back(fnt);

    if (!ren_get_default_font()) {
    	ren_set_default_font(fnt);
    }
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

    *w = font->font_width * len;
    *h = font->font_height;
}

int ren_draw_text(RenFont* font, const char* text, int x, int y, RenColor clr, bool bold, bool italic, bool fixed_width)
{
    if (!font) {
        font = ren_get_default_font();
    }
    int length = strlen(text);

    cairo_t *cairo_context = ren_context();

	GlyphSet *set = font->regular;
	if (italic) {
		set = font->italic;
	}
	if (bold) {
		set = font->bold;
	}

    for(int i=0; i<length; i++) {
        char _p[] = { (char)text[i], 0 };
        unsigned cp = 0;
        utf8_to_codepoint(_p, &cp);

        clr.a = 255;

        GlyphSet *glyph = &set[cp]; 
		ren_draw_image(glyph->image, {
			x + (i*font->font_width) + (font->font_width/2) - (glyph->cw/2), y,
			glyph->cw, glyph->ch
		}, clr);
    }

    return x;
}
