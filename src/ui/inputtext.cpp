#include "inputtext.h"
#include "renderer.h"

#include "app.h";

inputtext_view::inputtext_view()
    : horizontal_container()
{
    app_t *app = app_t::instance();

    editor = std::make_shared<editor_view>();
    editor_view *ev = view_item::cast<editor_view>(editor);

    ev->editor = std::make_shared<editor_t>();
    ev->editor->singleLineEdit = true;
    ev->editor->highlighter.lang = language_from_file("", app->extensions);
    ev->editor->highlighter.theme = app->theme;
    ev->editor->view = ev;
    ev->editor->pushOp("OPEN", "");
    ev->editor->runAllOps();
    ev->layout()->height = 32;

    ev->v_scroll->disabled = true;
    ev->h_scroll->disabled = true;
    ev->v_scroll->layout()->visible = false;
    ev->h_scroll->layout()->visible = false;
    ev->resizer->layout()->visible = false;
    ev->gutter->layout()->visible = false;
    ev->minimap->layout()->visible = false;
    ((view_item*)(ev->resizer->parent))->layout()->visible = false;

    layout()->margin = 4;
    layout()->height = 60;

    add_child(editor);
}
