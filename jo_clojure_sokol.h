#pragma once

#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
//#define SOKOL_DEBUG
//#define SOKOL_TRACE_HOOKS 

#ifdef _WIN32
#define JO_BASIC_WINDOWS
#define SOKOL_D3D11
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if defined(TARGET_OS_IPHONE) && !TARGET_OS_IPHONE
#define JO_BASIC_MACOS
#define SOKOL_METAL
#else 
#define JO_BASIC_IOS
#define SOKOL_METAL
#endif
#endif

#if defined(__linux__) || defined(__unix__)
#define JO_BASIC_LINUX
#define SOKOL_GLCORE33
#endif

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "imgui/imgui.h"
#include "sokol_imgui.h"
#include "sokol_gl.h"

// overall state of the sgl stuffs
static struct {
    sg_pass_action pass_action;
    sgl_pipeline pip_3d;
} sokol_state;

static node_idx_t get_map_idx(hash_map_ptr_t map, const char *key, node_idx_t default_value) {
    auto it = map->find(new_node_string(key), node_sym_eq);
    if(it.third) {
        return it.second;
    }
    return default_value;
}

static int get_map_int(hash_map_ptr_t map, const char *key, int default_value) {
    auto it = map->find(new_node_string(key), node_sym_eq);
    if(it.third) {
        return get_node_int(it.second);
    }
    return default_value;
}

static jo_string get_map_string(hash_map_ptr_t map, const char *key, const char *default_value) {
    auto it = map->find(new_node_string(key), node_sym_eq);
    if(it.third) {
        return get_node_string(it.second);
    }
    return default_value;
}

