#include "inputtext.h"
#include "render_cache.h"
#include "renderer.h"

#include "app.h";

inputtext_view::inputtext_view()
    : horizontal_container()
{
    type = "inputtext";
    app_t* app = app_t::instance();

    editor = std::make_shared<editor_view>();
    editor_view* ev = view_item::cast<editor_view>(editor);

    int h = 26;
    int m = 4;

    ev->editor = std::make_shared<editor_t>();
    ev->editor->singleLineEdit = true;
    ev->editor->highlighter.lang = language_from_file("", app->extensions);
    ev->editor->highlighter.theme = app->theme;
    ev->editor->view = ev;
    ev->editor->pushOp("OPEN", "");
    ev->editor->runAllOps();
    ev->layout()->height = h;

    ev->v_scroll->disabled = true;
    ev->h_scroll->disabled = true;
    ev->v_scroll->layout()->visible = false;
    ev->h_scroll->layout()->visible = false;
    ev->resizer->layout()->visible = false;
    ev->gutter->layout()->visible = false;
    ev->minimap->layout()->visible = false;
    ev->bottom->layout()->visible = false;

    // ev->font = "ui";

    ev->layout()->margin = m;
    layout()->height = h + m + (m / 2);

    add_child(editor);
}

void inputtext_view::render()
{
    // background
    layout_item_ptr lo = layout();
    draw_rect({ lo->render_rect.x,
                  lo->render_rect.y,
                  lo->render_rect.w,
                  lo->render_rect.h },
        { 255, 0, 255 }, false, 1.0f);
}