#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include "view.h"
#include "text.h"
#include "panel.h"

#include "cursor.h"
#include "editor.h"

struct editor_view : panel_view {
    editor_view();

    void prelayout() override;

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

    void _update_scrollbars();
    
    int start_row;
    int v_scroll_index;

    int mouse_x;
    int mouse_y;
    int rows;
    int cols;

    editor_ptr editor;
    view_item_ptr gutter;
    view_item_ptr minimap;

};


#endif // EDITOR_VIEW_H