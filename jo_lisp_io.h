#pragma once

#include "jo_stdcpp.h"

// for popen and pclose
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#endif

// for opendir and closedir
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#define opendir _opendir
#define closedir _closedir
#else
#include <dirent.h>
#endif


// This functionality deviates from clojure's IO library, cause in my personal opinion, 
// clojure's IO library is not great. I assume because Rich was trying to be purist.
// However, IMO, side-effects are side-effects and you can't avoid them with files.
// Might as well embrace them in this case.

// TODO: should directory scans be lazy? would make sense for read-dir-all and the like

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
                str += n->as_string(env).replace("/", "\\");
            } else {
                str += "\\" + n->as_string(env).replace("/", "\\");
            }
        #else
            if(str.empty()) {
                str += n->as_string(env).replace("\\", "/");
            } else {
                str += "/" + n->as_string(env).replace("\\", "/");
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
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    char buf[1024]; // TODO: not sure I like this fixed size, but its better to have a limit than no limit... right?
    if(!fgets(buf, 1024, n->t_file)) {
        return NIL_NODE;
    }
    return new_node_string(buf);
}

// (io/write-line file str)
// Writes a line to the file.
static node_idx_t native_io_write_line(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    jo_string str = get_node_string(args->second_value());
    fputs(str.c_str(), n->t_file);
    fputs("\n", n->t_file);
    return NIL_NODE;
}

// (io/flush file)
// Flushes the file.
static node_idx_t native_io_flush(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    fflush(n->t_file);
    return NIL_NODE;
}

// (io/seek file pos)
// Seeks to the specified position in the file.
static node_idx_t native_io_seek(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    int pos = get_node_int(args->second_value());
    fseek(n->t_file, pos, SEEK_SET);
    return NIL_NODE;
}

// (io/tell file)
// Returns the current position in the file.
static node_idx_t native_io_tell(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    return new_node_int(ftell(n->t_file));
}

// (io/size file)
// Returns the size of the file.
static node_idx_t native_io_size(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    long pos = ftell(n->t_file);
    fseek(n->t_file, 0, SEEK_END);
    long size = ftell(n->t_file);
    fseek(n->t_file, pos, SEEK_SET);
    return new_node_int(size);
}

// (io/eof file)
// Returns true if the file is at the end.
static node_idx_t native_io_eof(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    return new_node_bool(feof(n->t_file));
}

// (io/file-exists? file)
// Returns true if the file exists.
static node_idx_t native_io_file_exists(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(jo_file_exists(get_node_string(args->first_value()).c_str()));
}

// (io/file-readable? file)
// Returns true if the file is readable.
static node_idx_t native_io_file_readable(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(jo_file_readable(get_node_string(args->first_value()).c_str()));
}

// (io/file-writable? file)
// Returns true if the file is writable.
static node_idx_t native_io_file_writable(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(jo_file_writable(get_node_string(args->first_value()).c_str()));
}

