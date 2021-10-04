#ifndef COMPLETER_VIEW_H
#define COMPLETER_VIEW_H

#include "list.h"
#include "popup.h"
#include "view.h"

#include "block.h"
#include "cursor.h"
#include "editor.h"

struct completer_view : popup_view {
    completer_view();

    DECLAR_VIEW_TYPE(CUSTOM, popup_view)

    view_item_ptr list;
    editor_ptr editor;

    cursor_t current_cursor;

    void show_completer(editor_ptr editor);
    bool commit(std::string text);
};

#endif // COMPLETER_VIEW_H