static node_idx_t native_sokol_run(env_ptr_t env, list_ptr_t args) {
    node_idx_t desc_idx = args->first_value();
	if(get_node_type(desc_idx) != NODE_HASH_MAP) {
        warnf("sapp_run: expected hash map as first argument\n");
        return NIL_NODE;
    }
    node_t *desc_node = get_node(desc_idx);
    hash_map_ptr_t desc_map = desc_node->as_hash_map();
    sapp_desc d = {0};
    d.init_cb = [=]() {
        // init sokol_gfx
        sg_desc sgd = {0};
        sgd.context = sapp_sgcontext();
        sg_setup(&sgd);
        simgui_desc_t simgui_desc = {0};
        simgui_setup(&simgui_desc);
        sgl_desc_t sgl_desc = {0};
        sgl_setup(&sgl_desc);
        sokol_state.pass_action.colors[0].action = SG_ACTION_CLEAR;
        sokol_state.pass_action.colors[0].value = {0.0f, 0.0f, 0.0f, 1.0f};
        node_idx_t init_cb_idx = get_map_idx(desc_map, "init_cb", NIL_NODE);
        if(init_cb_idx != NIL_NODE) {
            eval_node(env, new_node_list(list_va(init_cb_idx)));
        }
    };
    d.frame_cb = [=]() {
        simgui_frame_desc_t frame_desc = {0};
        frame_desc.width = sapp_width();
        frame_desc.height = sapp_height();
        frame_desc.delta_time = sapp_frame_duration();
        frame_desc.dpi_scale = sapp_dpi_scale();
        simgui_new_frame(&frame_desc);

        node_idx_t frame_cb_idx = get_map_idx(desc_map, "frame_cb", NIL_NODE);
        if(frame_cb_idx != NIL_NODE) {
            eval_node(env, new_node_list(list_va(frame_cb_idx)));
        }

        sg_begin_default_pass(&sokol_state.pass_action, sapp_width(), sapp_height());
        sgl_draw();
        simgui_render();
        sg_end_pass();
        sg_commit();
    };
    d.cleanup_cb = [=]() {
        node_idx_t cleanup_cb_idx = get_map_idx(desc_map, "cleanup_cb", NIL_NODE);
        if(cleanup_cb_idx != NIL_NODE) {
            eval_node(env, new_node_list(list_va(cleanup_cb_idx)));
        }
        sgl_shutdown();
        simgui_shutdown();
        sg_shutdown();
    };
    d.event_cb = [=](const sapp_event*ev) {
        simgui_handle_event(ev);
        node_idx_t event_cb_idx = get_map_idx(desc_map, "event_cb", NIL_NODE);
        if(event_cb_idx != NIL_NODE) {
            hash_map_ptr_t event_map = new_hash_map();
            switch(ev->type) {
                case SAPP_EVENTTYPE_INVALID: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("invalid"), node_sym_eq); break;
                case SAPP_EVENTTYPE_KEY_DOWN: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("key-down"), node_sym_eq); break;
                case SAPP_EVENTTYPE_KEY_UP: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("key-up"), node_sym_eq); break;
                case SAPP_EVENTTYPE_CHAR: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("char"), node_sym_eq); break;
                case SAPP_EVENTTYPE_MOUSE_DOWN: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("mouse-down"), node_sym_eq); break;
                case SAPP_EVENTTYPE_MOUSE_UP: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("mouse-up"), node_sym_eq); break;
                case SAPP_EVENTTYPE_MOUSE_SCROLL: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("mouse-scroll"), node_sym_eq); break;
                case SAPP_EVENTTYPE_MOUSE_MOVE: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("mouse-move"), node_sym_eq); break;
                case SAPP_EVENTTYPE_MOUSE_ENTER: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("mouse-enter"), node_sym_eq); break;
                case SAPP_EVENTTYPE_MOUSE_LEAVE: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("mouse-leave"), node_sym_eq); break;
                case SAPP_EVENTTYPE_TOUCHES_BEGAN: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("touches-began"), node_sym_eq); break;
                case SAPP_EVENTTYPE_TOUCHES_MOVED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("touches-moved"), node_sym_eq); break;
                case SAPP_EVENTTYPE_TOUCHES_ENDED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("touches-ended"), node_sym_eq); break;
                case SAPP_EVENTTYPE_TOUCHES_CANCELLED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("touches-cancelled"), node_sym_eq); break;
                case SAPP_EVENTTYPE_RESIZED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("resized"), node_sym_eq); break;
                case SAPP_EVENTTYPE_ICONIFIED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("iconified"), node_sym_eq); break;
                case SAPP_EVENTTYPE_RESTORED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("restored"), node_sym_eq); break;
                case SAPP_EVENTTYPE_FOCUSED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("focused"), node_sym_eq); break;
                case SAPP_EVENTTYPE_UNFOCUSED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("unfocused"), node_sym_eq); break;
                case SAPP_EVENTTYPE_SUSPENDED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("suspended"), node_sym_eq); break;
                case SAPP_EVENTTYPE_RESUMED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("resumed"), node_sym_eq); break;
                case SAPP_EVENTTYPE_QUIT_REQUESTED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("quit-requested"), node_sym_eq); break;
                case SAPP_EVENTTYPE_CLIPBOARD_PASTED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("clipboard-pasted"), node_sym_eq); break;
                case SAPP_EVENTTYPE_FILES_DROPPED: event_map->assoc_inplace(new_node_string("type"), new_node_keyword("files-dropped"), node_sym_eq); break;
                default: event_map->assoc_inplace(new_node_string("type"), new_node_int(ev->type), node_sym_eq); break;
            }
            event_map->assoc_inplace(new_node_string("frame_count"), new_node_int(ev->frame_count), node_sym_eq);
            switch(ev->key_code) {
                case SAPP_KEYCODE_INVALID: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("invalid"), node_sym_eq); break;
                case SAPP_KEYCODE_SPACE: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("space"), node_sym_eq); break;
                case SAPP_KEYCODE_APOSTROPHE: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("apostrophe"), node_sym_eq); break;
                case SAPP_KEYCODE_COMMA: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("comma"), node_sym_eq); break;
                case SAPP_KEYCODE_MINUS: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("minus"), node_sym_eq); break;
                case SAPP_KEYCODE_PERIOD: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("period"), node_sym_eq); break;
                case SAPP_KEYCODE_SLASH: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("slash"), node_sym_eq); break;
                case SAPP_KEYCODE_0: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("0"), node_sym_eq); break;
                case SAPP_KEYCODE_1: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("1"), node_sym_eq); break;
                case SAPP_KEYCODE_2: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("2"), node_sym_eq); break;
                case SAPP_KEYCODE_3: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("3"), node_sym_eq); break;
                case SAPP_KEYCODE_4: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("4"), node_sym_eq); break;
                case SAPP_KEYCODE_5: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("5"), node_sym_eq); break;
                case SAPP_KEYCODE_6: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("6"), node_sym_eq); break;
                case SAPP_KEYCODE_7: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("7"), node_sym_eq); break;
                case SAPP_KEYCODE_8: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("8"), node_sym_eq); break;
                case SAPP_KEYCODE_9: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("9"), node_sym_eq); break;
                case SAPP_KEYCODE_SEMICOLON: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("semicolon"), node_sym_eq); break;
                case SAPP_KEYCODE_EQUAL: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("equal"), node_sym_eq); break;
                case SAPP_KEYCODE_A: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("a"), node_sym_eq); break;
                case SAPP_KEYCODE_B: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("b"), node_sym_eq); break;
                case SAPP_KEYCODE_C: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("c"), node_sym_eq); break;
                case SAPP_KEYCODE_D: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("d"), node_sym_eq); break;
                case SAPP_KEYCODE_E: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("e"), node_sym_eq); break;
                case SAPP_KEYCODE_F: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f"), node_sym_eq); break;
                case SAPP_KEYCODE_G: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("g"), node_sym_eq); break;
                case SAPP_KEYCODE_H: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("h"), node_sym_eq); break;
                case SAPP_KEYCODE_I: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("i"), node_sym_eq); break;
                case SAPP_KEYCODE_J: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("j"), node_sym_eq); break;
                case SAPP_KEYCODE_K: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("k"), node_sym_eq); break;
                case SAPP_KEYCODE_L: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("l"), node_sym_eq); break;
                case SAPP_KEYCODE_M: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("m"), node_sym_eq); break;
                case SAPP_KEYCODE_N: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("n"), node_sym_eq); break;
                case SAPP_KEYCODE_O: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("o"), node_sym_eq); break;
                case SAPP_KEYCODE_P: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("p"), node_sym_eq); break;
                case SAPP_KEYCODE_Q: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("q"), node_sym_eq); break;
                case SAPP_KEYCODE_R: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("r"), node_sym_eq); break;
                case SAPP_KEYCODE_S: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("s"), node_sym_eq); break;
                case SAPP_KEYCODE_T: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("t"), node_sym_eq); break;
                case SAPP_KEYCODE_U: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("u"), node_sym_eq); break;
                case SAPP_KEYCODE_V: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("v"), node_sym_eq); break;
                case SAPP_KEYCODE_W: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("w"), node_sym_eq); break;
                case SAPP_KEYCODE_X: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("x"), node_sym_eq); break;
                case SAPP_KEYCODE_Y: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("y"), node_sym_eq); break;
                case SAPP_KEYCODE_Z: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("z"), node_sym_eq); break;
                case SAPP_KEYCODE_LEFT_BRACKET: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("left-bracket"), node_sym_eq); break;
                case SAPP_KEYCODE_BACKSLASH: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("backslash"), node_sym_eq); break;
                case SAPP_KEYCODE_RIGHT_BRACKET: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("right-bracket"), node_sym_eq); break;
                case SAPP_KEYCODE_GRAVE_ACCENT: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("grave-accent"), node_sym_eq); break;
                case SAPP_KEYCODE_WORLD_1: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("world-1"), node_sym_eq); break;
                case SAPP_KEYCODE_WORLD_2: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("world-2"), node_sym_eq); break;
                case SAPP_KEYCODE_ESCAPE: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("escape"), node_sym_eq); break;
                case SAPP_KEYCODE_ENTER: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("enter"), node_sym_eq); break;
                case SAPP_KEYCODE_TAB: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("tab"), node_sym_eq); break;
                case SAPP_KEYCODE_BACKSPACE: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("backspace"), node_sym_eq); break;
                case SAPP_KEYCODE_INSERT: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("insert"), node_sym_eq); break;
                case SAPP_KEYCODE_DELETE: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("delete"), node_sym_eq); break;
                case SAPP_KEYCODE_RIGHT: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("right"), node_sym_eq); break;
                case SAPP_KEYCODE_LEFT: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("left"), node_sym_eq); break;
                case SAPP_KEYCODE_DOWN: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("down"), node_sym_eq); break;
                case SAPP_KEYCODE_UP: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("up"), node_sym_eq); break;
                case SAPP_KEYCODE_PAGE_UP: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("page-up"), node_sym_eq); break;
                case SAPP_KEYCODE_PAGE_DOWN: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("page-down"), node_sym_eq); break;
                case SAPP_KEYCODE_HOME: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("home"), node_sym_eq); break;
                case SAPP_KEYCODE_END: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("end"), node_sym_eq); break;
                case SAPP_KEYCODE_CAPS_LOCK: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("caps-lock"), node_sym_eq); break;
                case SAPP_KEYCODE_SCROLL_LOCK: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("scroll-lock"), node_sym_eq); break;
                case SAPP_KEYCODE_NUM_LOCK: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("num-lock"), node_sym_eq); break;
                case SAPP_KEYCODE_PRINT_SCREEN: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("print-screen"), node_sym_eq); break;
                case SAPP_KEYCODE_PAUSE: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("pause"), node_sym_eq); break;
                case SAPP_KEYCODE_F1: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f1"), node_sym_eq); break;
                case SAPP_KEYCODE_F2: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f2"), node_sym_eq); break;
                case SAPP_KEYCODE_F3: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f3"), node_sym_eq); break;
                case SAPP_KEYCODE_F4: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f4"), node_sym_eq); break;
                case SAPP_KEYCODE_F5: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f5"), node_sym_eq); break;
                case SAPP_KEYCODE_F6: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f6"), node_sym_eq); break;
                case SAPP_KEYCODE_F7: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f7"), node_sym_eq); break;
                case SAPP_KEYCODE_F8: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f8"), node_sym_eq); break;
                case SAPP_KEYCODE_F9: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f9"), node_sym_eq); break;
                case SAPP_KEYCODE_F10: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f10"), node_sym_eq); break;
                case SAPP_KEYCODE_F11: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f11"), node_sym_eq); break;
                case SAPP_KEYCODE_F12: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f12"), node_sym_eq); break;
                case SAPP_KEYCODE_F13: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f13"), node_sym_eq); break;
                case SAPP_KEYCODE_F14: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f14"), node_sym_eq); break;
                case SAPP_KEYCODE_F15: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f15"), node_sym_eq); break;
                case SAPP_KEYCODE_F16: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f16"), node_sym_eq); break;
                case SAPP_KEYCODE_F17: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f17"), node_sym_eq); break;
                case SAPP_KEYCODE_F18: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f18"), node_sym_eq); break;
                case SAPP_KEYCODE_F19: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f19"), node_sym_eq); break;
                case SAPP_KEYCODE_F20: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f20"), node_sym_eq); break;
                case SAPP_KEYCODE_F21: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f21"), node_sym_eq); break;
                case SAPP_KEYCODE_F22: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f22"), node_sym_eq); break;
                case SAPP_KEYCODE_F23: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f23"), node_sym_eq); break;
                case SAPP_KEYCODE_F24: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f24"), node_sym_eq); break;
                case SAPP_KEYCODE_F25: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("f25"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_0: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-0"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_1: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-1"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_2: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-2"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_3: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-3"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_4: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-4"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_5: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-5"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_6: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-6"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_7: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-7"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_8: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-8"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_9: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-9"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_DECIMAL: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-decimal"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_DIVIDE: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-divide"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_MULTIPLY: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-multiply"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_SUBTRACT: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-subtract"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_ADD: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-add"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_ENTER: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-enter"), node_sym_eq); break;
                case SAPP_KEYCODE_KP_EQUAL: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("kp-equal"), node_sym_eq); break;
                case SAPP_KEYCODE_LEFT_SHIFT: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("left-shift"), node_sym_eq); break;
                case SAPP_KEYCODE_LEFT_CONTROL: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("left-control"), node_sym_eq); break;
                case SAPP_KEYCODE_LEFT_ALT: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("left-alt"), node_sym_eq); break;
                case SAPP_KEYCODE_LEFT_SUPER: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("left-super"), node_sym_eq); break;
                case SAPP_KEYCODE_RIGHT_SHIFT: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("right-shift"), node_sym_eq); break;
                case SAPP_KEYCODE_RIGHT_CONTROL: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("right-control"), node_sym_eq); break;
                case SAPP_KEYCODE_RIGHT_ALT: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("right-alt"), node_sym_eq); break;
                case SAPP_KEYCODE_RIGHT_SUPER: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("right-super"), node_sym_eq); break;
                case SAPP_KEYCODE_MENU: event_map->assoc_inplace(new_node_string("key"), new_node_keyword("menu"), node_sym_eq); break;
                default: event_map->assoc_inplace(new_node_string("key"), new_node_int(ev->key_code), node_sym_eq); break;
            }
            event_map->assoc_inplace(new_node_string("char"), new_node_int(ev->char_code), node_sym_eq);
            event_map->assoc_inplace(new_node_string("key-repeat"), new_node_int(ev->key_repeat), node_sym_eq);
            event_map->assoc_inplace(new_node_string("modifiers"), new_node_int(ev->modifiers), node_sym_eq);
            switch(ev->mouse_button) {
                case SAPP_MOUSEBUTTON_INVALID: event_map->assoc_inplace(new_node_string("mouse-button"), new_node_keyword("invalid"), node_sym_eq); break;
                case SAPP_MOUSEBUTTON_LEFT: event_map->assoc_inplace(new_node_string("mouse-button"), new_node_keyword("left"), node_sym_eq); break;
                case SAPP_MOUSEBUTTON_RIGHT: event_map->assoc_inplace(new_node_string("mouse-button"), new_node_keyword("right"), node_sym_eq); break;
                case SAPP_MOUSEBUTTON_MIDDLE: event_map->assoc_inplace(new_node_string("mouse-button"), new_node_keyword("middle"), node_sym_eq); break;
                default: event_map->assoc_inplace(new_node_string("mouse-button"), new_node_int(ev->mouse_button), node_sym_eq); break;
            }
            event_map->assoc_inplace(new_node_string("mouse-x"), new_node_float(ev->mouse_x), node_sym_eq);
            event_map->assoc_inplace(new_node_string("mouse-y"), new_node_float(ev->mouse_y), node_sym_eq);
            event_map->assoc_inplace(new_node_string("mouse-dx"), new_node_float(ev->mouse_dx), node_sym_eq);
            event_map->assoc_inplace(new_node_string("mouse-dy"), new_node_float(ev->mouse_dy), node_sym_eq);
            event_map->assoc_inplace(new_node_string("scroll-x"), new_node_float(ev->scroll_x), node_sym_eq);
            event_map->assoc_inplace(new_node_string("scroll-y"), new_node_float(ev->scroll_y), node_sym_eq);
            event_map->assoc_inplace(new_node_string("num-touches"), new_node_int(ev->num_touches), node_sym_eq);
            if(ev->num_touches) {
                list_ptr_t touches = new_list();
                for(int i = 0; i < ev->num_touches; ++i) {
                    hash_map_ptr_t touch_map = new_hash_map();
                    touch_map->assoc_inplace(new_node_string("identifier"), new_node_int(ev->touches[i].identifier), node_sym_eq);
                    touch_map->assoc_inplace(new_node_string("pos-x"), new_node_float(ev->touches[i].pos_x), node_sym_eq);
                    touch_map->assoc_inplace(new_node_string("pos-y"), new_node_float(ev->touches[i].pos_y), node_sym_eq);
                    switch(ev->touches[i].android_tooltype) {
                        case SAPP_ANDROIDTOOLTYPE_UNKNOWN: touch_map->assoc_inplace(new_node_string("android-tooltype"), new_node_keyword("unknown"), node_sym_eq); break;
                        case SAPP_ANDROIDTOOLTYPE_FINGER: touch_map->assoc_inplace(new_node_string("android-tooltype"), new_node_keyword("finger"), node_sym_eq); break;
                        case SAPP_ANDROIDTOOLTYPE_STYLUS: touch_map->assoc_inplace(new_node_string("android-tooltype"), new_node_keyword("stylus"), node_sym_eq); break;
                        case SAPP_ANDROIDTOOLTYPE_MOUSE: touch_map->assoc_inplace(new_node_string("android-tooltype"), new_node_keyword("mouse"), node_sym_eq); break;
                        default: touch_map->assoc_inplace(new_node_string("android-tooltype"), new_node_int(ev->touches[i].android_tooltype), node_sym_eq); break;
                    }
                    touch_map->assoc_inplace(new_node_string("changed"), new_node_int(ev->touches[i].changed), node_sym_eq);
                    touches->push_back_inplace(new_node_hash_map(touch_map));
                }
                event_map->assoc_inplace(new_node_string("touches"), new_node_list(touches), node_sym_eq);
            }
            event_map->assoc_inplace(new_node_string("window-width"), new_node_int(ev->window_width), node_sym_eq);
            event_map->assoc_inplace(new_node_string("window-height"), new_node_int(ev->window_height), node_sym_eq);
            event_map->assoc_inplace(new_node_string("framebuffer-width"), new_node_int(ev->framebuffer_width), node_sym_eq);
            event_map->assoc_inplace(new_node_string("framebuffer-height"), new_node_int(ev->framebuffer_height), node_sym_eq);
            eval_node(env, new_node_list(list_va(event_cb_idx, new_node_hash_map(event_map))));
        }
    };
    d.fail_cb = [=](const char *msg) {
        node_idx_t fail_cb_idx = get_map_idx(desc_map, "fail_cb", NIL_NODE);
        if(fail_cb_idx != NIL_NODE) {
            eval_node(env, new_node_list(list_va(fail_cb_idx, new_node_string(msg))));
        }
    };
    d.width = get_map_int(desc_map, "width", 640);
    d.height = get_map_int(desc_map, "height", 480);
    d.sample_count = get_map_int(desc_map, "sample_count", 1);
    d.swap_interval = get_map_int(desc_map, "swap_interval", 1);
    d.high_dpi = get_map_int(desc_map, "high_dpi", 0);
    d.fullscreen = get_map_int(desc_map, "fullscreen", 0);
    d.alpha = get_map_int(desc_map, "alpha", 0);
    d.window_title = jo_strdup(get_map_string(desc_map, "window_title", "Basic App").c_str()); // mem leak... but who cares
    d.enable_clipboard = get_map_int(desc_map, "enable_clipboard", 0);
    d.clipboard_size = get_map_int(desc_map, "clipboard_size", 0);
    d.enable_dragndrop = get_map_int(desc_map, "enable_dragndrop", 0);
    d.max_dropped_files = get_map_int(desc_map, "max_dropped_files", 1);
    d.max_dropped_file_path_length = get_map_int(desc_map, "max_dropped_file_path_length", 2048);
    d.gl_force_gles2 = get_map_int(desc_map, "gl_force_gles2", 0);
    d.gl_major_version = get_map_int(desc_map, "gl_major_version", 3);
    d.gl_minor_version = get_map_int(desc_map, "gl_minor_version", 2);
    d.win32_console_utf8 = get_map_int(desc_map, "win32_console_utf8", 0);
    d.win32_console_create = get_map_int(desc_map, "win32_console_create", 0);
    d.win32_console_attach = get_map_int(desc_map, "win32_console_attach", 0);
    d.html5_canvas_name = jo_strdup(get_map_string(desc_map, "html5_canvas_name", "canvas").c_str()); // mem leak... but who cares
    d.html5_canvas_resize = get_map_int(desc_map, "html5_canvas_resize", 0); 
    d.html5_preserve_drawing_buffer = get_map_int(desc_map, "html5_preserve_drawing_buffer", 0);
    d.html5_premultiplied_alpha = get_map_int(desc_map, "html5_premultiplied_alpha", 0); 
    d.html5_ask_leave_site = get_map_int(desc_map, "html5_ask_leave_site", 0);
    d.ios_keyboard_resizes_canvas = get_map_int(desc_map, "ios_keyboard_resizes_canvas", 0);
    sapp_run(&d);
    return 0;
}

