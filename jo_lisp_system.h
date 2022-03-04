#pragma once

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

void jo_lisp_system_init(list_ptr_t env) {
	env->push_back_inplace(new_node_var("System/setenv", new_node_native_function(&native_system_setenv, false)));
	env->push_back_inplace(new_node_var("System/getenv", new_node_native_function(&native_system_getenv, false)));
	env->push_back_inplace(new_node_var("System/exec", new_node_native_function(&native_system_exec, false)));
	env->push_back_inplace(new_node_var("System/getcwd", new_node_native_function(&native_system_getcwd, false)));
	env->push_back_inplace(new_node_var("-d", new_node_native_function(&native_system_dir_exists, false)));
	env->push_back_inplace(new_node_var("-e", new_node_native_function(&native_system_file_exists, false)));
	env->push_back_inplace(new_node_var("-s", new_node_native_function(&native_system_file_size, false)));
}