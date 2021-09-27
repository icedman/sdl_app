#ifndef MINIMAP_VIEW_H
#define MINIMAP_VIEW_H

#include "block.h"
#include "scrollbar.h"
#include "view.h"

struct minimap_view : view_item {
    minimap_view();

    void update(int millis) override;
    void render() override;
    bool mouse_click(int x, int y, int button) override;

    void buildUpDotsForBlock(block_ptr block, float textCompress, int bufferWidth);
    void render_terminal();

    int scroll_y;
    int spacing;

    int start_row;
    int end_row;
    int render_h;
};

#endif // MINIMAP_VIEW_H