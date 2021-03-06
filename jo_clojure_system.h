#pragma once

#include "jo_stdcpp.h"


// execute a shell command
static node_idx_t native_system_exec(env_ptr_t env, list_ptr_t args) {
	jo_string str;
	for(list_t::iterator it(args); it; it++) {
		node_t *n = get_node(*it);
		str += " ";
		str += n->as_string(env);
	}
	//printf("system_exec: %s\n", str.c_str());
	long long ret = system(str.c_str());
	return new_node_int(ret);
}

// Set system enviorment variables
static node_idx_t native_system_setenv(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
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
static node_idx_t native_time_now(env_ptr_t env, list_ptr_t args) { return new_node_float(jo_time() - time_program_start); }

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

static node_idx_t native_system_currentTimeMillis(env_ptr_t env, list_ptr_t args) {
	return new_node_int((jo_time() - time_program_start) * 1000);
}

static node_idx_t native_system_tmpnam(env_ptr_t env, list_ptr_t args) {
	char *t = jo_tmpnam();
	node_idx_t ret = new_node_string(t);
	free(t);
	return ret;
}

void jo_clojure_system_init(env_ptr_t env) {
	env->set("System/setenv", new_node_native_function("System/setenv", &native_system_setenv, false, NODE_FLAG_PRERESOLVE));
	env->set("System/getenv", new_node_native_function("System/getenv", &native_system_getenv, false, NODE_FLAG_PRERESOLVE));
	env->set("System/exec", new_node_native_function("System/exec", &native_system_exec, false, NODE_FLAG_PRERESOLVE));
	env->set("System/getcwd", new_node_native_function("System/getcwd", &native_system_getcwd, false, NODE_FLAG_PRERESOLVE));
	env->set("System/chdir", new_node_native_function("System/chdir", &native_system_chdir, false, NODE_FLAG_PRERESOLVE));
	env->set("System/kbhit", new_node_native_function("System/kbhit", &native_system_kbhit, false, NODE_FLAG_PRERESOLVE));
	env->set("System/getch", new_node_native_function("System/getch", &native_system_getch, false, NODE_FLAG_PRERESOLVE));
	env->set("System/date", new_node_native_function("System/date", &native_system_date, false, NODE_FLAG_PRERESOLVE));
	env->set("System/currentTimeMillis", new_node_native_function("System/currentTimeMillis", &native_system_currentTimeMillis, false, NODE_FLAG_PRERESOLVE));
	env->set("System/tmpnam", new_node_native_function("System/tmpnam", &native_system_tmpnam, false, NODE_FLAG_PRERESOLVE));
	env->set("read-line", new_node_native_function("read-line", &native_system_read_line, false, NODE_FLAG_PRERESOLVE));
	env->set("-d", new_node_native_function("-d", &native_system_dir_exists, false, NODE_FLAG_PRERESOLVE));
	env->set("-e", new_node_native_function("-e", &native_system_file_exists, false, NODE_FLAG_PRERESOLVE));
	env->set("-s", new_node_native_function("-s", &native_system_file_size, false, NODE_FLAG_PRERESOLVE));
	env->set("-r", new_node_native_function("-r", &native_system_file_readable, false, NODE_FLAG_PRERESOLVE));
	env->set("-w", new_node_native_function("-w", &native_system_file_writable, false, NODE_FLAG_PRERESOLVE));
	env->set("-x", new_node_native_function("-x", &native_system_file_executable, false, NODE_FLAG_PRERESOLVE));
	env->set("-z", new_node_native_function("-z", &native_system_file_empty, false, NODE_FLAG_PRERESOLVE));
	env->set("Time/now", new_node_native_function("Time/now", &native_time_now, false, NODE_FLAG_PRERESOLVE));
}