#include "renderer.h"
#include "app.h"
#include "view.h"

static int throttle_up_events_counter = 0;
static int throttle_down_rendering_counter = 0;

void Renderer::throttle_up_events(int frames)
{
    if (throttle_up_events_counter < frames) {
        throttle_up_events_counter = frames;
    }
}

bool Renderer::is_throttle_up_events()
{
    if (throttle_up_events_counter > 0) {
        throttle_up_events_counter--;
        return true;
    }
    return false;
}

static inline void render_item(layout_item_ptr item)
{
    if (!item->visible || item->offscreen) {
        return;
    }

    view_item* view = (view_item*)item->view;

    int pad = Renderer::instance()->is_terminal() ? 0 : 1;
    RenRect clip = {
        item->render_rect.x - pad,
        item->render_rect.y - pad,
        item->render_rect.w + (pad * 2),
        item->render_rect.h + (pad * 2)
    };

    Renderer::instance()->state_save();

    if (view) {
        if (view->_views.size()) {
            Renderer::instance()->set_clip_rect(clip);
        }

        if (view) {
            view->prerender();
            view->render();
        }
    } else {
        // debug layout only
        #if 1
        RenRect r = {
            item->render_rect.x,
            item->render_rect.y,
            item->render_rect.w,
            item->render_rect.h
        };
        Renderer::instance()->draw_rect(r, { 255,0,0,255 }, false, 1);
        Renderer::instance()->draw_text(NULL, item->name.c_str(), r.x, r.y, { 255,0,255 });
        #endif
    }

    for (auto child : item->children) {
        child->render_rect = child->rect;
        child->render_rect.x += item->render_rect.x + item->scroll_x;
        child->render_rect.y += item->render_rect.y + item->scroll_y;
        render_item(child);
    }

    Renderer::instance()->state_restore();
}

void Renderer::render_view_tree(view_item* root)
{
    render_item(root->layout());
}