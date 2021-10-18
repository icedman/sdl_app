#ifndef VIEW_H
#define VIEW_H

#include "events.h"
#include "layout.h"
#include "view_types.h"
#include "styled_frame.h"

#include <memory>
#include <string>
#include <vector>

#define STATE_HASH_SEED 0x0f
#define CONTENT_HASH_SEED 0x0e

struct font_t;
struct renderer_t;
struct damage_t;
struct view_t;
typedef std::shared_ptr<view_t> view_ptr;
typedef std::vector<view_ptr> view_list;

#define DECLAR_VIEW_TYPE(T, P)                           \
    virtual view_type_e type_of() override { return T; } \
    virtual bool is_type_of(view_type_e t) override { return t == T || P::is_type_of(t); }

struct view_t : events_manager_t, event_object_t {
    view_t();
    ~view_t();

    virtual view_type_e type_of() { return view_type_e::CONTAINER; }
    virtual bool is_type_of(view_type_e t) { return t == view_type_e::CONTAINER; }
    virtual std::string type_name();

    std::string uid;
    view_t* parent;

    virtual void prerender();
    virtual void render(renderer_t* renderer);
    virtual void render_frame(renderer_t* renderer);

    template <class T>
    static T* cast(view_ptr v) { return (T*)(v.get()); };

    template <class T>
    static T* cast(view_t* v) { return (T*)(v); };

    template <class T>
    T* cast() { return (T*)this; }

    void add_child(view_ptr view);
    void remove_child(view_ptr view);
    view_ptr find_child(std::string uid);
    view_ptr find_child(view_type_e t);
    view_ptr ptr();

    virtual void update();
    virtual void prelayout() {}
    virtual void postlayout() {}

    void relayout();

    static bool is_hovered(view_t* view);
    static bool is_focused(view_t* view);
    static bool is_dragged(view_t* view);
    static bool set_hovered(view_t* view);
    static bool set_focused(view_t* view);

    void propagate_event(event_t& event);

    layout_item_ptr layout();
    layout_item_ptr _layout;

    view_list children;

    // style
    void set_font(font_t* font);
    font_t* font();
    font_t* _font;

    bool disabled;
    bool can_focus;
    bool can_hover;

    struct view_state_t {
        bool hovered;
        bool pressed;
        rect_t rect;
        int scroll_x;
        int scroll_y;
        styled_frame_t style;
    } state;

    void set_style(styled_frame_t style);

    // hashes
    virtual int state_hash(bool peek = false);
    virtual int content_hash(bool peek = false);
    virtual void rerender();

    virtual view_ptr content() { return nullptr; }

    int _state_hash;
    int _content_hash;
};

struct spacer_t : view_t {
    DECLAR_VIEW_TYPE(view_type_e::SPACER, view_t)
};

struct vertical_container_t : view_t {
    vertical_container_t()
        : view_t()
    {
        layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;
    }
};

struct horizontal_container_t : view_t {
    horizontal_container_t()
        : view_t()
    {
        layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
    }
};

void view_dispatch_events(event_list& events, view_list& views);
void view_prerender(view_ptr view, view_list& visible_views, damage_t* damage = NULL);
void view_render(renderer_t* renderer, view_ptr view, damage_t* damage = NULL);

#endif // VIEW_H