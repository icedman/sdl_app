#include "search_view.h"
#include "renderer.h"
#include "list.h"

search_view::search_view()
    : popup_view()
{
    class_name = "completer";
    list = std::make_shared<list_view>();
    content()->add_child(list);
    interactive = true;
}

void search_view::show_search()
{
    list_view* lv = view_item::cast<list_view>(list);

    lv->data.clear();
    for(int i=0;i<20;i++) {
        std::string text = "item " + std::to_string(i);
        list_item_data_t item = {
            text : text,
            value : text
        };
        lv->data.push_back(item);
        lv->value = "";
    }

    int fw, fh;
    Renderer::instance()->get_font_extents(Renderer::instance()->font((char*)style.font.c_str()), &fw, &fh, NULL, 1);

    int list_size = lv->data.size();    
    if (list_size > 10) {
        list_size = 10;
    }

    int searchWidth = 30;
    layout()->width = searchWidth * fw;
    layout()->height = list_size * fh;

    layout_request();
    Renderer::instance()->throttle_up_events();

    view_set_focused(this);
}