/* returns true after sokol-app has been initialized */
static node_idx_t native_sokol_valid_q(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(sapp_isvalid());
}

/* returns the current framebuffer width in pixels */
static node_idx_t native_sokol_width(env_ptr_t env, list_ptr_t args) {
    return new_node_int(sapp_width());
}

static node_idx_t native_sokol_height(env_ptr_t env, list_ptr_t args) {
    return new_node_int(sapp_height());
}

/* get default framebuffer color pixel format */
static node_idx_t native_sokol_color_format(env_ptr_t env, list_ptr_t args) {
    return new_node_int(sapp_color_format());
}

/* get default framebuffer depth pixel format */
static node_idx_t native_sokol_depth_format(env_ptr_t env, list_ptr_t args) {
    return new_node_int(sapp_depth_format());
}

/* get default framebuffer sample count */
static node_idx_t native_sokol_sample_count(env_ptr_t env, list_ptr_t args) {
    return new_node_int(sapp_sample_count());
}

/* returns true when high_dpi was requested and actually running in a high-dpi scenario */
static node_idx_t native_sokol_high_dpi_q(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(sapp_high_dpi());
}

/* returns the dpi scaling factor (window pixels to framebuffer pixels) */
static node_idx_t native_sokol_dpi_scale(env_ptr_t env, list_ptr_t args) {
    return new_node_float(sapp_dpi_scale());
}

