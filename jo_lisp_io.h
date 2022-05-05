#pragma once

#include "jo_stdcpp.h"

// This functionality deviates from clojure's IO library, cause in my personal opinion, 
// clojure's IO library is not great. I assume because Rich was trying to be purist.
// However, IMO, side-effects are side-effects and you can't avoid them with files.
// Might as well embrace them in this case.

// (file-seq dir)
// A tree seq on java.io.Files
static node_idx_t native_io_file_seq(env_ptr_t env, list_ptr_t args) {
    return NIL_NODE;
}

// this API is OK
static node_idx_t native_io_slurp(env_ptr_t env, list_ptr_t args) {
	// TODO: HTTP/HTTPS!
    jo_string path = get_node_string(args->first_value());
    char *c = (char*)jo_slurp_file(path.c_str());
    node_idx_t ret = new_node_string(c);
    free(c);
    return ret;
}

// this API is OK
static node_idx_t native_io_spit(env_ptr_t env, list_ptr_t args) {
	// TODO: HTTP/HTTPS!
    jo_string path = get_node_string(args->first_value());
    jo_string contents_str = get_node_string(args->second_value());
    return new_node_bool(jo_spit_file(path.c_str(), contents_str.c_str()) == 0);
}

// (file opts arg)(file opts parent child)(file opts parent child & more)
// Returns a java.io.File, passing each arg to as-file.  Multiple-arg
// versions treat the first argument as parent and subsequent args as
// children relative to the parent.
static node_idx_t native_io_open_file(env_ptr_t env, list_ptr_t args) {
    if(args->size() < 2) {
        return NIL_NODE;
    }
    list_t::iterator it = args->begin();
    jo_string opts = get_node_string(*it++);
    jo_string str;
    for(; it; it++) {
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
    FILE *fp = fopen(str.c_str(), opts.c_str());
    if(!fp) {
        return NIL_NODE;
    }
    return new_node_file(fp);
}

// (io/close-file file)
// Closes the file and frees the node.
static node_idx_t native_io_close_file(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE) {
        return NIL_NODE;
    }
    if(n->t_file) {
        fclose(n->t_file);
        n->t_file = 0;
    }
    return NIL_NODE;
}

// (io/read-line file)
// Reads a line from the file.
static node_idx_t native_io_read_line(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE) {
        return NIL_NODE;
    }
    if(!n->t_file) {
        return NIL_NODE;
    }
    char buf[1024]; // TODO: not sure I like this fixed size, but its better to have a limit than no limit... right?
    if(!fgets(buf, 1024, n->t_file)) {
        return NIL_NODE;
    }
    return new_node_string(buf);
}

static node_idx_t native_io_write_file(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE) {
        return NIL_NODE;
    }
    if(!n->t_file) {
        return NIL_NODE;
    }
    jo_string str = get_node_string(args->second_value());
    fputs(str.c_str(), n->t_file);
    return NIL_NODE;
}

// (io/write-line file str)
// Writes a line to the file.
static node_idx_t native_io_write_line(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE) {
        return NIL_NODE;
    }
    if(!n->t_file) {
        return NIL_NODE;
    }
    jo_string str = get_node_string(args->second_value());
    fputs(str.c_str(), n->t_file);
    fputs("\n", n->t_file);
    return NIL_NODE;
}

// (io/flush-file file)
// Flushes the file.
static node_idx_t native_io_flush_file(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE) {
        return NIL_NODE;
    }
    if(!n->t_file) {
        return NIL_NODE;
    }
    fflush(n->t_file);
    return NIL_NODE;
}

// (io/seek-file file pos)
// Seeks to the specified position in the file.
static node_idx_t native_io_seek_file(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE) {
        return NIL_NODE;
    }
    if(!n->t_file) {
        return NIL_NODE;
    }
    int pos = get_node_int(args->second_value());
    fseek(n->t_file, pos, SEEK_SET);
    return NIL_NODE;
}

// (io/tell-file file)
// Returns the current position in the file.
static node_idx_t native_io_tell_file(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE) {
        return NIL_NODE;
    }
    if(!n->t_file) {
        return NIL_NODE;
    }
    return new_node_int(ftell(n->t_file));
}

