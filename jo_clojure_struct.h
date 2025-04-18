#pragma once

// Functions for working with Clojure struct data types

// native_defstruct, native_struct, native_struct_map, native_accessor

static node_idx_t native_defstruct(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t struct_name = *it++;
	
	// Collect the field keys into a vector
	vector_ptr_t fields = new_vector();
	while (it) {
		node_idx_t field = *it++;
		fields->push_back_inplace(field);
	}
	
	// Create a structure definition
	hash_map_ptr_t struct_def = new_hash_map();
	struct_def->assoc_inplace(new_node_keyword("fields"), new_node_vector(fields), node_eq);
	
	// Create and store the structure factory function
	node_idx_t factory_fn = new_node_native_function(
		get_node_string(struct_name).c_str(), 
		[fields](env_ptr_t env, list_ptr_t args) -> node_idx_t {
			hash_map_ptr_t instance = new_hash_map();
			
			// Add fields from the struct definition with values from args
			size_t i = 0;
			list_t::iterator val_it(args);
			for (auto it = fields->begin(); it && val_it; it++, val_it++) {
				instance->assoc_inplace(*it, eval_node(env, *val_it), node_eq);
				i++;
			}
			
			// Fill remaining fields with nil
			for (; i < fields->size(); i++) {
				instance->assoc_inplace(fields->nth(i), NIL_NODE, node_eq);
			}
			
			return new_node_hash_map(instance);
		},
		false
	);
	
	// Store the factory function with the struct name
	env->set(struct_name, factory_fn);
	
	// Store the struct definition in a metadata map associated with the function
	env->set(new_node_symbol(get_node_string(struct_name) + "-struct-def"), new_node_hash_map(struct_def));
	
	return struct_name;
}

// (struct struct-name & field-values)
// Creates an instance of the specified struct with the given field values
static node_idx_t native_struct(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t struct_factory = eval_node(env, *it++);
	
	// If the argument is a symbol, look up the factory function
	if (get_node_type(struct_factory) == NODE_SYMBOL) {
		struct_factory = env->get(struct_factory);
		if (struct_factory == INV_NODE) {
			return new_node_exception("Struct not found: " + get_node_string(args->front()));
		}
	}
	
	// The factory should be a function
	if (get_node_type(struct_factory) != NODE_NATIVE_FUNC && get_node_type(struct_factory) != NODE_FUNC) {
		return new_node_exception("Not a struct factory: " + get_node_string(args->front()));
	}
	
	// Create the list of remaining arguments for the factory function
	list_ptr_t factory_args = new_list();
	while (it) {
		factory_args->push_back_inplace(eval_node(env, *it++));
	}
	
	// Create a new list with the factory function as the first element
	// followed by the evaluated arguments
	list_ptr_t call_list = new_list();
	call_list->push_back_inplace(struct_factory);
	for (list_t::iterator arg_it(factory_args); arg_it; ++arg_it) {
		call_list->push_back_inplace(*arg_it);
	}
	
	// Use eval_list to evaluate the function call
	return eval_list(env, call_list);
}

