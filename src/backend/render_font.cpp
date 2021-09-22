#include "renderer.h"

#include <cairo.h>
#include <fontconfig/fontconfig.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

#include "utf8.h"

#define MAX_GLYPHSET 256

extern int items_drawn;

cairo_pattern_t* ren_image_pattern(RenImage* image);
cairo_surface_t* ren_image_surface(RenImage* image);

typedef struct {
    char t[3];
    int index;
} Ligature;

typedef struct {
    RenImage* image;
    int cw;
    int ch;
    int cp;
} GlyphSet;

const char _ligatures[][3] = {
    "==", "!=", "<=", ">=", "->", "<-", "!!", "&&", "||", "::", ":=", "++", "--",
    ":)", ":(", ":|", 0
};

struct RenFont {
    int font_width;
    int font_height;
    bool firable;

    GlyphSet regular[MAX_GLYPHSET];
    GlyphSet italic[MAX_GLYPHSET];
    GlyphSet bold[MAX_GLYPHSET];
    std::map<int, GlyphSet> utf8;

    Ligature ligatures[32];

    std::string desc;
    std::string alias;

    PangoFontMap* font_map;
    PangoContext* context;
    PangoLayout* layout;
};

std::vector<RenFont*> fonts;
std::map<std::string, RenFont*> font_alias;

cairo_t* ren_context();
cairo_t* ren_image_context(RenImage* image);

void pango_get_font_extents(PangoLayout* layout, int* w, int* h, const char* text, int len)
{
    pango_layout_set_attributes(layout, nullptr);
    pango_layout_set_text(layout, text, len);
    pango_layout_get_pixel_size(layout, w, h);
}

GlyphSet bake_glyph(RenFont* fnt, char* c)
{
    int cw, ch;
    int x = 0;
    int y = 0;

    char* _p = c;
    unsigned cp = 0;
    utf8_to_codepoint(_p, &cp);

    int _pl = strlen(_p);
    pango_get_font_extents(fnt->layout, &cw, &ch, _p, _pl);

    GlyphSet glyph;
    glyph.image = Renderer::instance()->create_image(cw + 1, ch);
    glyph.cw = cw;
    glyph.ch = ch;
    glyph.cp = cp;

    Renderer::instance()->begin_frame(glyph.image);

    pango_layout_set_text(fnt->layout, _p, _pl);
    cairo_set_source_rgb(ren_image_context(glyph.image), 1, 1, 1);
    cairo_move_to(ren_image_context(glyph.image), x, y);
    pango_cairo_show_layout(ren_image_context(glyph.image), fnt->layout);

    Renderer::instance()->end_frame();

    return glyph;
}

