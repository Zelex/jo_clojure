#pragma once

#include <stdio.h>

// (file-seq dir)
// A tree seq on java.io.Files
static node_idx_t native_file_seq(env_ptr_t env, list_ptr_t args) {
    return NIL_NODE;
}

static node_idx_t native_io_slurp(env_ptr_t env, list_ptr_t args) {
	// TODO: HTTP/HTTPS!
    jo_string path = get_node_string(args->first_value());
    char *c = (char*)jo_slurp_file(path.c_str());
    node_idx_t ret = new_node_string(c);
    free(c);
    return ret;
}

static node_idx_t native_io_spit(env_ptr_t env, list_ptr_t args) {
	// TODO: HTTP/HTTPS!
    jo_string path = get_node_string(args->first_value());
    jo_string contents_str = get_node_string(args->second_value());
    return new_node_bool(jo_spit_file(path.c_str(), contents_str.c_str()) == 0);
}

// (file arg)(file parent child)(file parent child & more)
// Returns a java.io.File, passing each arg to as-file.  Multiple-arg
// versions treat the first argument as parent and subsequent args as
// children relative to the parent.
static node_idx_t native_io_file(env_ptr_t env, list_ptr_t args) {
	jo_string str;
	for(list_t::iterator it = args->begin(); it; it++) {
		node_t *n = get_node(*it);
        #ifdef _WIN32
            if(str.empty()) {
                str += n->as_string().replace("/", "\\");
            } else {
                str += "\\" + n->as_string().replace("/", "\\");
            }
        #else
            if(str.empty()) {
                str += n->as_string().replace("\\", "/");
            } else {
                str += "/" + n->as_string().replace("\\", "/");
            }
        #endif
	}
    FILE *fp = fopen(str.c_str(), "r");
    if(!fp) {
        return NIL_NODE;
    }
    return new_node_file(fp);
}

// (delete-file f & [silently])
// Delete file f. If silently is nil or false, raise an exception on failure, 
// else return the value of silently.
static node_idx_t native_io_delete_file(env_ptr_t env, list_ptr_t args) {
    jo_string path = get_node_string(args->first_value());
#ifdef _WIN32
    if(!DeleteFile(path.c_str())) {
#else
    if(unlink(path.c_str())) {
#endif
        if(!get_node_bool(args->second_value())) {
            warnf("Failed to delete file %s", path.c_str());
        }
    }
    return args->second_value();
}

// (copy input output & opts)
// Copies input to output.
static node_idx_t native_io_copy(env_ptr_t env, list_ptr_t args) {
    jo_string input = get_node_string(args->first_value());
    jo_string output = get_node_string(args->second_value());
    return new_node_bool(jo_file_copy(input.c_str(), output.c_str()) == 0);
}

void jo_lisp_io_init(env_ptr_t env) {
	env->set("file-seq", new_node_native_function("file-seq", &native_file_seq, false));
	env->set("slurp", new_node_native_function("slurp", &native_io_slurp, false));
	env->set("spit", new_node_native_function("spit", &native_io_spit, false));
	env->set("io/file", new_node_native_function("io/file", &native_io_file, false));
	env->set("io/delete-file", new_node_native_function("io/delete-file", &native_io_delete_file, false));
	env->set("io/copy", new_node_native_function("io/copy", &native_io_copy, false));
}
