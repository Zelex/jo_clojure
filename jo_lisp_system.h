#pragma once

#include "jo_stdcpp.h"

// execute a shell command
static node_idx_t native_system_exec(env_ptr_t env, list_ptr_t args) {
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
static node_idx_t native_system_setenv(env_ptr_t env, list_ptr_t args) {
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
static node_idx_t native_system_getenv(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string key = node->as_string();
	jo_string value = getenv(key.c_str());
	return new_node_string(value);
}

static node_idx_t native_system_dir_exists(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string path = node->as_string();
	return new_node_bool(jo_dir_exists(path.c_str()));
}

static node_idx_t native_system_file_exists(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string path = node->as_string();
	return new_node_bool(jo_file_exists(path.c_str()));
}

static node_idx_t native_system_file_size(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string path = node->as_string();
	return new_node_int(jo_file_size(path.c_str()));
}

static node_idx_t native_system_file_readable(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    return new_node_bool(jo_file_readable(path.c_str()));
}

static node_idx_t native_system_file_writable(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    return new_node_bool(jo_file_writable(path.c_str()));
}

static node_idx_t native_system_file_executable(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    return new_node_bool(jo_file_executable(path.c_str()));
}

static node_idx_t native_system_file_empty(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    return new_node_bool(jo_file_empty(path.c_str()));
}

static node_idx_t native_system_chdir(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string path = node->as_string();
	return new_node_bool(jo_chdir(path.c_str()) == 0);
}

static node_idx_t native_system_getcwd(env_ptr_t env, list_ptr_t args) {
	char cwd[256];
	getcwd(cwd, sizeof(cwd));
	cwd[sizeof(cwd)-1] = 0;
	return new_node_string(cwd);
}

static node_idx_t native_system_kbhit(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(jo_kbhit() != 0);
}

static node_idx_t native_system_getch(env_ptr_t env, list_ptr_t args) {
    return new_node_int(jo_getch());
}

static node_idx_t native_system_slurp(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    jo_string contents = (char*)jo_slurp_file(path.c_str());
    return new_node_string(contents);
}

static node_idx_t native_system_spit(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    jo_string path = node->as_string();
    node_idx_t contents_idx = *it++;
    node_t *contents = get_node(contents_idx);
    jo_string contents_str = contents->as_string();
    return new_node_bool(jo_spit_file(path.c_str(), contents_str.c_str()) == 0);
}

static node_idx_t native_system_date(env_ptr_t env, list_ptr_t args) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char buf[256];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return new_node_string(buf);
}

// reads a single line from stdin
static node_idx_t native_system_read_line(env_ptr_t env, list_ptr_t args) {
    char line[1024];
    fgets(line, sizeof(line), stdin);
    return new_node_string(line);
}

static node_idx_t native_system_sleep(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	float seconds = node->as_float();
	jo_sleep(seconds);
	return NIL_NODE;
}

void jo_lisp_system_init(env_ptr_t env) {
	env->set("System/setenv", new_node_native_function(&native_system_setenv, false));
	env->set("System/getenv", new_node_native_function(&native_system_getenv, false));
	env->set("System/exec", new_node_native_function(&native_system_exec, false));
	env->set("System/getcwd", new_node_native_function(&native_system_getcwd, false));
	env->set("System/kbhit", new_node_native_function(&native_system_kbhit, false));
	env->set("System/getch", new_node_native_function(&native_system_getch, false));
	env->set("System/date", new_node_native_function(&native_system_date, false));
	env->set("System/sleep", new_node_native_function(&native_system_sleep, false));
	env->set("slurp", new_node_native_function(&native_system_slurp, false));
	env->set("spit", new_node_native_function(&native_system_spit, false));
	env->set("read-line", new_node_native_function(&native_system_read_line, false));
	env->set("-d", new_node_native_function(&native_system_dir_exists, false));
	env->set("-e", new_node_native_function(&native_system_file_exists, false));
	env->set("-s", new_node_native_function(&native_system_file_size, false));
	env->set("-r", new_node_native_function(&native_system_file_readable, false));
	env->set("-w", new_node_native_function(&native_system_file_writable, false));
	env->set("-x", new_node_native_function(&native_system_file_executable, false));
	env->set("-z", new_node_native_function(&native_system_file_empty, false));
}