#include "scripting.h"
#include "app.h"

#include <iostream>
#ifdef ENABLE_SCRIPTING
#include "js_app.h"
#endif

static Scripting scriptingEngine;

Scripting* Scripting::instance()
{
    return &scriptingEngine;
}

#ifdef ENABLE_SCRIPTING

static JSRuntime* rt = 0;
static JSContext* ctx = 0;

/* also used to initialize the worker context */
static JSContext* JS_NewCustomContext(JSRuntime* rt)
{
    JSContext* ctx;
    ctx = JS_NewContext(rt);
    if (!ctx)
        return NULL;
    /* system modules */
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");
    return ctx;
}

bool Scripting::init()
{
    rt = JS_NewRuntime();

    js_std_set_worker_new_context_func(JS_NewCustomContext);
    js_std_init_handlers(rt);
    ctx = JS_NewCustomContext(rt);
    if (!ctx) {
        fprintf(stderr, "qjs: cannot allocate JS context\n");
        return false;
    }

    /* loader for ES6 modules */
    JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);

    JSApp_Init(ctx);

    JSValue ret;

    std::string imports = "import * as std from 'std';";
    imports += "import * as os from 'os';";
    imports += "globalThis.os = os;";
    imports += "globalThis.std = std;";
    ret = JS_Eval(ctx, imports.c_str(), imports.length(), "<input>", JS_EVAL_TYPE_MODULE);
    if (JS_IsException(ret)) {
        app_t::log("JS err : %s\n", JS_ToCString(ctx, JS_GetException(ctx)));
        js_std_dump_error(ctx);
        JS_ResetUncatchableError(ctx);
        return false;
    }

    // std::string path = "./dist/app.js";
    // std::ifstream file(path, std::ifstream::in);
    // std::string script((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // // std::cout << script << std::endl;
    // ret = JS_Eval(ctx, script.c_str(), script.length(), path.c_str(), JS_EVAL_TYPE_GLOBAL);
    // if (JS_IsException(ret)) {
    //     // printf("JS err : %s\n", JS_ToCString(ctx, JS_GetException(ctx)));
    //     js_std_dump_error(ctx);
    //     JS_ResetUncatchableError(ctx);
    //     return 0;
    // }

    return false;
}

void Scripting::shutdown()
{
    if (rt) {
        js_std_free_handlers(rt);
        if (ctx) {
            JS_FreeContext(ctx);
        }
        JS_FreeRuntime(rt);
    }
}

void Scripting::update(int ticks)
{
    js_std_loop(ctx);
}

int Scripting::execute(std::string script)
{
    JSValue ret = JS_Eval(ctx, script.c_str(), script.length(), "<input>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(ret)) {
        js_std_dump_error(ctx);
        JS_ResetUncatchableError(ctx);
        return 0;
    }
}

void Scripting::restart()
{
    shutdown();
    init();
}

#else

bool Scripting::init() { return true; }
void Scripting::shutdown() {}
void Scripting::update(int ticks) {}
int Scripting::execute(std::string script) { return 0; }
void Scripting::restart() {}

#endif