#pragma once

// (file arg)(file parent child)(file parent child & more)
// Returns a java.io.File, passing each arg to as-file.  Multiple-arg
// versions treat the first argument as parent and subsequent args as
// children relative to the parent.
static node_idx_t native_io_file(env_ptr_t env, list_ptr_t args) {
    return NIL_NODE;
}

// (file-seq dir)
// A tree seq on java.io.Files
static node_idx_t native_file_seq(env_ptr_t env, list_ptr_t args) {
    return NIL_NODE;
}

// (delete-file f & [silently])
// Delete file f. If silently is nil or false, raise an exception on failure, 
// else return the value of silently.
static node_idx_t native_io_delete_file(env_ptr_t env, list_ptr_t args) {
    return NIL_NODE;
}

// (copy input output & opts)
// Copies input to output.
static node_idx_t native_io_copy(env_ptr_t env, list_ptr_t args) {
    return NIL_NODE;
}

void jo_lisp_io_init(env_ptr_t env) {
	env->set("io/file", new_node_native_function("io/file", &native_io_file, false));
	env->set("file-seq", new_node_native_function("file-seq", &native_file_seq, false));
	env->set("io/delete-file", new_node_native_function("io/delete-file", &native_io_delete_file, false));
	env->set("io/copy", new_node_native_function("io/copy", &native_io_copy, false));
}