/* show or hide the mobile device onscreen keyboard */
static node_idx_t native_sokol_show_keyboard(env_ptr_t env, list_ptr_t args) {
    sapp_show_keyboard(get_node_bool(args->first_value()));
    return NIL_NODE;
}

/* return true if the mobile device onscreen keyboard is currently shown */
static node_idx_t native_sokol_keyboard_shown_q(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(sapp_keyboard_shown());
}

/* query fullscreen mode */
static node_idx_t native_sokol_fullscreen_q(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(sapp_is_fullscreen());
}

/* toggle fullscreen mode */
static node_idx_t native_sokol_toggle_fullscreen(env_ptr_t env, list_ptr_t args) {
    sapp_toggle_fullscreen();
    return NIL_NODE;
}

/* show or hide the mouse cursor */
static node_idx_t native_sokol_show_mouse(env_ptr_t env, list_ptr_t args) {
    sapp_show_mouse(get_node_bool(args->first_value()));
    return NIL_NODE;
}

/* show or hide the mouse cursor */
static node_idx_t native_sokol_mouse_shown_q(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(sapp_mouse_shown());
}

/* enable/disable mouse-pointer-lock mode */
static node_idx_t native_sokol_lock_mouse(env_ptr_t env, list_ptr_t args) {
    sapp_lock_mouse(get_node_bool(args->first_value()));
    return NIL_NODE;
}

/* return true if in mouse-pointer-lock mode (this may toggle a few frames later) */
static node_idx_t native_sokol_mouse_locked_q(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(sapp_mouse_locked());
}

/* set mouse cursor type */
static node_idx_t native_sokol_set_mouse_cursor(env_ptr_t env, list_ptr_t args) {
    sapp_set_mouse_cursor((sapp_mouse_cursor)get_node_int(args->first_value()));
    return NIL_NODE;
}

/* get current mouse cursor type */
static node_idx_t native_sokol_mouse_cursor(env_ptr_t env, list_ptr_t args) {
    return new_node_int(sapp_get_mouse_cursor());
}

/* initiate a "soft quit" (sends SAPP_EVENTTYPE_QUIT_REQUESTED) */
static node_idx_t native_sokol_request_quit(env_ptr_t env, list_ptr_t args) {
    sapp_request_quit();
    return NIL_NODE;
}

/* cancel a pending quit (when SAPP_EVENTTYPE_QUIT_REQUESTED has been received) */
static node_idx_t native_sokol_cancel_quit(env_ptr_t env, list_ptr_t args) {
    sapp_cancel_quit();
    return NIL_NODE;
}

/* initiate a "hard quit" (quit application without sending SAPP_EVENTTYPE_QUIT_REQUSTED) */
static node_idx_t native_sokol_quit(env_ptr_t env, list_ptr_t args) {
    sapp_quit();
    return NIL_NODE;
}

/* call from inside event callback to consume the current event (don't forward to platform) */
static node_idx_t native_sokol_consume_event(env_ptr_t env, list_ptr_t args) {
    sapp_consume_event();
    return NIL_NODE;
}

/* get the current frame counter (for comparison with sapp_event.frame_count) */
static node_idx_t native_sokol_frame_count(env_ptr_t env, list_ptr_t args) {
    return new_node_int(sapp_frame_count());
}

/* get an averaged/smoothed frame duration in seconds */
static node_idx_t native_sokol_frame_duration(env_ptr_t env, list_ptr_t args) {
    return new_node_float(sapp_frame_duration());
}

/* write string into clipboard */
static node_idx_t native_sokol_set_clipboard_string(env_ptr_t env, list_ptr_t args) {
    sapp_set_clipboard_string(get_node_string(args->first_value()).c_str());
    return NIL_NODE;
}

/* read string from clipboard (usually during SAPP_EVENTTYPE_CLIPBOARD_PASTED) */
static node_idx_t native_sokol_clipboard_string(env_ptr_t env, list_ptr_t args) {
    return new_node_string(sapp_get_clipboard_string());
}

/* set the window title (only on desktop platforms) */
static node_idx_t native_sokol_set_window_title(env_ptr_t env, list_ptr_t args) {
    sapp_set_window_title(get_node_string(args->first_value()).c_str());
    return NIL_NODE;
}

/* set the window icon (only on Windows and Linux) */
//SOKOL_APP_API_DECL void sapp_set_icon(const sapp_icon_desc* icon_desc);
// TODO: implement

/* gets the total number of dropped files (after an SAPP_EVENTTYPE_FILES_DROPPED event) */
static node_idx_t native_sokol_get_num_dropped_files(env_ptr_t env, list_ptr_t args) {
    return new_node_int(sapp_get_num_dropped_files());
}

/* gets the dropped file paths */
static node_idx_t native_sokol_get_dropped_file_path(env_ptr_t env, list_ptr_t args) {
    return new_node_string(sapp_get_dropped_file_path(get_node_int(args->first_value())));
}

/* render state functions */
static node_idx_t native_sgl_defaults(env_ptr_t env, list_ptr_t args) {
    sgl_defaults();
    return NIL_NODE;
}

static node_idx_t native_sgl_viewport(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float w = get_node_float(*it++);
    float h = get_node_float(*it++);
    bool origin_top_left = get_node_bool(*it++);
    sgl_viewport(x, y, w, h, origin_top_left);
    return NIL_NODE;
}

static node_idx_t native_sgl_scissor_rect(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float w = get_node_float(*it++);
    float h = get_node_float(*it++);
    bool origin_top_left = get_node_bool(*it++);
    sgl_scissor_rectf(x, y, w, h, origin_top_left);
    return NIL_NODE;
}

static node_idx_t native_sgl_enable_texture(env_ptr_t env, list_ptr_t args) {
    sgl_enable_texture();
    return NIL_NODE;
}

static node_idx_t native_sgl_disable_texture(env_ptr_t env, list_ptr_t args) {
    sgl_disable_texture();
    return NIL_NODE;
}

static node_idx_t native_sgl_texture(env_ptr_t env, list_ptr_t args) {
    sg_image img;
    img.id = get_node_int(args->first_value());
    sgl_texture(img);
    return NIL_NODE;
}

static node_idx_t native_sgl_layer(env_ptr_t env, list_ptr_t args) {
    sgl_layer(get_node_int(args->first_value()));
    return NIL_NODE;
}

/* pipeline stack functions */
static node_idx_t native_sgl_load_default_pipeline(env_ptr_t env, list_ptr_t args) {
    sgl_load_default_pipeline();
    return NIL_NODE;
}

