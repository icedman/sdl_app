#include "text.h"
#include "renderer.h"

text_view::text_view(std::string text)
    : view_item("text")
    , text(text)
{
    layout()->height = 40;
}

void text_view::precalculate()
{
    int fw, fh;
    ren_get_font_extents(NULL, &fw, &fh, text.c_str(), text.length(), false);

    int pad = 2;
    layout()->width = fw + pad*2;
    layout()->height = fh + pad*2;
}
