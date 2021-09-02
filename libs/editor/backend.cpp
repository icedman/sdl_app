#include "backend.h"

static struct backend_t* backendInstance = 0;

struct backend_t* backend_t::instance()
{
    return backendInstance;
}

backend_t::backend_t()
{
    backendInstance = this;
}
