#include "button.h"
#include "renderer.h"
#include "system.h"
#include "text.h"

button_t::button_t(std::string t)
    : horizontal_container_t()
{
    text = std::make_shared<text_t>(t);
    add_child(text);

    layout()->fit_children_x = true;
    layout()->fit_children_y = true;
    layout()->justify = LAYOUT_JUSTIFY_CENTER;
    layout()->align = LAYOUT_ALIGN_CENTER;
    layout()->margin = 8;
}

void button_t::render(renderer_t* renderer)
{
    render_frame(renderer);
}