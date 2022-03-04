#pragma once

#include "jo_stdcpp.h"

// concat all arguments as_string
node_idx_t native_str(list_ptr_t env, list_ptr_t args) {
	jo_string str;
	for(list_t::iterator it = args->begin(); it; it++) {
		node_t *n = get_node(*it);
		str += n->as_string();
	}
	return new_node_string(str);
}

// Returns the substring of ‘s’ beginning at start inclusive, and ending at end (defaults to length of string), exclusive.
node_idx_t native_subs(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_t *node = get_node(*it++);
    node_t *start = get_node(*it++);
    node_t *end = get_node(*it++);
    return new_node_string(node->as_string().substr(start->as_int(), end->as_int()));
}

node_idx_t native_compare(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_t *node = get_node(*it++);
    node_t *other = get_node(*it++);
    return new_node_int(node->as_string().compare(other->as_string()));
}

node_idx_t native_lower_case(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_t *node = get_node(*it++);
    return new_node_string(node->as_string().lower());
}

node_idx_t native_upper_case(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_t *node = get_node(*it++);
    return new_node_string(node->as_string().upper());
}

node_idx_t native_trim(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_t *node = get_node(*it++);
    return new_node_string(node->as_string().trim());
}

node_idx_t native_triml(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_t *node = get_node(*it++);
    return new_node_string(node->as_string().ltrim());
}

node_idx_t native_trimr(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_t *node = get_node(*it++);
    return new_node_string(node->as_string().rtrim());
}

node_idx_t native_replace(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_t *node = get_node(*it++);
    node_t *what = get_node(*it++);
    node_t *with = get_node(*it++);
    return new_node_string(node->as_string().replace(what->as_string().c_str(), with->as_string().c_str()));
}

// splits a string separated by newlines into a list of strings
node_idx_t native_split_lines(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(!node->is_string()) {
		return NIL_NODE;
	}
	jo_string str = node->as_string();
	list_ptr_t list_list = new_list();
	int pos = 0;
	int prev_pos = 0;
	while(pos != jo_string_npos) {
		pos = str.find('\n', prev_pos);
		if(pos == jo_string_npos) {
			list_list->push_back_inplace(new_node_string(str.substr(prev_pos)));
		} else {
			list_list->push_back_inplace(new_node_string(str.substr(prev_pos, pos - prev_pos)));
		}
		prev_pos = pos + 1;
	}
	return new_node_list(list_list);
}

// (join sep collection)
node_idx_t native_join(list_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    if(!node->is_list()) {
        return NIL_NODE;
    }
    jo_string sep = get_node(*it++)->as_string();
    list_ptr_t list = node->as_list();
    jo_string str;
    for(list_t::iterator it = list->begin(); it;) {
        node_t *n = get_node(*it++);
        str += n->as_string();
        if(it) {
            str += sep;
        }
    }
    return new_node_string(str);
}

void jo_lisp_string_init(list_ptr_t env) {
	env->push_back_inplace(new_node_var("str", new_node_native_function(&native_str, false)));
	env->push_back_inplace(new_node_var("subs", new_node_native_function(&native_subs, false)));
	env->push_back_inplace(new_node_var("compare", new_node_native_function(&native_compare, false)));
	env->push_back_inplace(new_node_var("lower-case", new_node_native_function(&native_lower_case, false)));
	env->push_back_inplace(new_node_var("upper-case", new_node_native_function(&native_upper_case, false)));
	env->push_back_inplace(new_node_var("trim", new_node_native_function(&native_trim, false)));
	env->push_back_inplace(new_node_var("triml", new_node_native_function(&native_triml, false)));
	env->push_back_inplace(new_node_var("trimr", new_node_native_function(&native_trimr, false)));
	env->push_back_inplace(new_node_var("replace", new_node_native_function(&native_replace, false)));
	env->push_back_inplace(new_node_var("split-lines", new_node_native_function(&native_split_lines, false)));
	env->push_back_inplace(new_node_var("join", new_node_native_function(&native_join, false)));
}
