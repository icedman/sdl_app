#include "pango_font.h"
#include "renderer.h"
#include "utf8.h"

#include <map>

#include <cairo.h>
#include <fontconfig/fontconfig.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

// #define FONT_PREBAKE_GLYPHS // define at build config

#define MAX_GLYPHSET 256
#define MAX_CODEPOINT 0xffff

#define CARET_WIDTH 2
#define UNDERLINE_WIDTH 1

font_ptr pango_font_create(std::string name, int size);
void pango_font_destroy(font_t*);
int pango_font_draw_text(renderer_t* renderer, font_t* font, char* text, int x, int y, color_t clr, bool bold, bool italic, bool underline);
void pango_get_font_extents(renderer_t* renderer, font_t* font, const char* text, int len, int* w, int* h);
bool pango_register_font(char* text);

cairo_t* ctx_cairo_context(context_t* ctx);
cairo_pattern_t* ctx_cairo_pattern(context_t* ctx);
cairo_surface_t* ctx_cairo_surface(context_t* ctx);

typedef struct {
    image_ptr image;
    int cw;
    int ch;
    int cp;
} glypset_t;

struct pango_font_t : font_t {
    pango_font_t();
    ~pango_font_t();

    glypset_t set[MAX_GLYPHSET];
    std::map<int, glypset_t> utf8;

    PangoFontMap* font_map;
    PangoContext* context;
    PangoLayout* layout;

    font_ptr italic;
    font_ptr bold;
};

pango_font_t::pango_font_t()
{}

pango_font_t::~pango_font_t()
{
    // free pango resources?
    g_object_unref(layout);
    g_object_unref(font_map);
    g_object_unref(context);
}

static std::vector<std::string> registered_fonts;
bool pango_register_font(char* path)
{
    for(auto s : registered_fonts) {
        if (s == path) return true;
    }
    
    registered_fonts.push_back(path);
    const FcChar8 * fontFile = (const FcChar8 *)path;
    FcBool fontAddStatus = FcConfigAppFontAddFile(FcConfigGetCurrent(), fontFile);
    // FcChar8 *configName = FcConfigFilename(NULL);
    return fontAddStatus;
}

void _pango_get_font_extents(PangoLayout* layout, int* w, int* h, const char* text, int len)
{
    pango_layout_set_attributes(layout, nullptr);
    pango_layout_set_text(layout, text, len);
    pango_layout_get_pixel_size(layout, w, h);
}

void pango_get_font_extents(renderer_t* renderer, font_t* font, const char* text, int len, int* w, int* h)
{
    pango_font_t* fnt = (pango_font_t*)(font);
    _pango_get_font_extents(fnt->layout, w, h, text, len);
}

glypset_t bake_glyph(pango_font_t* fnt, char* c)
{
    int cw, ch;
    int x = 0;
    int y = 0;

    char* _p = c;
    unsigned cp = 0;
    utf8_to_codepoint(_p, &cp);

    renderer_t renderer;

    int _pl = strlen(_p);
    _pango_get_font_extents(fnt->layout, &cw, &ch, _p, _pl);

    renderer.init(cw + 1, ch);

    glypset_t glyph;
    glyph.image = renderer.context;
    glyph.cw = cw;
    glyph.ch = ch;
    glyph.cp = cp;

    renderer.begin_frame();

    pango_layout_set_text(fnt->layout, _p, _pl);
    cairo_set_source_rgb(ctx_cairo_context(glyph.image.get()), 1, 1, 1);
    cairo_move_to(ctx_cairo_context(glyph.image.get()), x, y);
    pango_cairo_show_layout(ctx_cairo_context(glyph.image.get()), fnt->layout);

    renderer.end_frame();

    renderer.shutdown();

    return glyph;
}

font_ptr pango_font_create(char* fdsc, char* alias)
{

    font_ptr _f = std::make_shared<pango_font_t>();
    pango_font_t* fnt = (pango_font_t*)(_f.get());
    // fnt->font_map = pango_cairo_font_map_get_default(); // pango-owned, don't delete
    fnt->font_map = pango_cairo_font_map_new();
    // fnt->font_map = pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
    fnt->context = pango_font_map_create_context(fnt->font_map);

#if 0
    cairo_font_options_t* font_options = cairo_font_options_create();
    // cairo_font_options_set_hint_style(font_options, CAIRO_HINT_STYLE_FULL);
    cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_SUBPIXEL);
    cairo_font_options_set_hint_metrics(font_options, CAIRO_HINT_METRICS_ON); // ON OFF
    cairo_font_options_set_hint_style(font_options, CAIRO_HINT_STYLE_NONE); // NONE DEFAULT SLIGHT MEDIUM FULL
    pango_cairo_context_set_font_options(fnt->context, font_options);
    cairo_font_options_destroy(font_options);
