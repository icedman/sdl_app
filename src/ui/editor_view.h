#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include "view.h"
#include "text.h"

#include "cursor.h"

struct editor_view : view_item {
    editor_view();

    bool mouse_wheel(int x, int y) override;
    bool mouse_down(int x, int y, int button, int clicks = 0) override;
    bool mouse_up(int x, int y, int button) override;
    bool mouse_move(int x, int y, int button) override;
    bool on_scroll() override; // scrollbar events
    
    void render() override;
    void update() override;
    
    bool input_key(int k) override;
    bool input_text(std::string text) override;
    bool input_sequence(std::string text) override;

    void ensure_visible_cursor(bool animate = false);
    void scroll_to_cursor(cursor_t c, bool animate = false, bool centered = false);

    int start_row;
    int scrollbar_index;

    view_item_ptr vscroll;
    view_item_ptr hscroll;

    int mouse_x;
    int mouse_y;
    int rows;
    int cols;
};


#endif // EDITOR_VIEW_H