#include "inputtext.h"
#include "renderer.h"

#include "app.h";

inputtext_view::inputtext_view(std::string text)
    : horizontal_container()
{
    class_name = "inputtext";

    view_item_ptr editor = std::make_shared<editor_view>();
    view_item::cast<editor_view>(editor)->font = "ui-small";
    set_editor(editor);
    set_value(text);
}

void inputtext_view::prelayout()
{
    int fw, fh;
    Renderer::instance()->get_font_extents(NULL, &fw, &fh, NULL, 1);
    int m = 4;
    if (Renderer::instance()->is_terminal()) {
        m = 0;
    }
    layout()->height = (1.5f * fh) + (m * 2);
}

void inputtext_view::render()
{
    return;
    // background
    layout_item_ptr lo = layout();
    Renderer::instance()->draw_rect({ lo->render_rect.x,
                                        lo->render_rect.y,
                                        lo->render_rect.w,
                                        lo->render_rect.h },
        { 255, 255, 255, 10 }, false, 1.0f);
}

void inputtext_view::set_editor(view_item_ptr _editor)
{
    if (editor) {
        remove_child(editor);
    }

    app_t* app = app_t::instance();

    editor = _editor;
    editor_view* ev = view_item::cast<editor_view>(editor);

    ev->editor = std::make_shared<editor_t>();
    ev->editor->singleLineEdit = true;
    ev->editor->highlighter.lang = language_from_file("", app->extensions);
    ev->editor->highlighter.theme = app->theme;
    ev->editor->view = ev;
    ev->editor->pushOp("OPEN", "");
    ev->editor->runAllOps();
    ev->layout()->height = layout()->height;

    ev->v_scroll->disabled = true;
    ev->h_scroll->disabled = true;
    ev->v_scroll->layout()->visible = false;
    ev->h_scroll->layout()->visible = false;
    ev->resizer->layout()->visible = false;
    ev->gutter->layout()->visible = false;
    ev->minimap->layout()->visible = false;
    ev->bottom->layout()->visible = false;

    ev->showMinimap = false;
    ev->showGutter = false;

    add_child(editor);
}

std::string inputtext_view::value()
{
    return view_item::cast<editor_view>(editor)->editor->document.blocks.back()->text();
}

void inputtext_view::set_value(std::string value)
{
    return view_item::cast<editor_view>(editor)->editor->document.blocks.back()->setText(value);
}