// (io/file-executable? file)
// Returns true if the file is executable.
static node_idx_t native_io_file_executable(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(jo_file_executable(get_node_string(args->first_value()).c_str()));
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

// (io/proc-open cmd opts)
// Opens a pipe to cmd.
static node_idx_t native_io_open_proc(env_ptr_t env, list_ptr_t args) {
    jo_string cmd = get_node_string(args->first_value());
    jo_string opts = get_node_string(args->second_value());
    FILE *fp = popen(cmd.c_str(), opts.c_str());
    if(!fp) {
        return NIL_NODE;
    }
    return new_node_file(fp);
}

// (io/proc-close file)
// Closes the file and frees the node.
static node_idx_t native_io_close_proc(env_ptr_t env, list_ptr_t args) {
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

static node_idx_t native_io_write_str(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    jo_string str = get_node_string(args->second_value());
    fputs(str.c_str(), n->t_file);
    return NIL_NODE;
}

static node_idx_t native_io_write_int(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    int i = get_node_int(args->second_value());
    fwrite(&i, sizeof(int), 1, n->t_file);
    return NIL_NODE;
}

static node_idx_t native_io_write_float(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    float f = get_node_float(args->second_value());
    fwrite(&f, sizeof(float), 1, n->t_file);
    return NIL_NODE;
}

static node_idx_t native_io_write_short(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    int s = get_node_int(args->second_value());
    fwrite(&s, sizeof(short), 1, n->t_file);
    return NIL_NODE;
}

static node_idx_t native_io_write_byte(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    int b = get_node_int(args->second_value());
    fwrite(&b, sizeof(char), 1, n->t_file);
    return NIL_NODE;
}

static node_idx_t native_io_read_int(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    int i;
    fread(&i, sizeof(int), 1, n->t_file);
    return new_node_int(i);
}

static node_idx_t native_io_read_float(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    float f;
    fread(&f, sizeof(float), 1, n->t_file);
    return new_node_float(f);
}

static node_idx_t native_io_read_short(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    short s;
    fread(&s, sizeof(short), 1, n->t_file);
    return new_node_int(s);
}

static node_idx_t native_io_read_byte(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    char b;
    fread(&b, sizeof(char), 1, n->t_file);
    return new_node_int(b);
}

// (io/read-str file terminator)
// Reads a string from the file.
static node_idx_t native_io_read_str(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_FILE || !n->t_file) {
        return NIL_NODE;
    }
    char buf[1024];
    int term = '\n';
    if(args->size() == 2) {
        term = get_node_int(args->second_value());
    }
    int i = 0;
    while(1) {
        int c = fgetc(n->t_file);
        if(c == EOF || c == term) {
            break;
        }
        buf[i] = c;
        i++;
    }
    buf[i] = 0;
    return new_node_string(buf);
}

static node_idx_t native_io_open_dir(env_ptr_t env, list_ptr_t args) {
    jo_string str = get_node_string(args->first_value());
#ifdef _WIN32
    HANDLE h = FindFirstFile(str.c_str(), NULL);
    if(h == INVALID_HANDLE_VALUE) {
        return NIL_NODE;
    }
    return new_node_dir(h);
#else
    DIR *dir = opendir(str.c_str());
    if(!dir) {
        return NIL_NODE;
    }
    return new_node_dir(dir);
#endif
}

static node_idx_t native_io_close_dir(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_DIR || !n->t_dir) {
        return NIL_NODE;
    }
    if(n->t_dir != 0) {
#ifdef _WIN32
        FindClose(n->t_dir);
#else
        closedir((DIR*)n->t_dir);
#endif
        n->t_dir = NULL;
    }
    return NIL_NODE;
}

// (io/read-dir dir)
// Reads a directory entry from the directory.
static node_idx_t native_io_read_dir(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_DIR || !n->t_dir) {
        return NIL_NODE;
    }
#ifdef _WIN32
    WIN32_FIND_DATA fd;
    if(!FindNextFile(n->t_dir, &fd)) {
        return NIL_NODE;
    }
    return new_node_string(fd.cFileName);
#else
    struct dirent *ent = readdir((DIR*)n->t_dir);
    if(!ent) {
        return NIL_NODE;
    }
    return new_node_string(ent->d_name);
#endif
}

// (io/read-dir-all dir)
// Reads all directory entries from the directory.
static node_idx_t native_io_read_dir_all(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_DIR || !n->t_dir) {
        return NIL_NODE;
    }
    list_ptr_t list = new_list();
    while(1) {
#ifdef _WIN32
        WIN32_FIND_DATA fd;
        if(!FindNextFile(n->t_dir, &fd)) {
            break;
        }
        list->push_back_inplace(new_node_string(fd.cFileName));
#else
        struct dirent *ent = readdir((DIR*)n->t_dir);
        if(!ent) {
            break;
        }
        list->push_back_inplace(new_node_string(ent->d_name));
#endif
    }
    return new_node_list(list);
}

// (io/read-dir-files dir)
// Reads all files from the directory.
static node_idx_t native_io_read_dir_files(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_DIR || !n->t_dir) {
        return NIL_NODE;
    }
    list_ptr_t list = new_list();
    while(1) {
#ifdef _WIN32
        WIN32_FIND_DATA fd;
        if(!FindNextFile(n->t_dir, &fd)) {
            break;
        }
        if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        list->push_back_inplace(new_node_string(fd.cFileName));
#else
        struct dirent *ent = readdir((DIR*)n->t_dir);
        if(!ent) {
            break;
        }
        if(ent->d_type == DT_REG) {
            list->push_back_inplace(new_node_string(ent->d_name));
        }
#endif
    }
    return new_node_list(list);
}

// (io/read-dir-dirs dir)
// Reads all directories from the directory.
static node_idx_t native_io_read_dir_dirs(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_DIR || !n->t_dir) {
        return NIL_NODE;
    }
    list_ptr_t list = new_list();
    while(1) {
#ifdef _WIN32
        WIN32_FIND_DATA fd;
        if(!FindNextFile(n->t_dir, &fd)) {
            break;
        }
        if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            list->push_back_inplace(new_node_string(fd.cFileName));
        }
#else
        struct dirent *ent = readdir((DIR*)n->t_dir);
        if(!ent) {
            break;
        }
        if(ent->d_type == DT_DIR) {
            list->push_back_inplace(new_node_string(ent->d_name));
        }
