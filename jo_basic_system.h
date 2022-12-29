#pragma once

#include "jo_stdcpp.h"


#ifndef NO_NFD
#if defined _WIN32
#include "nfd/nfd_win.cpp"
#elif defined __APPLE__
#include "nfd/nfd_cocoa.m"
#elif defined __linux__
#include "nfd/nfd_gtk.c"
#else
#define NO_NFD
#endif
#endif

#ifndef NO_NFD
#include "nfd/nfd.h"
#include "nfd/nfd_common.c"
#endif

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

// execute a shell command
static node_idx_t native_system_exec_output(env_ptr_t env, list_ptr_t args) {
	jo_string str;
	for(list_t::iterator it(args); it; it++) {
		node_t *n = get_node(*it);
		str += " ";
		str += n->as_string(env);
	}
	jo_string output = "";
	FILE *fp = jo_popen(str.c_str(), "r");
	if (fp == NULL) {
		warnf("failed to run command '%s'\n", str.c_str());
	}
	char line[16384] = {0};
	while (fgets(line, sizeof(line), fp) != NULL) {
		output += line;
		output += "\n";
	}
	jo_pclose(fp);
	return new_node_string(output);
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

static node_idx_t native_system_getenv(env_ptr_t env, list_ptr_t args) { 
	const char *val = getenv(get_node_string(args->first_value()).c_str());
	return val ? new_node_string(val) : NIL_NODE; 
}
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
	node_idx_t file_idx = args->first_value();
	node_t *file = get_node(file_idx);
    char line[1024];
	line[0] = 0;
	if(file_idx == NIL_NODE ) {
		fgets(line, sizeof(line), stdin);
	} else if(file->is_file()) {
		fgets(line, sizeof(line), file->t_file);
	} else {
		warnf("read-line: not a file");
	}
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

static node_idx_t native_system_move_file(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string from = node->as_string(env);
	node_idx_t value_idx = *it++;
	node_t *value = get_node(value_idx);
	jo_string to = value->as_string(env);
	return new_node_bool(jo_file_move(from.c_str(), to.c_str()) == 0);
}

static node_idx_t native_system_copy_file(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	jo_string from = node->as_string(env);
	node_idx_t value_idx = *it++;
	node_t *value = get_node(value_idx);
	jo_string to = value->as_string(env);
	return new_node_bool(jo_file_copy(from.c_str(), to.c_str()) == 0);
}

static node_idx_t native_system_delete_file(env_ptr_t env, list_ptr_t args) {
	return new_node_bool(remove(get_node_string(args->first_value()).c_str()) == 0);
}

#ifndef NO_NFD
static node_idx_t native_system_open_dialog(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	jo_string filter_list = get_node_string(eval_node(env, *it++));
	jo_string default_path = get_node_string(eval_node(env, *it++));
	node_idx_t status_idx = *it++;
	if(get_node_type(status_idx) != NODE_SYMBOL) {
		warnf("open-dialog: status name must be a symbol");
		return NIL_NODE;
	}
	node_idx_t result_idx = *it++;
	if(get_node_type(result_idx) != NODE_SYMBOL) {
		warnf("open-dialog: result name must be a symbol");
		return NIL_NODE;
	}
	nfdchar_t *out_path = jo_strdup("");
    nfdresult_t result = NFD_OpenDialog( filter_list.c_str(), default_path.c_str(), &out_path );
	env_ptr_t env2 = new_env(env);
	switch(result) {
		case NFD_OKAY: env2->set_temp(status_idx, new_node_keyword("ok")); break;
		case NFD_CANCEL: env2->set_temp(status_idx, new_node_keyword("cancel")); break;
		case NFD_ERROR: env2->set_temp(status_idx, new_node_keyword("error")); break;
	}
	env2->set_temp(result_idx, new_node_string(out_path));
	free(out_path);
	node_idx_t res = NIL_NODE;
	for(; it; ++it) {
		res = eval_node(env2, *it);
	}
	return res;
}

static node_idx_t native_system_save_dialog(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	jo_string filter_list = get_node_string(eval_node(env, *it++));
	jo_string default_path = get_node_string(eval_node(env, *it++));
	node_idx_t status_idx = *it++;
	if(get_node_type(status_idx) != NODE_SYMBOL) {
		warnf("save-dialog: status name must be a symbol");
		return NIL_NODE;
	}
	node_idx_t result_idx = *it++;
	if(get_node_type(result_idx) != NODE_SYMBOL) {
		warnf("save-dialog: result name must be a symbol");
		return NIL_NODE;
	}
	nfdchar_t *out_path = jo_strdup("");
    nfdresult_t result = NFD_SaveDialog( filter_list.c_str(), default_path.c_str(), &out_path );
	env_ptr_t env2 = new_env(env);
	switch(result) {
		case NFD_OKAY: env2->set_temp(status_idx, new_node_keyword("ok")); break;
		case NFD_CANCEL: env2->set_temp(status_idx, new_node_keyword("cancel")); break;
		case NFD_ERROR: env2->set_temp(status_idx, new_node_keyword("error")); break;
	}
	env2->set_temp(result_idx, new_node_string(out_path));
	free(out_path);
	node_idx_t res = NIL_NODE;
	for(; it; ++it) {
		res = eval_node(env2, *it);
	}
	return res;
}

static node_idx_t native_system_pick_folder(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	jo_string default_path = get_node_string(eval_node(env, *it++));
	node_idx_t status_idx = *it++;
	if(get_node_type(status_idx) != NODE_SYMBOL) {
		warnf("pick-folder: status name must be a symbol");
		return NIL_NODE;
	}
	node_idx_t result_idx = *it++;
	if(get_node_type(result_idx) != NODE_SYMBOL) {
		warnf("pick-folder: result name must be a symbol");
		return NIL_NODE;
	}
	nfdchar_t *out_path = jo_strdup("");
    nfdresult_t result = NFD_PickFolder( default_path.c_str(), &out_path );
	env_ptr_t env2 = new_env(env);
	switch(result) {
		case NFD_OKAY: env2->set_temp(status_idx, new_node_keyword("ok")); break;
		case NFD_CANCEL: env2->set_temp(status_idx, new_node_keyword("cancel")); break;
		case NFD_ERROR: env2->set_temp(status_idx, new_node_keyword("error")); break;
	}
	env2->set_temp(result_idx, new_node_string(out_path));
	free(out_path);
	node_idx_t res = NIL_NODE;
	for(; it; ++it) {
		res = eval_node(env2, *it);
	}
	return res;
}
#endif

void jo_basic_system_init(env_ptr_t env) {
	env->set("sys/setenv", new_node_native_function("sys/setenv", &native_system_setenv, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/getenv", new_node_native_function("sys/getenv", &native_system_getenv, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/exec-output", new_node_native_function("sys/exec-output", &native_system_exec_output, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/exec", new_node_native_function("sys/exec", &native_system_exec, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/getcwd", new_node_native_function("sys/getcwd", &native_system_getcwd, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/chdir", new_node_native_function("sys/chdir", &native_system_chdir, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/kbhit", new_node_native_function("sys/kbhit", &native_system_kbhit, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/getch", new_node_native_function("sys/getch", &native_system_getch, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/date", new_node_native_function("sys/date", &native_system_date, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/currentTimeMillis", new_node_native_function("sys/currentTimeMillis", &native_system_currentTimeMillis, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/tmpnam", new_node_native_function("sys/tmpnam", &native_system_tmpnam, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/move-file", new_node_native_function("sys/move-file", &native_system_move_file, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/copy-file", new_node_native_function("sys/copy-file", &native_system_copy_file, false, NODE_FLAG_PRERESOLVE));
	env->set("sys/delete-file", new_node_native_function("sys/delete-file", &native_system_delete_file, false, NODE_FLAG_PRERESOLVE));
	env->set("read-line", new_node_native_function("read-line", &native_system_read_line, false, NODE_FLAG_PRERESOLVE));
	env->set("-d", new_node_native_function("-d", &native_system_dir_exists, false, NODE_FLAG_PRERESOLVE));
	env->set("-e", new_node_native_function("-e", &native_system_file_exists, false, NODE_FLAG_PRERESOLVE));
	env->set("-s", new_node_native_function("-s", &native_system_file_size, false, NODE_FLAG_PRERESOLVE));
	env->set("-r", new_node_native_function("-r", &native_system_file_readable, false, NODE_FLAG_PRERESOLVE));
	env->set("-w", new_node_native_function("-w", &native_system_file_writable, false, NODE_FLAG_PRERESOLVE));
	env->set("-x", new_node_native_function("-x", &native_system_file_executable, false, NODE_FLAG_PRERESOLVE));
	env->set("-z", new_node_native_function("-z", &native_system_file_empty, false, NODE_FLAG_PRERESOLVE));
	env->set("time/now", new_node_native_function("time/now", &native_time_now, false, NODE_FLAG_PRERESOLVE));
	
#ifndef NO_NFD
	env->set("sys/open-dialog", new_node_native_function("sys/open-dialog", &native_system_open_dialog, true, NODE_FLAG_PRERESOLVE));
	env->set("sys/save-dialog", new_node_native_function("sys/save-dialog", &native_system_save_dialog, true, NODE_FLAG_PRERESOLVE));
	env->set("sys/pick-folder", new_node_native_function("sys/pick-folder", &native_system_pick_folder, true, NODE_FLAG_PRERESOLVE));
#endif
}
