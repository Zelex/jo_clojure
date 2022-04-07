#pragma once

#include "jo_stdcpp.h"

// concat all arguments as_string
static node_idx_t native_str(env_ptr_t env, list_ptr_t args) {
	jo_string str;
	for(list_t::iterator it = args->begin(); it; it++) {
		node_t *n = get_node(*it);
		str += n->as_string();
	}
	return new_node_string(str);
}

// Returns the substring of ‘s’ beginning at start inclusive, and ending at end (defaults to length of string), exclusive.
static node_idx_t native_subs(env_ptr_t env, list_ptr_t args) { return new_node_string(get_node_string(args->first_value()).substr(get_node_int(args->second_value()), get_node_int(args->third_value()))); }
static node_idx_t native_compare(env_ptr_t env, list_ptr_t args) { return new_node_int(get_node_string(args->first_value()).compare(get_node_string(args->second_value()))); }
static node_idx_t native_lower_case(env_ptr_t env, list_ptr_t args) { return new_node_string(get_node_string(args->first_value()).lower()); }
static node_idx_t native_upper_case(env_ptr_t env, list_ptr_t args) { return new_node_string(get_node_string(args->first_value()).upper()); }
static node_idx_t native_trim(env_ptr_t env, list_ptr_t args) { return new_node_string(get_node_string(args->first_value()).trim()); }
static node_idx_t native_triml(env_ptr_t env, list_ptr_t args) { return new_node_string(get_node_string(args->first_value()).ltrim()); }
static node_idx_t native_trimr(env_ptr_t env, list_ptr_t args) { return new_node_string(get_node_string(args->first_value()).rtrim()); }
static node_idx_t native_trim_newline(env_ptr_t env, list_ptr_t args) { return new_node_string(get_node_string(args->first_value()).chomp()); }
static node_idx_t native_replace(env_ptr_t env, list_ptr_t args) { return new_node_string(get_node_string(args->first_value()).replace(get_node_string(args->second_value()).c_str(), get_node_string(args->third_value()).c_str())); }
static node_idx_t native_replace_first(env_ptr_t env, list_ptr_t args) { return new_node_string(get_node_string(args->first_value()).replace_first(get_node_string(args->second_value()).c_str(), get_node_string(args->third_value()).c_str())); }

// splits a string separated by newlines into a list of strings
static node_idx_t native_split_lines(env_ptr_t env, list_ptr_t args) {
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
static node_idx_t native_join(env_ptr_t env, list_ptr_t args) {
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

// True if s is nil, empty, or contains only whitespace.
static node_idx_t native_is_blank(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    if(node_idx == NIL_NODE) {
        return TRUE_NODE;
    }
    node_t *node = get_node(node_idx);
    if(node->is_string()) {
        return new_node_bool(node->as_string().empty());
    }
    return FALSE_NODE;
}

// Converts first character of the string to upper-case, all other characters to lower-case.
static node_idx_t native_capitalize(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    if(!node->is_string()) {
        return NIL_NODE;
    }
    return new_node_string(node->as_string().capitalize());
}

static node_idx_t native_ends_with(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    if(!node->is_string()) {
        return NIL_NODE;
    }
    node_idx = *it++;
    node_t *what = get_node(node_idx);
    return new_node_bool(node->as_string().ends_with(what->as_string().c_str()));
}

static node_idx_t native_starts_with(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    if(!node->is_string()) {
        return NIL_NODE;
    }
    node_idx = *it++;
    node_t *what = get_node(node_idx);
    return new_node_bool(node->as_string().starts_with(what->as_string().c_str()));
}

static node_idx_t native_includes(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    if(!node->is_string()) {
        return NIL_NODE;
    }
    node_idx = *it++;
    node_t *what = get_node(node_idx);
    return new_node_bool(node->as_string().includes(what->as_string().c_str()));
}

static node_idx_t native_index_of(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    if(!node->is_string()) {
        return NIL_NODE;
    }
    node_idx = *it++;
    node_t *what = get_node(node_idx);
    return new_node_int(node->as_string().index_of(what->as_string().c_str()));
}

static node_idx_t native_last_index_of(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    if(!node->is_string()) {
        return NIL_NODE;
    }
    node_idx = *it++;
    node_t *what = get_node(node_idx);
    return new_node_int(node->as_string().last_index_of(what->as_string().c_str()));
}

static node_idx_t native_is_string(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    return new_node_bool(node->is_string());
}

static node_idx_t native_ston(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    if(!node->is_string()) {
        return NIL_NODE;
    }
    return new_node_int(atoi(node->as_string().c_str()));
}

static node_idx_t native_ntos(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    if(!node->is_int()) {
        return NIL_NODE;
    }
    return new_node_string(node->as_string());
}

void jo_lisp_string_init(env_ptr_t env) {
	env->set("str", new_node_native_function("str", &native_str, false));
	env->set("subs", new_node_native_function("subs", &native_subs, false));
	env->set("compare", new_node_native_function("compare", &native_compare, false));
	env->set("lower-case", new_node_native_function("lower-case", &native_lower_case, false));
	env->set("upper-case", new_node_native_function("upper-case", &native_upper_case, false));
	env->set("trim", new_node_native_function("trim", &native_trim, false));
	env->set("triml", new_node_native_function("triml", &native_triml, false));
	env->set("trimr", new_node_native_function("trimr", &native_trimr, false));
	env->set("trim-newline", new_node_native_function("trim-newline", &native_trim_newline, false));
	env->set("replace", new_node_native_function("replace", &native_replace, false));
	env->set("replace-first", new_node_native_function("replace-first", &native_replace_first, false));
	env->set("split-lines", new_node_native_function("split-lines", &native_split_lines, false));
	env->set("join", new_node_native_function("join", &native_join, false));
	env->set("blank?", new_node_native_function("blank", &native_is_blank, false));
	env->set("capitalize", new_node_native_function("capitalize", &native_capitalize, false));
	env->set("ends-with?", new_node_native_function("ends-with?", &native_ends_with, false));
	env->set("starts-with?", new_node_native_function("starts-with?", &native_starts_with, false));
	env->set("includes?", new_node_native_function("includes?", &native_includes, false));
	env->set("index-of", new_node_native_function("index-of", &native_index_of, false));
	env->set("last-index-of", new_node_native_function("last-index-of", &native_last_index_of, false));
	env->set("string?", new_node_native_function("string?", &native_is_string, false));
	env->set("ston", new_node_native_function("ston", &native_ston, false));
	env->set("ntos", new_node_native_function("ntos", &native_ntos, false));
}