#endif

    fnt->layout = pango_layout_new(fnt->context);
    fnt->desc = fdsc;
    fnt->italic = NULL;
    fnt->bold = NULL;

    fnt->width = 0;
    fnt->height = 0;

    if (alias) {
        fnt->alias = alias;
    }

    std::string desc = fdsc;

    glypset_t* set = fnt->set;

    PangoFontDescription* font_desc = pango_font_description_from_string(desc.c_str());
    pango_layout_set_font_description(fnt->layout, font_desc);
    pango_font_map_load_font(fnt->font_map, fnt->context, font_desc);
    pango_font_description_free(font_desc);

    const char text[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 ~!@#$%^&*()-_=+{}[]";
    int len = strlen(text);
    _pango_get_font_extents(fnt->layout, &fnt->width, &fnt->height, text, len);
    fnt->width = ((float)fnt->width / len);

#ifdef FONT_FIX_FIXED_WIDTH_EXTENTS
    fnt->width += 0.10f * fnt->width;
#endif

    for (int i = 0; i < MAX_GLYPHSET; i++) {
        set[i].image = 0;
    }

#ifdef FONT_PREBAKE_GLYPHS // define at build config
    // generate glyphs
    for (int i = 0; i < MAX_GLYPHSET; i++) {
        if (!isprint((char)i)) {
            continue;
        }
        char str[3] = { (char)i, 0, 0 };
        glypset_t glyph = bake_glyph(fnt, str);
        set[glyph.cp & 0xff] = glyph;
    }
#endif

    fnt->draw_text = pango_font_draw_text;
    fnt->get_font_extents = pango_get_font_extents;
    return _f;
}

font_ptr pango_font_create(std::string name, int size)
{
    std::string desc = name + " " + std::to_string(size);
    font_ptr fnt = pango_font_create((char*)desc.c_str(), "");
    fnt->name = name;
    fnt->size = size;

    pango_font_t* pf = (pango_font_t*)(fnt.get());
    desc = name + " italic " + std::to_string(size);
    pf->italic = pango_font_create((char*)desc.c_str(), "");
    desc = name + " bold " + std::to_string(size);
    pf->bold = pango_font_create((char*)desc.c_str(), "");
    return fnt;
}

int pango_font_draw_char_image(renderer_t* renderer, image_t* img, rect_t rect, color_t clr,
    bool bold, bool italic, bool underline, int caret, bool bg)
{
    cairo_t* cairo_context = ctx_cairo_context(renderer->context.get());

    int w = img->width;
    int h = img->height;

    cairo_save(cairo_context);
    cairo_translate(cairo_context, rect.x + (italic ? rect.w / 2 : 0), rect.y);
    cairo_scale(cairo_context,
        (double)rect.w / w,
        (double)rect.h / h);

    if (italic) {
        cairo_matrix_t matrix;
        cairo_matrix_init(&matrix,
            0.9, 0.0,
            -0.25, 1.0,
            0.0, 0.0);
        cairo_transform(cairo_context, &matrix);
    }

    if (!color_is_set(clr)) {
        cairo_set_source_surface(cairo_context, ctx_cairo_surface(img), 0, 0);
        cairo_paint(cairo_context);
    } else {
        cairo_set_source_rgba(cairo_context, clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f, 1.0f);
        cairo_mask(cairo_context, ctx_cairo_pattern(img));
    }

    cairo_restore(cairo_context);

    if (underline) {
        rect_t r = rect;
        r.y += r.h - (1 + UNDERLINE_WIDTH);
        r.h = UNDERLINE_WIDTH;
        renderer->draw_rect(r, clr);
    }

    if (caret) {
        rect_t r = rect;
        if (caret == 2) {
            r.x += r.w - CARET_WIDTH;
        }
        r.w = CARET_WIDTH;
        renderer->draw_rect(r, clr);
    }

    if (bold) {
        rect_t r = rect;
        r.x += 1;
        pango_font_draw_char_image(renderer, img, r, clr, false, italic, underline, false, false);
    }
    return 0;
}

