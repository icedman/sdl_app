#include "prompt_view.h"
#include "editor.h"
#include "editor_view.h"
#include "explorer.h"
#include "indexer.h"
#include "input_text.h"
#include "popup.h"
#include "search.h"
#include "system.h"

#include <algorithm>

prompt_view_t::prompt_view_t(editor_view_t* e)
    : popup_t()
    , editor(e)
{
    can_focus = true;

    layout()->margin_top = 4;
    layout()->width = 400;

    view_ptr vc = std::make_shared<vertical_container_t>();

    input = std::make_shared<input_text_t>();

    vc->add_child(input);
    content()->add_child(vc);

    on(EVT_KEY_SEQUENCE, [this](event_t& event) {
        event.cancelled = true;
        this->handle_key_sequence(event);
        return true;
    });
    on(EVT_KEY_TEXT, [this](event_t& event) {
        event.cancelled = true;
        this->handle_key_text(event);
        return true;
    });

    // retain focus - always
    input->on(EVT_FOCUS_IN, [this](event_t& event) {
        view_t::set_focused(this);
        return true;
    });
}

bool prompt_view_t::update_data(std::string text)
{
    return true;
}

void prompt_view_t::render(renderer_t* renderer)
{
    layout_item_ptr lo = layout();
    color_t clr = color_darker(system_t::instance()->renderer.background, 20);
    renderer->draw_rect(lo->render_rect, clr, true, 0);
}

bool prompt_view_t::handle_key_sequence(event_t& event)
{
    operation_e op = operationFromKeys(event.text);
    switch (op) {
    case ENTER:
        commit();
        return true;
    }

    input->cast<input_text_t>()->handle_key_sequence(event);
    return true;
}

bool prompt_view_t::handle_key_text(event_t& event)
{
    input->cast<input_text_t>()->handle_key_text(event);
    return true;
}

bool prompt_view_t::commit()
{
    event_t evt;
    evt.type = EVT_ITEM_SELECT;
    evt.source = this;
    evt.cancelled = false;
    propagate_event(evt);
    return false;
}

bool prompt_filename_t::commit()
{
    editor_ptr editor = this->editor->editor;
    struct document_t* doc = &editor->document;

    std::string new_filename = input->cast<input_text_t>()->value();
    if (new_filename.length()) {
        doc->saveAs((explorer_t::instance()->rootPath + std::string("/") + new_filename).c_str(), true);
        doc->fileName = new_filename;
        prompt_view_t::commit();
        return true;
    }

    return false;
}