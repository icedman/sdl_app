#ifndef EXPLORER_VIEW_H
#define EXPLORER_VIEW_H

#include "list.h"

struct explorer_view_t : list_t {
    explorer_view_t();

    DECLAR_VIEW_TYPE(CUSTOM, list_t)
    virtual std::string type_name() { return "explorer"; }

    void set_root_path(std::string path);
    void update_explorer_data();
};

#endif // EXPLORER_VIEW_H