#include "js_app.h"

#include "app.h"

static JSValue js_log(JSContext* ctx, JSValueConst this_val,
    int argc, JSValueConst* argv)
{
    int i;
    const char* str;

    for (i = 0; i < argc; i++) {
        str = JS_ToCString(ctx, argv[i]);
        if (!str)
            return JS_EXCEPTION;
        app_t::log("%s", str);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}


JSValue JSApp_Init(JSContext* ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    JSValue app = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, app, "log", JS_NewCFunction(ctx, js_log, "log", 1));
    JS_SetPropertyStr(ctx, global_obj, "app", app);

    JS_FreeValue(ctx, global_obj);
    return app;
}