RenFont* Renderer::create_font(char* fdsc, char* alias)
{
    for (auto fnt : fonts) {
        if (fnt->alias == alias) {
            return fnt;
        }
        if (fnt->desc == fdsc) {
            return fnt;
        }
    }

    RenFont* fnt = new RenFont();
    fnt->font_map = pango_cairo_font_map_get_default(); // pango-owned, don't delete
    fnt->context = pango_font_map_create_context(fnt->font_map);
    fnt->layout = pango_layout_new(fnt->context);
    fnt->desc = fdsc;
    fnt->firable = true;

    fnt->font_width = 0;
    fnt->font_height = 0;

    if (alias) {
        fnt->alias = alias;
    }

    for (int i = 0; i < 3; i++) {

        std::string desc = fdsc;

        GlyphSet* set = fnt->regular;
        switch (i) {
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
        pango_layout_set_font_description(fnt->layout, nullptr);
        pango_layout_set_font_description(fnt->layout, font_desc);
        pango_font_map_load_font(fnt->font_map, fnt->context, font_desc);
        pango_font_description_free(font_desc);

        cairo_font_options_t* cairo_font_options = cairo_font_options_create();
        cairo_font_options_set_antialias(cairo_font_options, CAIRO_ANTIALIAS_SUBPIXEL);

        cairo_font_options_set_hint_style(cairo_font_options, CAIRO_HINT_STYLE_DEFAULT); // NONE DEFAULT SLIGHT MEDIUM FULL
        cairo_font_options_set_hint_metrics(cairo_font_options, CAIRO_HINT_METRICS_ON); // ON OFF

        pango_cairo_context_set_font_options(fnt->context, cairo_font_options);

        if (i == 0) {
            const char text[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789";
            int len = strlen(text);
            pango_get_font_extents(fnt->layout, &fnt->font_width, &fnt->font_height, text, len);
            fnt->font_width = ((float)fnt->font_width / len);
            // printf(">>%d\n", fnt->font_width);
        }

        // generate glyphs
        for (int i = 0; i < MAX_GLYPHSET; i++) {
            if (!isprint((char)i)) {
                continue;
            }

            // bake common glyphs
            char str[3] = { (char)i, 0, 0 };
            GlyphSet glyph = bake_glyph(fnt, str);
            set[glyph.cp & 0xff] = glyph;
        }

        break; // no bold/italic glyphs
    }

    // fnt->font_width -= 1;
    fonts.push_back(fnt);
    if (alias[0] != 0) {
        font_alias[alias] = fnt;
    }

    if (!get_default_font()) {
        set_default_font(fnt);
    }
    return fnt;
}

RenFont* Renderer::font(char* alias)
{
    RenFont* fnt = font_alias[alias];
    if (fnt) {
        return fnt;
    }
    return Renderer::instance()->get_default_font();
}

void Renderer::destroy_font(RenFont* font)
{
    std::vector<RenFont*>::iterator it = std::find(fonts.begin(), fonts.end(), font);
    if (it != fonts.end()) {
        fonts.erase(it);
        delete font;
    }
}

void Renderer::destroy_fonts()
{
    std::vector<RenFont*> _fonts = fonts;
    for (auto fnt : _fonts) {
        destroy_font(fnt);
    }
}

void Renderer::get_font_extents(RenFont* font, int* w, int* h, const char* text, int len)
{
    if (!font) {
        font = Renderer::instance()->get_default_font();
    }

    *w = font->font_width * len;
    *h = font->font_height;
}

static inline void ren_draw_char_image(RenImage* image, RenRect rect, RenColor clr, bool italic)
{
    items_drawn++;

    cairo_t* cairo_context = ren_context();

    int w, h;
    Renderer::instance()->image_size(image, &w, &h);

    cairo_save(cairo_context);
    cairo_translate(cairo_context, rect.x + (italic ? rect.width / 2 : 0), rect.y);
    cairo_scale(cairo_context,
        (double)rect.width / w,
        (double)rect.height / h);

    if (italic) {
        cairo_matrix_t matrix;
        cairo_matrix_init(&matrix,
            0.9, 0.0,
            -0.25, 1.0,
            0.0, 0.0);
        cairo_transform(cairo_context, &matrix);
    }

    if (clr.r == clr.g != 255 && clr.g == clr.b) {
        cairo_set_source_surface(cairo_context, ren_image_surface(image), 0, 0);
        cairo_paint(cairo_context);
    } else {
        cairo_set_source_rgba(cairo_context, clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f, 1.0f);
        cairo_mask(cairo_context, ren_image_pattern(image));
    }

    cairo_restore(cairo_context);
}

int Renderer::draw_wtext(RenFont* font, const wchar_t* text, int x, int y, RenColor clr, bool bold, bool italic)
{
    items_drawn++;

    if (!font) {
        font = Renderer::instance()->get_default_font();
    }
    int length = wcslen(text);
    bool shear = false;

    cairo_t* cairo_context = ren_context();

    GlyphSet* set = font->regular;

    wchar_t* p = (wchar_t*)text;

    int xx = 0;

    int i = 0;
    while (*p) {
        clr.a = 0;

        GlyphSet glyph;
        if (*p < MAX_GLYPHSET) {
            glyph = set[*p];
        } else {
            glyph = font->utf8[*p];
            if (glyph.cp != *p && *p < 32768) {
                char u[3];
                codepoint_to_utf8(*p, u);
                font->utf8[*p] = bake_glyph(font, u);
                glyph = set[*p];
            }
        }

        if (glyph.cp == *p & 0xff && glyph.image) {
            ren_draw_char_image(glyph.image, { x + (i * font->font_width) + (font->font_width / 2) - (glyph.cw / 2), y + (font->font_height / 2) - (glyph.ch / 2), glyph.cw + 1, glyph.ch }, clr, italic);
        }

        i++;
        p++;
    }

    return x;
}

int Renderer::draw_text(RenFont* font, const char* text, int x, int y, RenColor clr, bool bold, bool italic)
{
    items_drawn++;

    if (!font) {
        font = Renderer::instance()->get_default_font();
    }
    int length = strlen(text);
    bool shear = false;

    cairo_t* cairo_context = ren_context();

    GlyphSet* set = font->regular;

    char* p = (char*)text;
    int xx = 0;

    int i = 0;
    while (*p) {
        unsigned cp;
        p = (char*)utf8_to_codepoint(p, &cp);
        clr.a = 0;

        GlyphSet glyph = set[cp & 0xff];
        if (glyph.cp == cp & 0xff && glyph.image) {
            ren_draw_char_image(glyph.image, { x + (i * font->font_width) + (font->font_width / 2) - (glyph.cw / 2), y + (font->font_height / 2) - (glyph.ch / 2), glyph.cw + 1, glyph.ch }, clr, italic);
        }

        i++;
    }

    return x;
}

void Renderer::register_font(char* path)
{
    std::string fontPath = path;
    const FcChar8* file = (const FcChar8*)fontPath.c_str();
    FcBool fontAddStatus = FcConfigAppFontAddFile(FcConfigGetCurrent(), file);
    printf(">font %s %d\n", path, fontAddStatus);
}