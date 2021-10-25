#ifndef RICH_TEXT_H
#define RICH_TEXT_H

#include "editor.h"
#include "panel.h"
#include "text_block.h"
#include "view.h"

struct rich_text_block_t : text_block_t {
    rich_text_block_t();

    DECLAR_VIEW_TYPE(TEXT, text_block_t)

    block_ptr block;
};

struct rich_text_t : panel_t {
    rich_text_t();

    editor_ptr editor;

    DECLAR_VIEW_TYPE(CUSTOM, panel_t)
    std::string type_name() override { return "rich_text"; }

    void prerender() override;
    void render(renderer_t* renderer) override;
    void prelayout() override;

    virtual view_ptr create_block();
    virtual void update_block(view_ptr item, block_ptr block);
    virtual void update_block(block_ptr block);
    virtual void update_blocks();
    virtual void relayout_virtual_blocks();
    virtual bool handle_mouse_wheel(event_t& event) override;
    virtual bool handle_scrollbar_move(event_t& event) override;

    int cursor_x(cursor_t cursor);
    int cursor_y(cursor_t cursor);
    point_t cursor_xy(cursor_t cursor);

    void ensure_visible_cursor();
    bool is_cursor_visible(cursor_t cursor);
    void scroll_to_cursor(cursor_t cursor);
    void scroll_up();
    void scroll_down();

    view_ptr lead_spacer;
    view_ptr tail_spacer;
    view_ptr subcontent;

    int first_visible;
    int visible_blocks;
    int block_height;
    bool wrapped;
    bool draw_cursors;

    color_t fg;
    color_t bg;
    color_t sel;

    int scroll_to_x;
    int scroll_to_y;

    int defer_relayout;
};

#endif RICH_TEXT_H