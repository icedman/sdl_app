#include "button.h"
#include "renderer.h"
#include "system.h"
#include "text.h"
#include "icons.h"

button_t::button_t(std::string t)
    : horizontal_container_t()
{
    layout()->fit_children_x = true;
    layout()->fit_children_y = true;
    layout()->justify = LAYOUT_JUSTIFY_CENTER;
    layout()->align = LAYOUT_ALIGN_CENTER;
    // layout()->margin = 8;

    can_hover = true;

    if (t.length()) {
        set_text(t);
    }
}

void button_t::render(renderer_t* renderer)
{
    // render_frame(renderer);
}

void button_t::set_text(std::string str)
{
    text()->cast<text_t>()->set_text(str);
}

void button_t::set_icon(std::string img)
{

    icon()->cast<icon_view_t>()->set_image(system_t::instance()->renderer.image(img));
}

view_ptr button_t::text()
{ 
    if (!_text) {
        _text = std::make_shared<text_t>();
        add_child(_text);
    }
    return _text;
}

view_ptr button_t::icon()
{
    if (!_icon) {
        _icon = std::make_shared<icon_view_t>();
        add_child(_icon);
    }
    return _icon;
}
