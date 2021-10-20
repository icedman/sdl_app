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
    virtual std::string type_name() { return "rich_text"; }

    void render(renderer_t* renderer) override;
    void prelayout() override;

    virtual view_ptr create_block();
    virtual void update_block(view_ptr item, block_ptr block);
    virtual void update_block(block_ptr block);
    void update_blocks();
    void relayout_virtual_blocks();

    view_ptr lead_spacer;
    view_ptr tail_spacer;
    view_ptr subcontent;

    int first_visible;
    int visible_blocks;
    int block_height;
    bool wrapped;
    bool draw_cursors;
};

#endif RICH_TEXT_H