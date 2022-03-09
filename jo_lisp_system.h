#pragma once

#include "jo_stdcpp.h"

// execute a shell command
node_idx_t native_system_exec(list_ptr_t env, list_ptr_t args) {
	jo_string str;
	for(list_t::iterator it = args->begin(); it; it++) {
		node_t *n = get_node(*it);
		str += " ";
		str += n->as_string();
	}
	//printf("system_exec: %s\n", str.c_str());
	int ret = system(str.c_str());
	return new_node_int(ret);
}

// Set system enviorment variables
node_idx_t native_system_setenv(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string key = node->as_string();
	node_idx_t value_idx = *it++;
	node_t *value = get_node(value_idx);
	jo_string value_str = value->as_string();
	jo_setenv(key.c_str(), value_str.c_str(), 1);
	return NIL_NODE;
}

// Get system enviorment variables
node_idx_t native_system_getenv(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string key = node->as_string();
	jo_string value = getenv(key.c_str());
	return new_node_string(value);
}

node_idx_t native_system_dir_exists(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string path = node->as_string();
	return new_node_bool(jo_dir_exists(path.c_str()));
}

node_idx_t native_system_file_exists(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string path = node->as_string();
	return new_node_bool(jo_file_exists(path.c_str()));
}

node_idx_t native_system_file_size(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string path = node->as_string();
	return new_node_int(jo_file_size(path.c_str()));
}

node_idx_t native_system_file_readable(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    return new_node_bool(jo_file_readable(path.c_str()));
}

node_idx_t native_system_file_writable(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    return new_node_bool(jo_file_writable(path.c_str()));
}

node_idx_t native_system_file_executable(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    return new_node_bool(jo_file_executable(path.c_str()));
}

node_idx_t native_system_file_empty(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    return new_node_bool(jo_file_empty(path.c_str()));
}

node_idx_t native_system_chdir(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string path = node->as_string();
	return new_node_bool(jo_chdir(path.c_str()) == 0);
}

node_idx_t native_system_getcwd(list_ptr_t env, list_ptr_t args) {
	char cwd[256];
	getcwd(cwd, sizeof(cwd));
	cwd[sizeof(cwd)-1] = 0;
	return new_node_string(cwd);
}

node_idx_t native_system_kbhit(list_ptr_t env, list_ptr_t args) {
    return new_node_bool(jo_kbhit() != 0);
}

node_idx_t native_system_getch(list_ptr_t env, list_ptr_t args) {
    return new_node_int(jo_getch());
}

node_idx_t native_system_slurp(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    jo_string contents = (char*)jo_slurp_file(path.c_str());
    return new_node_string(contents);
}

node_idx_t native_system_spit(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    node_idx_t contents_idx = *it++;
    node_t *contents = get_node(contents_idx);
    jo_string contents_str = contents->as_string();
    return new_node_bool(jo_spit_file(path.c_str(), contents_str.c_str()) == 0);
}

node_idx_t native_system_date(list_ptr_t env, list_ptr_t args) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char buf[256];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return new_node_string(buf);
}

// reads a single line from stdin
node_idx_t native_system_read_line(list_ptr_t env, list_ptr_t args) {
    char line[1024];
    fgets(line, sizeof(line), stdin);
    return new_node_string(line);
}

node_idx_t native_system_sleep(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	float seconds = node->as_float();
	jo_sleep(seconds);
	return NIL_NODE;
}

void jo_lisp_system_init(list_ptr_t env) {
	env->push_back_inplace(new_node_var("System/setenv", new_node_native_function(&native_system_setenv, false)));
	env->push_back_inplace(new_node_var("System/getenv", new_node_native_function(&native_system_getenv, false)));
	env->push_back_inplace(new_node_var("System/exec", new_node_native_function(&native_system_exec, false)));
	env->push_back_inplace(new_node_var("System/getcwd", new_node_native_function(&native_system_getcwd, false)));
	env->push_back_inplace(new_node_var("System/kbhit", new_node_native_function(&native_system_kbhit, false)));
	env->push_back_inplace(new_node_var("System/getch", new_node_native_function(&native_system_getch, false)));
	env->push_back_inplace(new_node_var("System/date", new_node_native_function(&native_system_date, false)));
	env->push_back_inplace(new_node_var("System/sleep", new_node_native_function(&native_system_sleep, false)));
	env->push_back_inplace(new_node_var("slurp", new_node_native_function(&native_system_slurp, false)));
	env->push_back_inplace(new_node_var("spit", new_node_native_function(&native_system_spit, false)));
	env->push_back_inplace(new_node_var("read-line", new_node_native_function(&native_system_read_line, false)));
	env->push_back_inplace(new_node_var("-d", new_node_native_function(&native_system_dir_exists, false)));
	env->push_back_inplace(new_node_var("-e", new_node_native_function(&native_system_file_exists, false)));
	env->push_back_inplace(new_node_var("-s", new_node_native_function(&native_system_file_size, false)));
	env->push_back_inplace(new_node_var("-r", new_node_native_function(&native_system_file_readable, false)));
	env->push_back_inplace(new_node_var("-w", new_node_native_function(&native_system_file_writable, false)));
	env->push_back_inplace(new_node_var("-x", new_node_native_function(&native_system_file_executable, false)));
	env->push_back_inplace(new_node_var("-z", new_node_native_function(&native_system_file_empty, false)));
}