int pango_font_draw_text(renderer_t* renderer, font_t* font, wchar_t* text, int x, int y, color_t clr, bool bold, bool italic, bool underline)
{
    pango_font_t* pf = (pango_font_t*)font;

    int length = wcslen(text);
    bool shear = false;

    glypset_t* set = pf->set;

    wchar_t* p = (wchar_t*)text;

    int i = 0;
    while (*p) {
        clr.a = 0;

        int cp = *p;
        glypset_t glyph;

        // text span
        color_t _clr = clr;
        color_t _bg = { 0, 0, 0, 0 };
        bool _bold = bold;
        bool _italic = italic;
        bool _underline = underline;
        int _caret = 0;

        text_span_t res = span_from_index(renderer->text_spans, renderer->text_span_idx++);
        if (res.length) {
            _clr = res.fg;
            _bg = res.bg;
            _bold = res.bold;
            _italic = res.italic;
            _underline = res.underline;
            _caret = res.caret;
        }

        if (cp > MAX_CODEPOINT) {
            cp = '?';
        }

        if (cp < MAX_GLYPHSET) {
            glyph = set[cp];

        } else {

            glyph = pf->utf8[cp];
            if (glyph.cp != *p) {
                char u[3];
                if (codepoint_to_utf8(*p, u) == 0) {
                    i++;
                    p++;
                    continue;
                }

                pf->utf8[*p] = bake_glyph(pf, u);
                // glyph = set[*p];
            }
        }

        if (glyph.cp == *p & 0xff && glyph.image) {
            if (color_is_set(_bg)) {
                renderer->draw_rect({ x + (i * font->width), y, font->width, font->height }, _bg, true);
            }

            renderer->_draw_count++;
            pango_font_draw_char_image(renderer,
                glyph.image.get(),
                { x + (i * font->width) + (font->width / 2) - (glyph.cw / 2),
                    y + (font->height / 2) - (glyph.ch / 2),
                    glyph.cw + 1,
                    glyph.ch },
                _clr, _bold, _italic, _underline, _caret, color_is_set(_bg));
        }

        i++;
        p++;
    }

    return 0;
}

static const char _ligatures[][3] = {
    "==", "!=", "<=", ">=", "->", "<-", "!!", "&&", "||", "::", ":=", "++", "--", "*=", "|=", "/=", "<<", ">>",
    ":)", ":(", ":|", 0
};

inline int pango_font_draw_span(renderer_t* renderer, font_t* font, char* text, int x, int y, text_span_t& span, color_t clr, bool bold, bool italic, bool underline)
{
    cairo_t* cairo_context = ctx_cairo_context(renderer->context.get());
    pango_font_t* fnt = (pango_font_t*)font;

    // text span
    color_t _clr = clr;
    color_t _bg = { 0, 0, 0, 0 };
    bool _bold = bold;
    bool _italic = italic;
    bool _underline = underline;
    int _caret = 0;

    if (span.length) {
        _clr = span.fg;
        _bg = span.bg;
        _bold = span.bold;
        _italic = span.italic;
        _underline = span.underline;
        _caret = span.caret;
    }

    pango_font_t* _pf = fnt;
    if (span.italic && fnt->italic) {
        _pf = (pango_font_t*)(fnt->italic.get());
    }
    if (span.bold && fnt->bold) {
        _pf = (pango_font_t*)(fnt->bold.get());
    }

    renderer->_draw_count++;

    // int sz = 14;
    // cairo_select_font_face(cairo_context, "Fira Code",
    //   CAIRO_FONT_SLANT_NORMAL,
    //   CAIRO_FONT_WEIGHT_NORMAL);
    // cairo_set_font_size(cairo_context, sz);

#ifdef FONT_FIX_FIXED_WIDTH_EXTENTS
    for(int ti=0; ti<span.length;) {
        int ln = 1;

        // firables
        if (ti+1<span.length) {
            // look ahead
            for(int li=0;;li++) {
                if (_ligatures[li][0] == 0) {
                    break;
                }
                if (_ligatures[li][0] == *(text + (span.start + ti)) && 
                    _ligatures[li][1] == *(text + (span.start + ti + 1))) {
                    ln = 2;
                }
            }
        }
        
        cairo_set_source_rgb(cairo_context, (float)_clr.r / 255, (float)_clr.g / 255, (float)_clr.b / 255);
        cairo_move_to(cairo_context, x + (span.start + ti) * fnt->width, y);
        pango_layout_set_text(_pf->layout, text + (span.start + ti), ln);
        pango_cairo_show_layout(cairo_context, _pf->layout);
        ti += ln;
    }
#else

    cairo_set_source_rgb(cairo_context, (float)_clr.r / 255, (float)_clr.g / 255, (float)_clr.b / 255);
    cairo_move_to(cairo_context, x + span.start * fnt->width, y);
    pango_layout_set_text(_pf->layout, text + span.start, span.length);
    pango_cairo_show_layout(cairo_context, _pf->layout);

    // cairo_move_to(cairo_context, x + span.start * fnt->width, y+sz);
    // std::string t = std::string(text + span.start, span.length);
    // cairo_show_text(cairo_context, t.c_str());

#endif

    if (span.underline) {
        rect_t r = { x + span.start * fnt->width, y, fnt->width, fnt->height };
        r.y += r.h - (1 + UNDERLINE_WIDTH);
        r.h = UNDERLINE_WIDTH;
        r.w *= span.length;
        renderer->draw_rect(r, clr);
    }
    return 0;
}

