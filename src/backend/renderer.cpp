#include "renderer.h"
#include "app.h"
#include "view.h"

static int throttle_up_events_counter = 0;
static int throttle_down_rendering_counter = 0;

bool rects_overlap(RenRect a, RenRect b)
{
    if (a.x >= b.x + b.width || b.x >= a.x + a.width | a.y >= b.y + b.height || b.y >= a.y + a.height) {
        return false;
    }

    return true;
}

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

static inline void prerender_item(layout_item_ptr item)
{
    if (!item->visible || item->offscreen) {
        return;
    }

    view_item* view = (view_item*)item->view;

    if (view) {
        view->prerender();
    }

    for (auto child : item->children) {
        child->render_rect = child->rect;
        child->render_rect.x += item->render_rect.x + item->scroll_x;
        child->render_rect.y += item->render_rect.y + item->scroll_y;
        prerender_item(child);
    }
}

static inline void render_item(layout_item_ptr item)
{
    if (!item->visible || item->offscreen) {
        return;
    }

    view_item* view = (view_item*)item->view;

    RenRect r = {
        item->render_rect.x,
        item->render_rect.y,
        item->render_rect.w,
        item->render_rect.h
    };

    bool render = false;
    for (auto d : Renderer::instance()->damage_rects) {
        bool o = rects_overlap(d, r);
        if (o) {
            render = true;
            break;
        }
    }

    if (!render)
        return;

    int pad = Renderer::instance()->is_terminal() ? 0 : 1;
    RenRect clip = {
        item->render_rect.x - pad,
        item->render_rect.y - pad,
        item->render_rect.w + (pad * 2),
        item->render_rect.h + (pad * 2)
    };

    Renderer::instance()->state_save();

    if (true) {
        if (view) {
            Renderer::instance()->set_clip_rect(clip);
            view->render();
        } else if (view_item::debug_render) {
            view_style_t vs = style_get("default");
            Renderer::instance()->draw_rect(r, { item->rgb.r, item->rgb.g, item->rgb.b, 0 }, false, 1);
            Renderer::instance()->draw_text(NULL, item->name.c_str(), r.x, r.y, { vs.fg.red, vs.fg.green, vs.fg.blue, 0 });
        }
    }

    for (auto child : item->children) {
        render_item(child);
    }

    Renderer::instance()->state_restore();
}

void Renderer::render_view_tree(view_item* root)
{
    render_item(root->layout());
}

void Renderer::prerender_view_tree(view_item* root)
{
    prerender_item(root->layout());
}

void Renderer::damage(RenRect rect)
{
    damage_rects.push_back(rect);
}