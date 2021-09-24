#include "completer_view.h"
#include "renderer.h"

completer_view::completer_view()
    : popup_view()
{
    class_name = "completer";
    list = std::make_shared<list_view>();
    content()->add_child(list);
    interactive = true;
}