// (io/size-file file)
// Returns the size of the file.
static node_idx_t native_io_size_file(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE) {
        return NIL_NODE;
    }
    if(!n->t_file) {
        return NIL_NODE;
    }
    long pos = ftell(n->t_file);
    fseek(n->t_file, 0, SEEK_END);
    long size = ftell(n->t_file);
    fseek(n->t_file, pos, SEEK_SET);
    return new_node_int(size);
}

// (io/eof-file file)
// Returns true if the file is at the end.
static node_idx_t native_io_eof_file(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE) {
        return NIL_NODE;
    }
    if(!n->t_file) {
        return NIL_NODE;
    }
    return new_node_bool(feof(n->t_file));
}

// (io/file-exists? file)
// Returns true if the file exists.
static node_idx_t native_io_file_exists(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_STRING) {
        return NIL_NODE;
    }
    return new_node_bool(jo_file_exists(n->as_string().c_str()));
}

// (io/file-readable? file)
// Returns true if the file is readable.
static node_idx_t native_io_file_readable(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_STRING) {
        return NIL_NODE;
    }
    return new_node_bool(jo_file_readable(n->as_string().c_str()));
}

// (io/file-writable? file)
// Returns true if the file is writable.
static node_idx_t native_io_file_writable(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_STRING) {
        return NIL_NODE;
    }
    return new_node_bool(jo_file_writable(n->as_string().c_str()));
}

// (io/file-executable? file)
// Returns true if the file is executable.
static node_idx_t native_io_file_executable(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_STRING) {
        return NIL_NODE;
    }
    return new_node_bool(jo_file_executable(n->as_string().c_str()));
}

// (io/delete-file f & [silently])
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

// (copy input output)
// Copies input to output.
static node_idx_t native_io_copy(env_ptr_t env, list_ptr_t args) {
    jo_string input = get_node_string(args->first_value());
    jo_string output = get_node_string(args->second_value());
    return new_node_bool(jo_file_copy(input.c_str(), output.c_str()) == 0);
}

// (io/popen cmd opts)
// Opens a pipe to cmd.
static node_idx_t native_io_popen(env_ptr_t env, list_ptr_t args) {
    jo_string cmd = get_node_string(args->first_value());
    jo_string opts = get_node_string(args->second_value());
    FILE *fp = popen(cmd.c_str(), opts.c_str());
    if(!fp) {
        return NIL_NODE;
    }
    return new_node_file(fp);
}

// (io/pclose file)
// Closes the file and frees the node.
static node_idx_t native_io_pclose(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE) {
        return NIL_NODE;
    }
    if(n->t_file) {
        pclose(n->t_file);
        n->t_file = 0;
    }
    return NIL_NODE;
}

void jo_lisp_io_init(env_ptr_t env) {
    env->set("file-seq", new_node_native_function("file-seq", &native_io_file_seq, false));
    env->set("slurp", new_node_native_function("slurp", &native_io_slurp, false));
    env->set("spit", new_node_native_function("spit", &native_io_spit, false));
    env->set("io/open-file", new_node_native_function("io/open-file", &native_io_open_file, false));
    env->set("io/close-file", new_node_native_function("io/close-file", &native_io_close_file, false));
    env->set("io/write-file", new_node_native_function("io/write-file", &native_io_write_file, false));
    env->set("io/read-line", new_node_native_function("io/read-line", &native_io_read_line, false));
    env->set("io/write-line", new_node_native_function("io/write-line", &native_io_write_line, false));
    env->set("io/flush-file", new_node_native_function("io/flush-file", &native_io_flush_file, false));
    env->set("io/seek-file", new_node_native_function("io/seek-file", &native_io_seek_file, false));
    env->set("io/tell-file", new_node_native_function("io/tell-file", &native_io_tell_file, false));
    env->set("io/size-file", new_node_native_function("io/size-file", &native_io_size_file, false));
    env->set("io/eof-file", new_node_native_function("io/eof-file", &native_io_eof_file, false));
    env->set("io/file-exists?", new_node_native_function("io/file-exists?", &native_io_file_exists, false));
    env->set("io/file-readable?", new_node_native_function("io/file-readable?", &native_io_file_readable, false));
    env->set("io/file-writable?", new_node_native_function("io/file-writable?", &native_io_file_writable, false));
    env->set("io/file-executable?", new_node_native_function("io/file-executable?", &native_io_file_executable, false));
    env->set("io/delete-file", new_node_native_function("io/delete-file", &native_io_delete_file, false));
    env->set("io/copy", new_node_native_function("io/copy", &native_io_copy, false));
}
