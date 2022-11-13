#pragma once

#include "jo_gif.cpp"

// simple wrapper so we can blop it into the t_object generic container
struct jo_basic_gif_t : jo_object {
    jo_gif_t g;
};

typedef jo_alloc_t<jo_basic_gif_t> jo_basic_gif_alloc_t;
jo_basic_gif_alloc_t jo_basic_gif_alloc;
typedef jo_shared_ptr_t<jo_basic_gif_t> jo_basic_gif_ptr_t;
template<typename...A>
jo_basic_gif_ptr_t new_gif(A...args) { return jo_basic_gif_ptr_t(jo_basic_gif_alloc.emplace(args...)); }

//typedef jo_shared_ptr<jo_basic_gif_t> jo_basic_gif_ptr_t;
//static jo_basic_gif_ptr_t new_gif() { return jo_basic_gif_ptr_t(new jo_basic_gif_t()); }
static node_idx_t new_node_gif(jo_basic_gif_ptr_t nodes, int flags=0) { return new_node_object(NODE_GIF, nodes.cast<jo_object>(), flags); }


static node_idx_t native_gif_open(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_string filename = get_node_string(*it++);
    int width = get_node_int(*it++);
    int height = get_node_int(*it++);
    int repeat = get_node_int(*it++);
    int pal_size = get_node_int(*it++);

    jo_gif_t g = jo_gif_start(filename.c_str(), width, height, repeat, pal_size);
    if(!g.fp) {
        warnf("failed to open gif file %s\n", filename.c_str());
        return NIL_NODE;
    }

    jo_basic_gif_ptr_t gif = new_gif();
    gif->g = g;
    return new_node_gif(gif);
}

static node_idx_t native_gif_frame(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_basic_gif_ptr_t gif = get_node(*it++)->t_object.cast<jo_basic_gif_t>();
    matrix_ptr_t canvas = get_node(*it++)->t_object.cast<matrix_t>();
    int delay_csec = get_node_int(*it++);
    bool local_palette = get_node_bool(*it++);

    if(!gif.ptr || !canvas.ptr) {
        warnf("invalid gif or not a matrix\n");
        return NIL_NODE;
    }

    if(!gif->g.fp) {
        warnf("gif file is closed\n");
        return NIL_NODE;
    }

    int width = gif->g.width;
    int height = gif->g.height;

    unsigned *rgba = new unsigned[width * height];
    // TODO: could be MUCH faster
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            unsigned color = (unsigned)get_node_int(canvas->get(x, y));
            rgba[y * width + x] = color;
        }
    }

    jo_gif_frame(&gif->g, (unsigned char*)rgba, delay_csec, local_palette);
    delete [] rgba;
    return NIL_NODE;
}

static node_idx_t native_gif_close(env_ptr_t env, list_ptr_t args) {
    jo_basic_gif_ptr_t gif = get_node(args->first_value())->t_object.cast<jo_basic_gif_t>();
    jo_gif_end(&gif->g);
    return NIL_NODE;
}


void jo_basic_gif_init(env_ptr_t env) {
	env->set("gif/open", new_node_native_function("gif/open", &native_gif_open, false, NODE_FLAG_PRERESOLVE));
	env->set("gif/frame", new_node_native_function("gif/frame", &native_gif_frame, false, NODE_FLAG_PRERESOLVE));
	env->set("gif/close", new_node_native_function("gif/close", &native_gif_close, false, NODE_FLAG_PRERESOLVE));
}