static node_idx_t native_sgl_load_pipeline(env_ptr_t env, list_ptr_t args) {
    sgl_pipeline pip;
    pip.id = get_node_int(args->first_value());
    sgl_load_pipeline(pip);
    return NIL_NODE;
}

static node_idx_t native_sgl_push_pipeline(env_ptr_t env, list_ptr_t args) {
    sgl_push_pipeline();
    return NIL_NODE;
}

static node_idx_t native_sgl_pop_pipeline(env_ptr_t env, list_ptr_t args) {
    sgl_pop_pipeline();
    return NIL_NODE;
}

/* matrix stack functions */
static node_idx_t native_sgl_matrix_mode_modelview(env_ptr_t env, list_ptr_t args) {
    sgl_matrix_mode_modelview();
    return NIL_NODE;
}

static node_idx_t native_sgl_matrix_mode_projection(env_ptr_t env, list_ptr_t args) {
    sgl_matrix_mode_projection();
    return NIL_NODE;
}

static node_idx_t native_sgl_matrix_mode_texture(env_ptr_t env, list_ptr_t args) {
    sgl_matrix_mode_texture();
    return NIL_NODE;
}

static node_idx_t native_sgl_load_identity(env_ptr_t env, list_ptr_t args) {
    sgl_load_identity();
    return NIL_NODE;
}

static node_idx_t native_sgl_load_matrix(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    vector_ptr_t vec = get_node(*it++)->as_vector();
    float m[16];
    for (int i = 0; i < 16; i++) {
        m[i] = get_node_float(vec->nth(i));
    }
    sgl_load_matrix(m);
    return NIL_NODE;
}

static node_idx_t native_sgl_load_transpose_matrix(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    vector_ptr_t vec = get_node(*it++)->as_vector();
    float m[16];
    for (int i = 0; i < 16; i++) {
        m[i] = get_node_float(vec->nth(i));
    }
    sgl_load_transpose_matrix(m);
    return NIL_NODE;
}

static node_idx_t native_sgl_mult_matrix(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    vector_ptr_t vec = get_node(*it++)->as_vector();
    float m[16];
    for (int i = 0; i < 16; i++) {
        m[i] = get_node_float(vec->nth(i));
    }
    sgl_mult_matrix(m);
    return NIL_NODE;
}

static node_idx_t native_sgl_mult_transpose_matrix(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    vector_ptr_t vec = get_node(*it++)->as_vector();
    float m[16];
    for (int i = 0; i < 16; i++) {
        m[i] = get_node_float(vec->nth(i));
    }
    sgl_mult_transpose_matrix(m);
    return NIL_NODE;
}

static node_idx_t native_sgl_rotate(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float angle = get_node_float(*it++);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    sgl_rotate(angle, x, y, z);
    return NIL_NODE;
}

static node_idx_t native_sgl_scale(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    sgl_scale(x, y, z);
    return NIL_NODE;
}

static node_idx_t native_sgl_translate(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    sgl_translate(x, y, z);
    return NIL_NODE;
}

static node_idx_t native_sgl_frustum(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float l = get_node_float(*it++);
    float r = get_node_float(*it++);
    float b = get_node_float(*it++);
    float t = get_node_float(*it++);
    float n = get_node_float(*it++);
    float f = get_node_float(*it++);
    sgl_frustum(l, r, b, t, n, f);
    return NIL_NODE;
}

static node_idx_t native_sgl_ortho(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float l = get_node_float(*it++);
    float r = get_node_float(*it++);
    float b = get_node_float(*it++);
    float t = get_node_float(*it++);
    float n = get_node_float(*it++);
    float f = get_node_float(*it++);
    sgl_ortho(l, r, b, t, n, f);
    return NIL_NODE;
}

static node_idx_t native_sgl_perspective(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float fov_y = get_node_float(*it++);
    float aspect = get_node_float(*it++);
    float z_near = get_node_float(*it++);
    float z_far = get_node_float(*it++);
    sgl_perspective(fov_y, aspect, z_near, z_far);
    return NIL_NODE;
}

static node_idx_t native_sgl_lookat(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float eye_x = get_node_float(*it++);
    float eye_y = get_node_float(*it++);
    float eye_z = get_node_float(*it++);
    float center_x = get_node_float(*it++);
    float center_y = get_node_float(*it++);
    float center_z = get_node_float(*it++);
    float up_x = get_node_float(*it++);
    float up_y = get_node_float(*it++);
    float up_z = get_node_float(*it++);
    sgl_lookat(eye_x, eye_y, eye_z, center_x, center_y, center_z, up_x, up_y, up_z);
    return NIL_NODE;
}

static node_idx_t native_sgl_push_matrix(env_ptr_t env, list_ptr_t args) {
    sgl_push_matrix();
    return NIL_NODE;
}

static node_idx_t native_sgl_pop_matrix(env_ptr_t env, list_ptr_t args) {
    sgl_pop_matrix();
    return NIL_NODE;
}

/* these functions only set the internal 'current texcoord / color / point size' (valid inside or outside begin/end) */
static node_idx_t native_sgl_t2f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    sgl_t2f(u, v);
    return NIL_NODE;
}

static node_idx_t native_sgl_c3f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float r = get_node_float(*it++);
    float g = get_node_float(*it++);
    float b = get_node_float(*it++);
    sgl_c3f(r, g, b);
    return NIL_NODE;
}

static node_idx_t native_sgl_c4f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float r = get_node_float(*it++);
    float g = get_node_float(*it++);
    float b = get_node_float(*it++);
    float a = get_node_float(*it++);
    sgl_c4f(r, g, b, a);
    return NIL_NODE;
}

static node_idx_t native_sgl_c3b(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    uint8_t r = get_node_int(*it++);
    uint8_t g = get_node_int(*it++);
    uint8_t b = get_node_int(*it++);
    sgl_c3b(r, g, b);
    return NIL_NODE;
}

static node_idx_t native_sgl_c4b(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    uint8_t r = get_node_int(*it++);
    uint8_t g = get_node_int(*it++);
    uint8_t b = get_node_int(*it++);
    uint8_t a = get_node_int(*it++);
    sgl_c4b(r, g, b, a);
    return NIL_NODE;
}

static node_idx_t native_sgl_c1i(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    uint32_t rgba = get_node_int(*it++);
    sgl_c1i(rgba);
    return NIL_NODE;
}

static node_idx_t native_sgl_point_size(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float s = get_node_float(*it++);
    sgl_point_size(s);
    return NIL_NODE;
}

/* define primitives, each begin/end is one draw command */
static node_idx_t native_sgl_points(env_ptr_t env, list_ptr_t args) {
    sgl_begin_points();
    node_idx_t ret = eval_node_list(env, args);
    sgl_end();
    return ret;
}

static node_idx_t native_sgl_lines(env_ptr_t env, list_ptr_t args) {
    sgl_begin_lines();
    node_idx_t ret = eval_node_list(env, args);
    sgl_end();
    return ret;
}

static node_idx_t native_sgl_line_strip(env_ptr_t env, list_ptr_t args) {
    sgl_begin_line_strip();
    node_idx_t ret = eval_node_list(env, args);
    sgl_end();
    return ret;
}

static node_idx_t native_sgl_triangles(env_ptr_t env, list_ptr_t args) {
    sgl_begin_triangles();
    node_idx_t ret = eval_node_list(env, args);
    sgl_end();
    return ret;
}

static node_idx_t native_sgl_triangle_strip(env_ptr_t env, list_ptr_t args) {
    sgl_begin_triangle_strip();
    node_idx_t ret = eval_node_list(env, args);
    sgl_end();
    return ret;
}

static node_idx_t native_sgl_quads(env_ptr_t env, list_ptr_t args) {
    sgl_begin_quads();
    node_idx_t ret = eval_node_list(env, args);
    sgl_end();
    return ret;
}

static node_idx_t native_sgl_v2f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    sgl_v2f(x, y);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    sgl_v3f(x, y, z);
    return NIL_NODE;
}

static node_idx_t native_sgl_v2f_t2f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    sgl_v2f_t2f(x, y, u, v);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f_t2f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    sgl_v3f_t2f(x, y, z, u, v);
    return NIL_NODE;
}

