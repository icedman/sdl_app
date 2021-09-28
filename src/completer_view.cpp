#include "completer_view.h"
#include "editor_view.h"
#include "panel.h"
#include "renderer.h"
#include "scrollarea.h"

#include "indexer.h"
#include "search.h"

completer_view::completer_view()
    : popup_view()
{
    class_name = "completer";
    list = std::make_shared<list_view>();
    content()->add_child(list);
    interactive = true;

    on(EVT_ITEM_SELECT, [this](event_t& e) {
        e.cancelled = true;
        list_item_view* item = (list_item_view*)e.target;
        return this->commit(item->data.value);
    });
}

// move to completer object
void completer_view::show_completer(editor_ptr e)
{
    editor = e;
    if (!editor || editor->singleLineEdit) {
        return;
    }

    editor_view* ev = (editor_view*)editor->view;
    popup_manager* pm = view_item::cast<popup_manager>(ev->popups);
    pm->clear();

    struct document_t* doc = &editor->document;
    if (doc->cursors.size() > 1) {
        return;
    }

    std::string prefix;

    cursor_t cursor = doc->cursor();
    cursor_t _cursor = cursor;
    struct block_t& block = *cursor.block();
    if (cursor.position() < 3)
        return;

    if (cursor.moveLeft(1)) {
        cursor.selectWord();
        prefix = cursor.selectedText();
        cursor.cursor = cursor.anchor;
        current_cursor = cursor;

        if (prefix.length() != _cursor.position() - cursor.position()) {
            return;
        }
    }

    if (prefix.length() < 2) {
        return;
    }

    completer_view* cm = this;
    list_view* list = view_item::cast<list_view>(cm->list);
    list->clear();

    int completerItemsWidth = 0;
    std::vector<std::string> words = editor->indexer->findWords(prefix);
    for (auto w : words) {
        if (w.length() <= prefix.length()) {
            continue;
        }
        int score = levenshtein_distance((char*)prefix.c_str(), (char*)(w.c_str()));

        if (completerItemsWidth < w.length()) {
            completerItemsWidth = w.length();
        }

        list_item_data_t item = {
            text : w,
            value : w
        };
        list->data.push_back(item);
        list->value = "";
    }

    if (!pm->_views.size() && list->data.size()) {
        int fw, fh;
        Renderer::instance()->get_font_extents(Renderer::instance()->font((char*)ev->font.c_str()), &fw, &fh, NULL, 1);

        blockdata_ptr blockData = current_cursor.block()->data;
        span_info_t s = spanAtBlock(blockData.get(), current_cursor.position(), true);
        int offset = current_cursor.position() - s.start;
        scrollarea_view* area = view_item::cast<scrollarea_view>(ev->scrollarea);

        int list_size = list->data.size();
        if (list_size > 4)
            list_size = 4;

        layout()->width = completerItemsWidth * fw;

        if (Renderer::instance()->is_terminal()) {
            layout()->width += 2;
            layout()->height = list_size;
        } else {
            layout()->height = list_size * 24;
        }

        list->focus_next();
        list->ensure_visible_cursor();

        int px = (s.x - pm->layout()->render_rect.x + offset * fw);
        int py = (s.y - ev->scrollarea->layout()->render_rect.y);

        pm->push_at(ev->completer,
            { px,
                py,
                fw * prefix.length(),
                fh },
            py > area->layout()->render_rect.h / 3 ? POPUP_DIRECTION_UP : POPUP_DIRECTION_DOWN);

        layout_request();
        Renderer::instance()->throttle_up_events();
    }
}

bool completer_view::commit(std::string text)
{
    cursor_t cur = editor->document.cursor();
    std::ostringstream ss;
    ss << (current_cursor.block()->lineNumber + 1);
    ss << ":";
    ss << current_cursor.position();
    editor->pushOp(MOVE_CURSOR, ss.str());
    ss.str("");
    ss.clear();
    ss << (cur.block()->lineNumber + 1);
    ss << ":";
    ss << (cur.position() - 1);
    editor->pushOp(MOVE_CURSOR_ANCHORED, ss.str());
    editor->pushOp(INSERT, text);
    editor->runAllOps();

    editor_view* ev = (editor_view*)editor->view;
    popup_manager* pm = view_item::cast<popup_manager>(ev->popups);
    pm->clear();

    return true;
}
