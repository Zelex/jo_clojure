#pragma once

#include "jo_stdcpp.h"

static double time_program_start = jo_time();

// execute a shell command
static node_idx_t native_system_exec(env_ptr_t env, list_ptr_t args) {
	jo_string str;
	for(list_t::iterator it = args->begin(); it; it++) {
		node_t *n = get_node(*it);
		str += " ";
		str += n->as_string(env);
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
	jo_string key = node->as_string(env);
	node_idx_t value_idx = *it++;
	node_t *value = get_node(value_idx);
	jo_string value_str = value->as_string(env);
	jo_setenv(key.c_str(), value_str.c_str(), 1);
	return NIL_NODE;
}

static node_idx_t native_system_getenv(env_ptr_t env, list_ptr_t args) { return new_node_string(getenv(get_node_string(args->first_value()).c_str())); }
static node_idx_t native_system_dir_exists(env_ptr_t env, list_ptr_t args) { return new_node_bool(jo_dir_exists(get_node_string(args->first_value()).c_str())); }
static node_idx_t native_system_file_exists(env_ptr_t env, list_ptr_t args) { return new_node_bool(jo_file_exists(get_node_string(args->first_value()).c_str())); }
static node_idx_t native_system_file_size(env_ptr_t env, list_ptr_t args) { return new_node_int(jo_file_size(get_node_string(args->first_value()).c_str())); }
static node_idx_t native_system_file_readable(env_ptr_t env, list_ptr_t args) { return new_node_bool(jo_file_readable(get_node_string(args->first_value()).c_str())); }
static node_idx_t native_system_file_writable(env_ptr_t env, list_ptr_t args) { return new_node_bool(jo_file_writable(get_node_string(args->first_value()).c_str())); }
static node_idx_t native_system_file_executable(env_ptr_t env, list_ptr_t args) { return new_node_bool(jo_file_executable(get_node_string(args->first_value()).c_str())); }
static node_idx_t native_system_file_empty(env_ptr_t env, list_ptr_t args) { return new_node_bool(jo_file_empty(get_node_string(args->first_value()).c_str())); }
static node_idx_t native_system_chdir(env_ptr_t env, list_ptr_t args) { return new_node_bool(jo_chdir(get_node_string(args->first_value()).c_str()) == 0); }
static node_idx_t native_system_kbhit(env_ptr_t env, list_ptr_t args) { return new_node_bool(jo_kbhit() != 0); }
static node_idx_t native_system_getch(env_ptr_t env, list_ptr_t args) { return new_node_int(jo_getch()); }
// returns current time since program start in seconds
static node_idx_t native_time_now(env_ptr_t env, list_ptr_t args) {	return new_node_float(jo_time() - time_program_start); }

static node_idx_t native_system_getcwd(env_ptr_t env, list_ptr_t args) {
	char cwd[256];
	getcwd(cwd, sizeof(cwd));
	cwd[sizeof(cwd)-1] = 0;
	return new_node_string(cwd);
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
	env->set("System/setenv", new_node_native_function("System/setenv", &native_system_setenv, false));
	env->set("System/getenv", new_node_native_function("System/getenv", &native_system_getenv, false));
	env->set("System/exec", new_node_native_function("System/exec", &native_system_exec, false));
	env->set("System/getcwd", new_node_native_function("System/getcwd", &native_system_getcwd, false));
	env->set("System/kbhit", new_node_native_function("System/kbhit", &native_system_kbhit, false));
	env->set("System/getch", new_node_native_function("System/getch", &native_system_getch, false));
	env->set("System/date", new_node_native_function("System/date", &native_system_date, false));
	env->set("System/sleep", new_node_native_function("System/sleep", &native_system_sleep, false));
	env->set("read-line", new_node_native_function("read-line", &native_system_read_line, false));
	env->set("-d", new_node_native_function("-d", &native_system_dir_exists, false));
	env->set("-e", new_node_native_function("-e", &native_system_file_exists, false));
	env->set("-s", new_node_native_function("-s", &native_system_file_size, false));
	env->set("-r", new_node_native_function("-r", &native_system_file_readable, false));
	env->set("-w", new_node_native_function("-w", &native_system_file_writable, false));
	env->set("-x", new_node_native_function("-x", &native_system_file_executable, false));
	env->set("-z", new_node_native_function("-z", &native_system_file_empty, false));
	env->set("Time/now", new_node_native_function("Time/now", &native_time_now, false));
}