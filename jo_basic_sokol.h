#pragma once

#define SOKOL_IMPL
#define SOKOL_NO_ENTRY

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
        node_idx_t init_cb_idx = get_map_idx(desc_map, "init_cb", NIL_NODE);
        if(init_cb_idx != NIL_NODE) {
            eval_node(env, new_node_list(list_va(init_cb_idx)));
        }
    };
    d.frame_cb = [=]() {
        node_idx_t frame_cb_idx = get_map_idx(desc_map, "frame_cb", NIL_NODE);
        if(frame_cb_idx != NIL_NODE) {
            eval_node(env, new_node_list(list_va(frame_cb_idx)));
        }
    };
    d.cleanup_cb = [=]() {
        node_idx_t cleanup_cb_idx = get_map_idx(desc_map, "cleanup_cb", NIL_NODE);
        if(cleanup_cb_idx != NIL_NODE) {
            eval_node(env, new_node_list(list_va(cleanup_cb_idx)));
        }
    };
    d.event_cb = [=](const sapp_event*ev) {
        node_idx_t event_cb_idx = get_map_idx(desc_map, "event_cb", NIL_NODE);
        if(event_cb_idx != NIL_NODE) {
            hash_map_ptr_t event_map = new_hash_map();
            event_map->assoc_inplace(new_node_string("frame_count"), new_node_int(ev->frame_count), node_sym_eq);
            event_map->assoc_inplace(new_node_string("type"), new_node_int(ev->type), node_sym_eq);
            event_map->assoc_inplace(new_node_string("key_code"), new_node_int(ev->key_code), node_sym_eq);
            event_map->assoc_inplace(new_node_string("char_code"), new_node_int(ev->char_code), node_sym_eq);
            event_map->assoc_inplace(new_node_string("key_repeat"), new_node_int(ev->key_repeat), node_sym_eq);
            event_map->assoc_inplace(new_node_string("modifiers"), new_node_int(ev->modifiers), node_sym_eq);
            event_map->assoc_inplace(new_node_string("mouse_button"), new_node_int(ev->mouse_button), node_sym_eq);
            event_map->assoc_inplace(new_node_string("mouse_x"), new_node_float(ev->mouse_x), node_sym_eq);
            event_map->assoc_inplace(new_node_string("mouse_y"), new_node_float(ev->mouse_y), node_sym_eq);
            event_map->assoc_inplace(new_node_string("mouse_dx"), new_node_float(ev->mouse_dx), node_sym_eq);
            event_map->assoc_inplace(new_node_string("mouse_dy"), new_node_float(ev->mouse_dy), node_sym_eq);
            event_map->assoc_inplace(new_node_string("scroll_x"), new_node_float(ev->scroll_x), node_sym_eq);
            event_map->assoc_inplace(new_node_string("scroll_y"), new_node_float(ev->scroll_y), node_sym_eq);
            event_map->assoc_inplace(new_node_string("num_touches"), new_node_int(ev->num_touches), node_sym_eq);
            if(ev->num_touches) {
                list_ptr_t touches = new_list();
                for(int i = 0; i < ev->num_touches; ++i) {
                    hash_map_ptr_t touch_map = new_hash_map();
                    touch_map->assoc_inplace(new_node_string("identifier"), new_node_int(ev->touches[i].identifier), node_sym_eq);
                    touch_map->assoc_inplace(new_node_string("pos_x"), new_node_float(ev->touches[i].pos_x), node_sym_eq);
                    touch_map->assoc_inplace(new_node_string("pos_y"), new_node_float(ev->touches[i].pos_y), node_sym_eq);
                    touch_map->assoc_inplace(new_node_string("android_tooltype"), new_node_int(ev->touches[i].android_tooltype), node_sym_eq);
                    touch_map->assoc_inplace(new_node_string("changed"), new_node_int(ev->touches[i].changed), node_sym_eq);
                    touches->push_back_inplace(new_node_hash_map(touch_map));
                }
                event_map->assoc_inplace(new_node_string("touches"), new_node_list(touches), node_sym_eq);
            }
            event_map->assoc_inplace(new_node_string("window_width"), new_node_int(ev->window_width), node_sym_eq);
            event_map->assoc_inplace(new_node_string("window_height"), new_node_int(ev->window_height), node_sym_eq);
            event_map->assoc_inplace(new_node_string("framebuffer_width"), new_node_int(ev->framebuffer_width), node_sym_eq);
            event_map->assoc_inplace(new_node_string("framebuffer_height"), new_node_int(ev->framebuffer_height), node_sym_eq);
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

void jo_basic_sokol_init(env_ptr_t env) {
	env->set("sokol/run", new_node_native_function("sokol/run", &native_sokol_run, false, NODE_FLAG_PRERESOLVE));
}



