#ifndef MINIMAP_VIEW_H
#define MINIMAP_VIEW_H

#include "block.h"
#include "scrollbar.h"
#include "view.h"

struct minimap_view : view_item {
    minimap_view();

    DECLAR_VIEW_TYPE(CUSTOM, view_item)

    void prerender() override;
    void update(int millis) override;
    void render() override;
    bool mouse_click(int x, int y, int button) override;

    void buildUpDotsForBlock(block_ptr block, float textCompress, int bufferWidth);
    void render_terminal();

    view_item_ptr scrollbar;

    int scroll_y;
    int spacing;

    int start_row;
    int end_row;

    float sliding_y;
    int render_y;
    int render_h;

    float prev_sliding_y;
    int prev_scroll_y;
    int prev_start_row;
    int prev_end_row;
};

#endif // MINIMAP_VIEW_H