// (struct-map struct-name & key-value-pairs)
// Creates an instance of the specified struct with field values specified as key-value pairs
static node_idx_t native_struct_map(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t struct_name = eval_node(env, *it++);
	
	// If the argument is a symbol, look up the factory function
	if (get_node_type(struct_name) == NODE_SYMBOL) {
		struct_name = env->get(struct_name);
		if (struct_name == INV_NODE) {
			return new_node_exception("Struct not found: " + get_node_string(args->front()));
		}
	}
	
	// The factory should be a function
	if (get_node_type(struct_name) != NODE_NATIVE_FUNC && get_node_type(struct_name) != NODE_FUNC) {
		return new_node_exception("Not a struct factory: " + get_node_string(args->front()));
	}
	
	// Get the struct definition to find out the field order
	jo_string def_name = get_node_string(args->front()) + "-struct-def";
	node_idx_t struct_def_node = env->get(new_node_symbol(def_name));
	
	if (struct_def_node == INV_NODE || get_node_type(struct_def_node) != NODE_HASH_MAP) {
		return new_node_exception("Struct definition not found: " + get_node_string(args->front()));
	}
	
	hash_map_ptr_t struct_def = get_node_map(struct_def_node);
	node_idx_t fields_node = struct_def->get(new_node_keyword("fields"), node_eq);
	
	if (fields_node == INV_NODE || get_node_type(fields_node) != NODE_VECTOR) {
		return new_node_exception("Invalid struct definition");
	}
	
	vector_ptr_t fields = get_node_vector(fields_node);
	
	// Create a hash map from key-value pairs
	hash_map_ptr_t values_map = new_hash_map();
	
	while (it) {
		node_idx_t key = eval_node(env, *it++);
		if (!it) {
			return new_node_exception("struct-map: odd number of key-value pairs");
		}
		node_idx_t value = eval_node(env, *it++);
		values_map->assoc_inplace(key, value, node_eq);
	}
	
	// Extract values in the correct order
	list_ptr_t values_list = new_list();
	
	for (size_t i = 0; i < fields->size(); i++) {
		node_idx_t field = fields->nth(i);
		node_idx_t value = values_map->get(field, node_eq);
		values_list->push_back_inplace(value != INV_NODE ? value : NIL_NODE);
	}
	
	// Create struct instance using the struct factory function
	list_ptr_t call_list = new_list();
	call_list->push_back_inplace(struct_name);
	
	for (list_t::iterator val_it(values_list); val_it; ++val_it) {
		call_list->push_back_inplace(*val_it);
	}
	
	// Use eval_list to evaluate the function call
	return eval_list(env, call_list);
}

// (accessor struct-basis key)
// Returns an accessor function that extracts the specified field from a struct
static node_idx_t native_accessor(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t struct_name = *it++;
	node_idx_t key = eval_node(env, *it++);
	
	// Get the struct definition - we use the symbol directly without evaluating it
	jo_string def_name = get_node_string(struct_name) + "-struct-def";
	
	node_idx_t struct_def_node = env->get(new_node_symbol(def_name));
	
	if (struct_def_node == INV_NODE || get_node_type(struct_def_node) != NODE_HASH_MAP) {
		return new_node_exception("Struct definition not found: " + get_node_string(struct_name));
	}
	
	// Print the map contents for debugging
	hash_map_ptr_t struct_def = get_node_map(struct_def_node);
	
	// Get the fields directly using the keyword
	node_idx_t fields_keyword = new_node_keyword("fields");
	node_idx_t fields_node = struct_def->get(fields_keyword, node_eq);
	
	if (fields_node == INV_NODE || get_node_type(fields_node) != NODE_VECTOR) {
		return new_node_exception("Invalid struct definition");
	}
	
	// Find the position of the key in the fields
	vector_ptr_t fields = get_node_vector(fields_node);
	
	int position = -1;
	for (size_t i = 0; i < fields->size(); i++) {
		if (node_eq(fields->nth(i), key)) {
			position = i;
			break;
		}
	}
	
	if (position == -1) {
		return new_node_exception("Key not found in struct: " + get_node_string(key));
	}
	
	// Create and return an accessor function for this key/position
	return new_node_native_function(
		("accessor-" + get_node_string(key)).c_str(),
		[key, position](env_ptr_t env, list_ptr_t args) -> node_idx_t {
			node_idx_t struct_instance = eval_node(env, args->first_value());
			
			if (get_node_type(struct_instance) != NODE_HASH_MAP) {
				return new_node_exception("Not a struct instance");
			}
			
			hash_map_ptr_t instance = get_node_map(struct_instance);
			return instance->get(key, node_eq);
		},
		false
	);
}

void jo_clojure_struct_init(env_ptr_t env) {
    env->set("defstruct", new_node_native_function("defstruct", &native_defstruct, true, NODE_FLAG_PRERESOLVE));
    env->set("struct", new_node_native_function("struct", &native_struct, false, NODE_FLAG_PRERESOLVE));
    env->set("struct-map", new_node_native_function("struct-map", &native_struct_map, false, NODE_FLAG_PRERESOLVE));
    env->set("accessor", new_node_native_function("accessor", &native_accessor, false, NODE_FLAG_PRERESOLVE));
} 