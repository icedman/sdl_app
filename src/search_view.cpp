#include "search_view.h"
#include "renderer.h"
#include "list.h"
#include "inputtext.h"

struct custom_editor_view_t : editor_view {
    custom_editor_view_t() : editor_view()
    {}

    bool input_text(std::string text)
    {
        search->input_text(text);
        return editor_view::input_text(text);
    }

    bool input_sequence(std::string text)
    {
        operation_e op = operationFromKeys(text);

        switch (op) {
        case MOVE_CURSOR_UP:
        case MOVE_CURSOR_DOWN:
        case ENTER:
            // return list->input_sequence(text);
        break;
        }
        return editor_view::input_sequence(text);
    }

    search_view *search;
    view_item_ptr list;
};

search_view::search_view()
    : popup_view()
{
    class_name = "completer";
    list = std::make_shared<list_view>();
    input = std::make_shared<inputtext_view>();

    view_item_ptr _editor = std::make_shared<custom_editor_view_t>();
    view_item::cast<custom_editor_view_t>(_editor)->search = this;
    view_item::cast<custom_editor_view_t>(_editor)->list = list;
    view_item::cast<inputtext_view>(input)->set_editor(_editor);

    view_item_ptr container = std::make_shared<horizontal_container>();
    container->class_name = "editor";
    if (Renderer::instance()->is_terminal()) {
        container->layout()->height = 1;
    } else {
        container->layout()->margin = 4;
        container->layout()->height = 28;
    }
    container->add_child(input);

    content()->add_child(container);
    content()->add_child(list);

    interactive = true;
}

void search_view::show_search()
{
    list_view* lv = view_item::cast<list_view>(list);
    lv->layout()->visible = false;

    layout_request();
    Renderer::instance()->throttle_up_events();

    view_set_focused(view_item::cast<inputtext_view>(input)->editor.get());
}

bool search_view::input_text(std::string text)
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

    lv->layout()->visible = list_size > 0;

    layout_request();
    Renderer::instance()->throttle_up_events();

    return false;
}