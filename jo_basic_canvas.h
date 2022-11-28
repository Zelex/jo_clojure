#pragma once

// TODO:
// - draw lines, circles, etc...
// - draw text

typedef jo_persistent_matrix<double> pixels_t;
template<> pixels_t::matrix_node_alloc_t pixels_t::alloc_node = pixels_t::matrix_node_alloc_t();
template<> pixels_t::matrix_alloc_t pixels_t::alloc = pixels_t::matrix_alloc_t();
typedef jo_shared_ptr_t<pixels_t> pixels_ptr_t;
template<typename...A> jo_shared_ptr_t<pixels_t> new_pixels(A... args) { return jo_shared_ptr_t<pixels_t>(pixels_t::alloc.emplace(args...)); }

// simple wrapper so we can blop it into the t_object generic container
struct jo_basic_canvas_t : jo_object {
    int width;
    int height;
    int channels;
    pixels_ptr_t pixels;

    jo_basic_canvas_t(int width, int height, int channels) {
        width = width;
        height = height;
        channels = channels;
        pixels = new_pixels(width*channels, height);
    }
};

typedef jo_alloc_t<jo_basic_canvas_t> jo_basic_canvas_alloc_t;
jo_basic_canvas_alloc_t jo_basic_canvas_alloc;
typedef jo_shared_ptr_t<jo_basic_canvas_t> jo_basic_canvas_ptr_t;
template<typename...A>
jo_basic_canvas_ptr_t new_canvas(A...args) { return jo_basic_canvas_ptr_t(jo_basic_canvas_alloc.emplace(args...)); }

//typedef jo_shared_ptr<jo_basic_canvas_t> jo_basic_canvas_ptr_t;
//static jo_basic_canvas_ptr_t new_canvas() { return jo_basic_canvas_ptr_t(new jo_basic_canvas_t()); }
static node_idx_t new_node_canvas(jo_basic_canvas_ptr_t nodes, int flags=0) { return new_node_object(NODE_CANVAS, nodes.cast<jo_object>(), flags); }

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "jo_resize.h"

static node_idx_t native_canvas(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
    int width = get_node_int(*it++);
    int height = get_node_int(*it++);
    int channels = get_node_int(*it++);

    jo_basic_canvas_ptr_t canvas = new_canvas(width, height, channels);
    return new_node_canvas(canvas);
}

static node_idx_t native_canvas_load_file(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
    jo_string filename = get_node_string(*it++);

    int width, height, channels;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if(!data) {
        warnf("failed to load image file: %s", filename.c_str());
        return NIL_NODE;
    }

    jo_basic_canvas_ptr_t canvas = new_canvas(width, height, channels);
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            for(int c = 0; c < channels; c++) {
                canvas->pixels->set(x*channels+c, y, data[(y*width+x)*channels+c]);
            }
        }
    }
    stbi_image_free(data);
    return new_node_canvas(canvas);
}

static node_idx_t native_canvas_save_file(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
    jo_string filename = get_node_string(*it++);
    jo_basic_canvas_ptr_t canvas = get_node(*it++)->t_object.cast<jo_basic_canvas_t>();

    int width = canvas->width;
    int height = canvas->height;
    int channels = canvas->channels;
    unsigned char *data = (unsigned char*)malloc(width*height*channels);
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            for(int c = 0; c < channels; c++) {
                data[(y*width+x)*channels+c] = canvas->pixels->get(x*channels+c, y);
            }
        }
    }
    stbi_write_png(filename.c_str(), width, height, channels, data, width*channels);
    free(data);
    return NIL_NODE;
}

static node_idx_t native_canvas_resize(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
    jo_basic_canvas_ptr_t canvas = get_node(*it++)->t_object.cast<jo_basic_canvas_t>();
    int new_width = get_node_int(*it++);
    int new_height = get_node_int(*it++);

    int width = canvas->width;
    int height = canvas->height;
    int channels = canvas->channels;
    unsigned char *data = (unsigned char*)malloc(width*height*channels);
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            for(int c = 0; c < channels; c++) {
                data[(y*width+x)*channels+c] = canvas->pixels->get(x*channels+c, y);
            }
        }
    }

    unsigned char *new_data = (unsigned char*)malloc(new_width*new_height*channels);
    if(channels == 1) {
        jo_resize<1, unsigned char, unsigned char>(data, width, height, width*channels, new_data, new_width, new_height, new_width*channels, jo_resize_filter_mitchell, jo_resize_filter_mitchell_support);
    } else if(channels == 2) {
        jo_resize<2, unsigned char, unsigned char>(data, width, height, width*channels, new_data, new_width, new_height, new_width*channels, jo_resize_filter_mitchell, jo_resize_filter_mitchell_support);
    } else if(channels == 3) {
        jo_resize<3, unsigned char, unsigned char>(data, width, height, width*channels, new_data, new_width, new_height, new_width*channels, jo_resize_filter_mitchell, jo_resize_filter_mitchell_support);
    } else if(channels == 4) {
        jo_resize<4, unsigned char, unsigned char>(data, width, height, width*channels, new_data, new_width, new_height, new_width*channels, jo_resize_filter_mitchell, jo_resize_filter_mitchell_support);
    }

    jo_basic_canvas_ptr_t canvas_new = new_canvas(new_width, new_height, channels);
    for(int y = 0; y < new_height; y++) {
        for(int x = 0; x < new_width; x++) {
            for(int c = 0; c < channels; c++) {
                canvas_new->pixels->set(x*channels+c, y, new_data[(y*new_width+x)*channels+c]);
            }
        }
    }
    free(data);
    free(new_data);
    return new_node_canvas(canvas_new);
}

void jo_basic_canvas_init(env_ptr_t env) {
	env->set("canvas", new_node_native_function("canvas", &native_canvas, false, NODE_FLAG_PRERESOLVE));
	env->set("canvas/load-file", new_node_native_function("canvas/load-file", &native_canvas_load_file, false, NODE_FLAG_PRERESOLVE));
	env->set("canvas/save-file", new_node_native_function("canvas/save-file", &native_canvas_load_file, false, NODE_FLAG_PRERESOLVE));
	env->set("canvas/resize", new_node_native_function("canvas/resize", &native_canvas_resize, false, NODE_FLAG_PRERESOLVE));
}