int pango_font_draw_text(renderer_t* renderer, font_t* font, char* text, int x, int y, color_t clr, bool bold, bool italic, bool underline)
{
    int l = utf8_clength(text);

#ifdef FONT_PREBAKE_GLYPHS
    bool draw_with_prebaked_glyphs = false;

#ifdef FONT_FORCE_DRAW_PREBAKED_GLYPHS
    draw_with_prebaked_glyphs = true;
#endif

    if (!draw_with_prebaked_glyphs && l != strlen(text)) {
        draw_with_prebaked_glyphs = true;
    }

    if (draw_with_prebaked_glyphs) {
        std::wstring w = utf8string_to_wstring(text);
        return pango_font_draw_text(renderer, font, (wchar_t*)w.c_str(), x, y, clr, bold, italic, underline);
    }
#endif

    char* _s = text;

    cairo_t* cairo_context = ctx_cairo_context(renderer->context.get());
    pango_font_t* fnt = (pango_font_t*)font;

    if (!renderer->text_spans.size()) {
        pango_layout_set_text(fnt->layout, _s, l);
        cairo_set_source_rgb(cairo_context, (float)clr.r / 255, (float)clr.g / 255, (float)clr.b / 255);
        cairo_move_to(cairo_context, x, y);
        pango_cairo_show_layout(cairo_context, fnt->layout);
        return l * fnt->width;
    }

    int clen = strlen(text);
    int start = renderer->text_span_idx;
    text_span_t prev;
    prev.start = 0;
    prev.length = 0;

    int last_start = -1;
    for (int i = 0; i < clen; i++) {

        text_span_t bgc = span_from_index(renderer->text_spans, renderer->text_span_idx + i, true);
        // background & caret

        if (bgc.length) {
            if (color_is_set(bgc.bg)) {
                rect_t r = { x + i * fnt->width, y, fnt->width, fnt->height };
                renderer->draw_rect(r, bgc.bg, true);
            }
        }

        if (bgc.caret && bgc.length == 1) {
            rect_t r = { x + i * fnt->width, y, fnt->width, fnt->height };
            if (bgc.caret == 2) {
                r.x += r.w - CARET_WIDTH;
            }
            r.w = CARET_WIDTH;
            renderer->draw_rect(r, clr);
        }

        // text run
        text_span_t res = span_from_index(renderer->text_spans, renderer->text_span_idx + i, false);
        if (i == 0) {
            prev = res;
            prev.start = i;
        }

        if (!res.equals(prev)) {
            prev.length = start + i - prev.start;
            pango_font_draw_span(renderer, fnt, _s, x, y, prev, clr, bold, italic, underline);
            prev = res;
            prev.start = i;
        }
    }

    renderer->text_span_idx += clen;

    prev.length = start + clen - prev.start;
    pango_font_draw_span(renderer, fnt, _s, x, y, prev, clr, bold, italic, underline);

    return l * fnt->width;
}