#endif
    }
    return new_node_list(list);
}

// (io/rewind-dir dir)
// Rewinds the directory.
static node_idx_t native_io_rewind_dir(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_DIR || !n->t_dir) {
        return NIL_NODE;
    }
#ifdef _WIN32
    FindClose(n->t_dir);
#else
    rewinddir((DIR*)n->t_dir);
#endif
    return NIL_NODE;
}

// (io/tell-dir dir)
// Returns the current position in the directory.
static node_idx_t native_io_tell_dir(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    if(n->type != NODE_DIR || !n->t_dir) {
        return NIL_NODE;
    }
#ifdef _WIN32
    return new_node_int(0);
#else
    return new_node_int(telldir((DIR*)n->t_dir));
#endif
}



void jo_lisp_io_init(env_ptr_t env) {
    env->set("file-seq", new_node_native_function("file-seq", &native_io_file_seq, false));
    env->set("slurp", new_node_native_function("slurp", &native_io_slurp, false));
    env->set("spit", new_node_native_function("spit", &native_io_spit, false));
    env->set("io/open-dir", new_node_native_function("io/open-dir", &native_io_open_dir, false));
    env->set("io/close-dir", new_node_native_function("io/close-dir", &native_io_close_dir, false));
    env->set("io/read-dir", new_node_native_function("io/read-dir", &native_io_read_dir, false));
    env->set("io/read-dir-all", new_node_native_function("io/read-dir-all", &native_io_read_dir_all, false));
    env->set("io/read-dir-files", new_node_native_function("io/read-dir-files", &native_io_read_dir_files, false));
    env->set("io/read-dir-dirs", new_node_native_function("io/read-dir-dirs", &native_io_read_dir_dirs, false));
    env->set("io/rewind-dir", new_node_native_function("io/rewind-dir", &native_io_rewind_dir, false));
    env->set("io/tell-dir", new_node_native_function("io/tell-dir", &native_io_tell_dir, false));
    env->set("io/open-proc", new_node_native_function("io/open-proc", &native_io_open_proc, false));
    env->set("io/close-proc", new_node_native_function("io/close-proc", &native_io_close_proc, false));
    env->set("io/open-file", new_node_native_function("io/open-file", &native_io_open_file, false));
    env->set("io/close-file", new_node_native_function("io/close-file", &native_io_close_file, false));
    env->set("io/read-line", new_node_native_function("io/read-line", &native_io_read_line, false));
    env->set("io/read-str", new_node_native_function("io/read-str", &native_io_read_str, false));
    env->set("io/read-int", new_node_native_function("io/read-int", &native_io_read_int, false));
    env->set("io/read-float", new_node_native_function("io/read-float", &native_io_read_float, false));
    env->set("io/read-short", new_node_native_function("io/read-short", &native_io_read_short, false));
    env->set("io/read-byte", new_node_native_function("io/read-byte", &native_io_read_byte, false));
    env->set("io/write-line", new_node_native_function("io/write-line", &native_io_write_line, false));
    env->set("io/write-str", new_node_native_function("io/write-str", &native_io_write_str, false));
    env->set("io/write-int", new_node_native_function("io/write-int", &native_io_write_int, false));
    env->set("io/write-float", new_node_native_function("io/write-float", &native_io_write_float, false));
    env->set("io/write-short", new_node_native_function("io/write-short", &native_io_write_short, false));
    env->set("io/write-byte", new_node_native_function("io/write-byte", &native_io_write_byte, false));
    env->set("io/flush", new_node_native_function("io/flush", &native_io_flush, false));
    env->set("io/seek", new_node_native_function("io/seek", &native_io_seek, false));
    env->set("io/tell", new_node_native_function("io/tell", &native_io_tell, false));
    env->set("io/size", new_node_native_function("io/size", &native_io_size, false));
    env->set("io/eof?", new_node_native_function("io/eof?", &native_io_eof, false));
    env->set("io/file-exists?", new_node_native_function("io/file-exists?", &native_io_file_exists, false));
    env->set("io/file-readable?", new_node_native_function("io/file-readable?", &native_io_file_readable, false));
    env->set("io/file-writable?", new_node_native_function("io/file-writable?", &native_io_file_writable, false));
    env->set("io/file-executable?", new_node_native_function("io/file-executable?", &native_io_file_executable, false));
    env->set("io/delete-file", new_node_native_function("io/delete-file", &native_io_delete_file, false));
    env->set("io/copy", new_node_native_function("io/copy", &native_io_copy, false));
    env->set("*in*", new_node_file(stdin));
    env->set("*out*", new_node_file(stdout));
    env->set("*err*", new_node_file(stderr));

}
