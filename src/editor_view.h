#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include "panel.h"
#include "text.h"
#include "view.h"

#include "cursor.h"
#include "editor.h"

struct editor_view : panel_view {
    editor_view();

    void prelayout() override;

    bool mouse_down(int x, int y, int button, int clicks = 0) override;
    bool mouse_up(int x, int y, int button) override;
    bool mouse_move(int x, int y, int button) override;

    bool input_key(int k) override;
    bool input_text(std::string text) override;
    bool input_sequence(std::string text) override;

    void render() override;
    void update() override;

    void ensure_visible_cursor(bool animate = false);
    void scroll_to_cursor(cursor_t c, bool animate = false, bool centered = false);

    int start_row;
    int v_scroll_index;

    int mouse_x;
    int mouse_y;
    int rows;
    int cols;

    editor_ptr editor;
    view_item_ptr gutter;
    view_item_ptr minimap;

    std::string font;
    int fw, fh;
};

#endif // EDITOR_VIEW_H