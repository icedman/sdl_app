#ifndef MINIMAP_VIEW_H
#define MINIMAP_VIEW_H

#include "block.h"
#include "scrollbar.h"
#include "view.h"

struct minimap_view : view_item {
    minimap_view();

    void update() override;
    void render() override;
    bool mouse_click(int x, int y, int button) override;

    void buildUpDotsForBlock(block_ptr block, float textCompress, int bufferWidth);
    void render_terminal();

    int scroll_y;
    int spacing;

    int start_y;
    int end_y;
    int render_h;
};

#endif // MINIMAP_VIEW_H