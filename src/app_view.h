#ifndef APP_VIEW_H
#define APP_VIEW_H

#include "view.h"

struct app_view_t : view_t {
    app_view_t();

    DECLAR_VIEW_TYPE(CUSTOM, view_t)
    virtual std::string type_name() { return "app"; }

    void configure(int argc, char** argv);

    view_ptr sidebar;
    view_ptr tabs;
};

#endif // APP_VIEW_H