static node_idx_t native_sgl_v2f_c3f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float r = get_node_float(*it++);
    float g = get_node_float(*it++);
    float b = get_node_float(*it++);
    sgl_v2f_c3f(x, y, r, g, b);
    return NIL_NODE;
}

static node_idx_t native_sgl_v2f_c3b(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    uint8_t r = get_node_int(*it++);
    uint8_t g = get_node_int(*it++);
    uint8_t b = get_node_int(*it++);
    sgl_v2f_c3b(x, y, r, g, b);
    return NIL_NODE;
}

static node_idx_t native_sgl_v2f_c4f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float r = get_node_float(*it++);
    float g = get_node_float(*it++);
    float b = get_node_float(*it++);
    float a = get_node_float(*it++);
    sgl_v2f_c4f(x, y, r, g, b, a);
    return NIL_NODE;
}

static node_idx_t native_sgl_v2f_c4b(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    uint8_t r = get_node_int(*it++);
    uint8_t g = get_node_int(*it++);
    uint8_t b = get_node_int(*it++);
    uint8_t a = get_node_int(*it++);
    sgl_v2f_c4b(x, y, r, g, b, a);
    return NIL_NODE;
}

static node_idx_t native_sgl_v2f_c1i(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    uint32_t rgba = get_node_int(*it++);
    sgl_v2f_c1i(x, y, rgba);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f_c3f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    float r = get_node_float(*it++);
    float g = get_node_float(*it++);
    float b = get_node_float(*it++);
    sgl_v3f_c3f(x, y, z, r, g, b);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f_c3b(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    uint8_t r = get_node_int(*it++);
    uint8_t g = get_node_int(*it++);
    uint8_t b = get_node_int(*it++);
    sgl_v3f_c3b(x, y, z, r, g, b);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f_c4f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    float r = get_node_float(*it++);
    float g = get_node_float(*it++);
    float b = get_node_float(*it++);
    float a = get_node_float(*it++);
    sgl_v3f_c4f(x, y, z, r, g, b, a);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f_c4b(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    uint8_t r = get_node_int(*it++);
    uint8_t g = get_node_int(*it++);
    uint8_t b = get_node_int(*it++);
    uint8_t a = get_node_int(*it++);
    sgl_v3f_c4b(x, y, z, r, g, b, a);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f_c1i(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    uint32_t rgba = get_node_int(*it++);
    sgl_v3f_c1i(x, y, z, rgba);
    return NIL_NODE;
}

static node_idx_t native_sgl_v2f_t2f_c3f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    float r = get_node_float(*it++);
    float g = get_node_float(*it++);
    float b = get_node_float(*it++);
    sgl_v2f_t2f_c3f(x, y, u, v, r, g, b);
    return NIL_NODE;
}

static node_idx_t native_sgl_v2f_t2f_c3b(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    uint8_t r = get_node_int(*it++);
    uint8_t g = get_node_int(*it++);
    uint8_t b = get_node_int(*it++);
    sgl_v2f_t2f_c3b(x, y, u, v, r, g, b);
    return NIL_NODE;
}

static node_idx_t native_sgl_v2f_t2f_c4f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    float r = get_node_float(*it++);
    float g = get_node_float(*it++);
    float b = get_node_float(*it++);
    float a = get_node_float(*it++);
    sgl_v2f_t2f_c4f(x, y, u, v, r, g, b, a);
    return NIL_NODE;
}

static node_idx_t native_sgl_v2f_t2f_c4b(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    uint8_t r = get_node_int(*it++);
    uint8_t g = get_node_int(*it++);
    uint8_t b = get_node_int(*it++);
    uint8_t a = get_node_int(*it++);
    sgl_v2f_t2f_c4b(x, y, u, v, r, g, b, a);
    return NIL_NODE;
}

static node_idx_t native_sgl_v2f_t2f_c1i(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    uint32_t rgba = get_node_int(*it++);
    sgl_v2f_t2f_c1i(x, y, u, v, rgba);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f_t2f_c3f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    float r = get_node_float(*it++);
    float g = get_node_float(*it++);
    float b = get_node_float(*it++);
    sgl_v3f_t2f_c3f(x, y, z, u, v, r, g, b);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f_t2f_c3b(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    uint8_t r = get_node_int(*it++);
    uint8_t g = get_node_int(*it++);
    uint8_t b = get_node_int(*it++);
    sgl_v3f_t2f_c3b(x, y, z, u, v, r, g, b);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f_t2f_c4f(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    float r = get_node_float(*it++);
    float g = get_node_float(*it++);
    float b = get_node_float(*it++);
    float a = get_node_float(*it++);
    sgl_v3f_t2f_c4f(x, y, z, u, v, r, g, b, a);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f_t2f_c4b(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    uint8_t r = get_node_int(*it++);
    uint8_t g = get_node_int(*it++);
    uint8_t b = get_node_int(*it++);
    uint8_t a = get_node_int(*it++);
    sgl_v3f_t2f_c4b(x, y, z, u, v, r, g, b, a);
    return NIL_NODE;
}

static node_idx_t native_sgl_v3f_t2f_c1i(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    float x = get_node_float(*it++);
    float y = get_node_float(*it++);
    float z = get_node_float(*it++);
    float u = get_node_float(*it++);
    float v = get_node_float(*it++);
    uint32_t rgba = get_node_int(*it++);
    sgl_v3f_t2f_c1i(x, y, z, u, v, rgba);
    return NIL_NODE;
}

static node_idx_t native_sgl_end(env_ptr_t env, list_ptr_t args) {
    sgl_end();
    return NIL_NODE;
}

static node_idx_t native_sg_image(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    sg_image_desc desc = {0};
    desc.width = get_node_int(*it++);
    desc.height = get_node_int(*it++);
    desc.pixel_format = (sg_pixel_format)get_node_int(*it++);
    desc.min_filter = (sg_filter)get_node_int(*it++);
    desc.mag_filter = (sg_filter)get_node_int(*it++);
    desc.wrap_u = (sg_wrap)get_node_int(*it++);
    desc.wrap_v = (sg_wrap)get_node_int(*it++);
    desc.num_mipmaps = get_node_int(*it++);
    desc.usage = (sg_usage)get_node_int(*it++);
    desc.label = get_node_string(*it++).c_str();
    sg_image img = sg_make_image(&desc);
    return new_node_int(img.id);
}

static node_idx_t native_sg_canvas_image(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    if(get_node_type(*it) != NODE_CANVAS) {
        return NIL_NODE;
    }
    jo_clojure_canvas_ptr_t canvas = get_node(*it++)->t_object.cast<jo_clojure_canvas_t>();
    sg_image_desc desc = {0};
    desc.width = canvas->width;
    desc.height = canvas->height;
    desc.num_mipmaps = 1;
    int in_ch = canvas->channels;
    int out_ch;
    if(in_ch == 1) {
        desc.pixel_format = SG_PIXELFORMAT_R8;
        out_ch = 1;
    } else if(in_ch == 2) {
        desc.pixel_format = SG_PIXELFORMAT_RG8;
        out_ch = 2;
    } else if(in_ch == 3) {
        desc.pixel_format = SG_PIXELFORMAT_RGBA8;
        out_ch = 4;
    } else if(in_ch == 4) {
        desc.pixel_format = SG_PIXELFORMAT_RGBA8;
        out_ch = 4;
    } else {
        return NIL_NODE;
    }
    int data_size = desc.width * desc.height * out_ch;
    unsigned char *data = (unsigned char *)malloc(data_size);
    for(int y = 0; y < desc.height; y+=8) {
        for(int x = 0; x < desc.width*in_ch; x+=8) {
            double block[8*8];
            canvas->pixels->get_block(x, y, block);

            for(int i = 0; i < 8; ++i) {
                for(int j = 0; j < 8; ++j) {
                    int xx = x+j;
                    int yy = y+i;
                    if(xx >= desc.width*in_ch || yy >= desc.height) continue;
                    int c = xx % in_ch;
                    data[yy*desc.width*out_ch+xx/in_ch*out_ch+c] = block[i*8+j];
                }
            }
        }
    }
    if(out_ch > in_ch) {
        for(int y = 0; y < desc.height; ++y) {
            for(int x = 0; x < desc.width; ++x) {
                for(int c = in_ch; c < out_ch; ++c) {
                    data[y*desc.width*out_ch+x*out_ch+c] = 255;
                }
            }
        }
    }
    desc.usage = SG_USAGE_DYNAMIC;
    sg_image img = sg_make_image(&desc);
    sg_image_data idata = {0};
    idata.subimage[0][0].ptr = data;
    idata.subimage[0][0].size = data_size;
    sg_update_image(img, &idata);
    free(data);
    return new_node_int(img.id);
}

static node_idx_t native_sg_destroy_image(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    sg_image img = {(unsigned)get_node_int(*it++)};
    sg_destroy_image(img);
    return NIL_NODE;
}

static node_idx_t native_sg_update_canvas_image(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    sg_image img = {(unsigned)get_node_int(*it++)};
    if(get_node_type(*it) != NODE_CANVAS) {
        return NIL_NODE;
    }
    jo_clojure_canvas_ptr_t canvas = get_node(*it++)->t_object.cast<jo_clojure_canvas_t>();
    int height = canvas->height;
    int width = canvas->width;
    int in_ch = canvas->channels;
    int out_ch;
    if(in_ch == 1) out_ch = 1;
    else if(in_ch == 2) out_ch = 2;
    else if(in_ch == 3) out_ch = 4;
    else if(in_ch == 4) out_ch = 4;
    else return NIL_NODE;
    int data_size = width * height * out_ch;
    unsigned char *data = (unsigned char *)malloc(data_size);
    for(int y = 0; y < height; y+=8) {
        for(int x = 0; x < width*in_ch; x+=8) {
            double block[8*8];
            canvas->pixels->get_block(x, y, block);

            for(int i = 0; i < 8; ++i) {
                for(int j = 0; j < 8; ++j) {
                    int xx = x+j;
                    int yy = y+i;
                    if(xx >= width*in_ch || yy >= height) continue;
                    int c = xx % in_ch;
                    data[yy*width*out_ch+xx/in_ch*out_ch+c] = block[i*8+j];
                }
            }
        }
    }
    if(out_ch > in_ch) {
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                for(int c = in_ch; c < out_ch; ++c) {
                    data[y*width*out_ch+x*out_ch+c] = 255;
                }
            }
        }
    }
    sg_image_data idata = {0};
    idata.subimage[0][0].ptr = data;
    idata.subimage[0][0].size = data_size;
    sg_update_image(img, &idata);
    free(data);
    return NIL_NODE;
}

static node_idx_t native_sg_file_image(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_string filename = get_node_string(*it++);
    int flags = get_node_int(*it++);
    sg_image img = sg_alloc_image();
    sg_image_desc desc = {0};
    desc.label = filename.c_str();
    desc.usage = SG_USAGE_IMMUTABLE;
    desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    desc.min_filter = SG_FILTER_LINEAR;
    desc.mag_filter = SG_FILTER_LINEAR;
    desc.wrap_u = SG_WRAP_REPEAT;
    desc.wrap_v = SG_WRAP_REPEAT;
    desc.num_mipmaps = 1;
    int width, height, channels;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    if(!data) {
        return NIL_NODE;
    }
    desc.width = width;
    desc.height = height;
    desc.data.subimage[0][0].ptr = data;
    desc.data.subimage[0][0].size = width * height * 4;
    sg_init_image(img, &desc);
    stbi_image_free(data);
    return new_node_int(img.id);
}

#include "jo_clojure_imgui.h"

void jo_clojure_sokol_init(env_ptr_t env) {
	env->set("sokol/run", new_node_native_function("sokol/run", &native_sokol_run, false, NODE_FLAG_PRERESOLVE));

    env->set("sokol/valid?", new_node_native_function("sokol/valid?", &native_sokol_valid_q, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/width", new_node_native_function("sokol/width", &native_sokol_width, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/height", new_node_native_function("sokol/height", &native_sokol_height, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/dpi-scale", new_node_native_function("sokol/dpi-scale", &native_sokol_dpi_scale, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/high-dpi?", new_node_native_function("sokol/high-dpi?", &native_sokol_high_dpi_q, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/sample-count", new_node_native_function("sokol/sample-count", &native_sokol_sample_count, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/color-format", new_node_native_function("sokol/color-format", &native_sokol_color_format, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/depth-format", new_node_native_function("sokol/depth-format", &native_sokol_depth_format, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/show-keyboard", new_node_native_function("sokol/show-keyboard", &native_sokol_show_keyboard, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/keyboard-shown?", new_node_native_function("sokol/keyboard-shown?", &native_sokol_keyboard_shown_q, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/fullscreen?", new_node_native_function("sokol/fullscreen?", &native_sokol_fullscreen_q, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/toggle-fullscreen", new_node_native_function("sokol/toggle-fullscreen", &native_sokol_toggle_fullscreen, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/show-mouse", new_node_native_function("sokol/show-mouse", &native_sokol_show_mouse, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/mouse-shown?", new_node_native_function("sokol/mouse-shown?", &native_sokol_mouse_shown_q, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/lock-mouse", new_node_native_function("sokol/lock-mouse", &native_sokol_lock_mouse, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/mouse-locked?", new_node_native_function("sokol/mouse-locked?", &native_sokol_mouse_locked_q, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/mouse-cursor", new_node_native_function("sokol/mouse-cursor", &native_sokol_mouse_cursor, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/set-mouse-cursor", new_node_native_function("sokol/set-mouse-cursor", &native_sokol_set_mouse_cursor, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/request-quit", new_node_native_function("sokol/request-quit", &native_sokol_request_quit, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/cancel-quit", new_node_native_function("sokol/cancel-quit", &native_sokol_cancel_quit, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/quit", new_node_native_function("sokol/quit", &native_sokol_quit, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/consume-event", new_node_native_function("sokol/consume-event", &native_sokol_consume_event, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/frame-count", new_node_native_function("sokol/frame-count", &native_sokol_frame_count, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/frame-duration", new_node_native_function("sokol/frame-duration", &native_sokol_frame_duration, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/set-clipboard-string", new_node_native_function("sokol/set-clipboard-string", &native_sokol_set_clipboard_string, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/clipboard-string", new_node_native_function("sokol/clipboard-string", &native_sokol_clipboard_string, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/set-window-title", new_node_native_function("sokol/set-window-title", &native_sokol_set_window_title, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/get-num-dropped-files", new_node_native_function("sokol/get-num-dropped-files", &native_sokol_get_num_dropped_files, false, NODE_FLAG_PRERESOLVE));
    env->set("sokol/get-dropped-file-path", new_node_native_function("sokol/get-dropped-file-path", &native_sokol_get_dropped_file_path, false, NODE_FLAG_PRERESOLVE));
    
    // enums
    env->set("sokol/SAPP_MODIFIER_SHIFT", new_node_int(SAPP_MODIFIER_SHIFT));
    env->set("sokol/SAPP_MODIFIER_CTRL", new_node_int(SAPP_MODIFIER_CTRL));
    env->set("sokol/SAPP_MODIFIER_ALT", new_node_int(SAPP_MODIFIER_ALT));
    env->set("sokol/SAPP_MODIFIER_SUPER", new_node_int(SAPP_MODIFIER_SUPER));
    env->set("sokol/SAPP_MODIFIER_LMB", new_node_int(SAPP_MODIFIER_LMB));
    env->set("sokol/SAPP_MODIFIER_RMB", new_node_int(SAPP_MODIFIER_RMB));
    env->set("sokol/SAPP_MODIFIER_MMB", new_node_int(SAPP_MODIFIER_MMB));
    
    // sokol_gfx.h
    env->set("sg/image", new_node_native_function("sg/image", &native_sg_image, false, NODE_FLAG_PRERESOLVE));
    env->set("sg/file-image", new_node_native_function("sg/file-image", &native_sg_file_image, false, NODE_FLAG_PRERESOLVE));
    env->set("sg/canvas-image", new_node_native_function("sg/canvas-image", &native_sg_canvas_image, false, NODE_FLAG_PRERESOLVE));
    env->set("sg/update-canvas-image", new_node_native_function("sg/update-canvas-image", &native_sg_update_canvas_image, false, NODE_FLAG_PRERESOLVE));
    env->set("sg/destroy-image", new_node_native_function("sg/destroy-image", &native_sg_destroy_image, false, NODE_FLAG_PRERESOLVE));

    /* render state functions */
    env->set("sgl/defaults", new_node_native_function("sgl/defaults", &native_sgl_defaults, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/viewport", new_node_native_function("sgl/viewport", &native_sgl_viewport, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/scissor-rect", new_node_native_function("sgl/scissor-rect", &native_sgl_scissor_rect, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/enable-texture", new_node_native_function("sgl/enable-texture", &native_sgl_enable_texture, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/disable-texture", new_node_native_function("sgl/disable-texture", &native_sgl_disable_texture, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/texture", new_node_native_function("sgl/texture", &native_sgl_texture, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/layer", new_node_native_function("sgl/layer", &native_sgl_layer, false, NODE_FLAG_PRERESOLVE));

    /* pipeline stack functions */
    env->set("sgl/load-default-pipeline", new_node_native_function("sgl/load-default-pipeline", &native_sgl_load_default_pipeline, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/load-pipeline", new_node_native_function("sgl/load-pipeline", &native_sgl_load_pipeline, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/push-pipeline", new_node_native_function("sgl/push-pipeline", &native_sgl_push_pipeline, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/pop-pipeline", new_node_native_function("sgl/pop-pipeline", &native_sgl_pop_pipeline, false, NODE_FLAG_PRERESOLVE));

    /* matrix stack functions */
    env->set("sgl/matrix-mode-modelview", new_node_native_function("sgl/matrix-mode-modelview", &native_sgl_matrix_mode_modelview, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/matrix-mode-projection", new_node_native_function("sgl/matrix-mode-projection", &native_sgl_matrix_mode_projection, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/matrix-mode-texture", new_node_native_function("sgl/matrix-mode-texture", &native_sgl_matrix_mode_texture, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/load-identity", new_node_native_function("sgl/load-identity", &native_sgl_load_identity, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/load-matrix", new_node_native_function("sgl/load-matrix", &native_sgl_load_matrix, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/load-transpose-matrix", new_node_native_function("sgl/load-transpose-matrix", &native_sgl_load_transpose_matrix, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/mult-matrix", new_node_native_function("sgl/mult-matrix", &native_sgl_mult_matrix, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/mult-transpose-matrix", new_node_native_function("sgl/mult-transpose-matrix", &native_sgl_mult_transpose_matrix, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/rotate", new_node_native_function("sgl/rotate", &native_sgl_rotate, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/scale", new_node_native_function("sgl/scale", &native_sgl_scale, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/translate", new_node_native_function("sgl/translate", &native_sgl_translate, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/frustum", new_node_native_function("sgl/frustum", &native_sgl_frustum, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/ortho", new_node_native_function("sgl/ortho", &native_sgl_ortho, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/perspective", new_node_native_function("sgl/perspective", &native_sgl_perspective, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/lookat", new_node_native_function("sgl/lookat", &native_sgl_lookat, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/push-matrix", new_node_native_function("sgl/push-matrix", &native_sgl_push_matrix, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/pop-matrix", new_node_native_function("sgl/pop-matrix", &native_sgl_pop_matrix, false, NODE_FLAG_PRERESOLVE));

    /* these functions only set the internal 'current texcoord / color / point size' (valid inside or outside begin/end) */
    env->set("sgl/t2f", new_node_native_function("sgl/t2f", &native_sgl_t2f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/c3f", new_node_native_function("sgl/c3f", &native_sgl_c3f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/c4f", new_node_native_function("sgl/c4f", &native_sgl_c4f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/c3b", new_node_native_function("sgl/c3b", &native_sgl_c3b, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/c4b", new_node_native_function("sgl/c4b", &native_sgl_c4b, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/c1i", new_node_native_function("sgl/c1i", &native_sgl_c1i, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/point-size", new_node_native_function("sgl/point-size", &native_sgl_point_size, false, NODE_FLAG_PRERESOLVE));

    /* define primitives, each begin/end is one draw command */
    env->set("sgl/points", new_node_native_function("sgl/points", &native_sgl_points, true, NODE_FLAG_PRERESOLVE));
    env->set("sgl/lines", new_node_native_function("sgl/lines", &native_sgl_lines, true, NODE_FLAG_PRERESOLVE));
    env->set("sgl/line-strip", new_node_native_function("sgl/line-strip", &native_sgl_line_strip, true, NODE_FLAG_PRERESOLVE));
    env->set("sgl/triangles", new_node_native_function("sgl/triangles", &native_sgl_triangles, true, NODE_FLAG_PRERESOLVE));
    env->set("sgl/triangle-strip", new_node_native_function("sgl/triangle-strip", &native_sgl_triangle_strip, true, NODE_FLAG_PRERESOLVE));
    env->set("sgl/quads", new_node_native_function("sgl/quads", &native_sgl_quads, true, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f", new_node_native_function("sgl/v2f", &native_sgl_v2f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f", new_node_native_function("sgl/v3f", &native_sgl_v3f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f-t2f", new_node_native_function("sgl/v2f-t2f", &native_sgl_v2f_t2f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f-t2f", new_node_native_function("sgl/v3f-t2f", &native_sgl_v3f_t2f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f-c3f", new_node_native_function("sgl/v2f-c3f", &native_sgl_v2f_c3f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f-c3b", new_node_native_function("sgl/v2f-c3b", &native_sgl_v2f_c3b, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f-c4f", new_node_native_function("sgl/v2f-c4f", &native_sgl_v2f_c4f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f-c4b", new_node_native_function("sgl/v2f-c4b", &native_sgl_v2f_c4b, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f_c1i", new_node_native_function("sgl/v2f_c1i", &native_sgl_v2f_c1i, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f-c3f", new_node_native_function("sgl/v3f-c3f", &native_sgl_v3f_c3f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f-c3b", new_node_native_function("sgl/v3f-c3b", &native_sgl_v3f_c3b, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f-c4f", new_node_native_function("sgl/v3f-c4f", &native_sgl_v3f_c4f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f-c4b", new_node_native_function("sgl/v3f-c4b", &native_sgl_v3f_c4b, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f-c1i", new_node_native_function("sgl/v3f-c1i", &native_sgl_v3f_c1i, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f-t2f-c3f", new_node_native_function("sgl/v2f-t2f-c3f", &native_sgl_v2f_t2f_c3f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f-t2f-c3b", new_node_native_function("sgl/v2f-t2f-c3b", &native_sgl_v2f_t2f_c3b, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f-t2f-c4f", new_node_native_function("sgl/v2f-t2f-c4f", &native_sgl_v2f_t2f_c4f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f-t2f-c4b", new_node_native_function("sgl/v2f-t2f-c4b", &native_sgl_v2f_t2f_c4b, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v2f-t2f-c1i", new_node_native_function("sgl/v2f-t2f-c1i", &native_sgl_v2f_t2f_c1i, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f-t2f-c3f", new_node_native_function("sgl/v3f-t2f-c3f", &native_sgl_v3f_t2f_c3f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f-t2f-c3b", new_node_native_function("sgl/v3f-t2f-c3b", &native_sgl_v3f_t2f_c3b, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f-t2f-c4f", new_node_native_function("sgl/v3f-t2f-c4f", &native_sgl_v3f_t2f_c4f, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f-t2f-c4b", new_node_native_function("sgl/v3f-t2f-c4b", &native_sgl_v3f_t2f_c4b, false, NODE_FLAG_PRERESOLVE));
    env->set("sgl/v3f-t2f-c1i", new_node_native_function("sgl/v3f-t2f-c1i", &native_sgl_v3f_t2f_c1i, false, NODE_FLAG_PRERESOLVE));

    jo_clojure_imgui_init(env);
}



