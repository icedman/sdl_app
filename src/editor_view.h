#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include "panel.h"
#include "text.h"
#include "view.h"

#include "block.h"
#include "cursor.h"
#include "editor.h"

struct editor_view : panel_view {
    editor_view();

    DECLAR_VIEW_TYPE(CUSTOM, panel_view)

    void prelayout() override;

    bool mouse_down(int x, int y, int button, int clicks = 0) override;
    bool mouse_up(int x, int y, int button) override;
    bool mouse_move(int x, int y, int button) override;

    bool input_key(int k) override;
    bool input_text(std::string text) override;
    bool input_sequence(std::string text) override;

    void prerender() override;
    void render() override;
    void update(int millis) override;

    void ensure_visible_cursor();
    void scroll_to_cursor(cursor_t c, bool centered = false);

    int start_row;
    int end_row;
    int v_scroll_index;

    int target_start_row;

    int mouse_x;
    int mouse_y;
    int rows;
    int cols;

    size_t computed_lines;

    editor_ptr editor;
    block_ptr longest_block;
    view_item_ptr gutter;
    view_item_ptr minimap;
    view_item_ptr completer;
    view_item_ptr popups;

    std::string font;
    int fw, fh;

    bool showMinimap;
    bool showGutter;

    block_ptr start_block;
    block_ptr end_block;
    block_ptr prev_start_block;
    block_ptr prev_end_block;
    int prev_doc_size;
    int prev_computed_lines;

    std::vector<RenRect> previous_block_damages;
};

#endif // EDITOR_VIEW_H