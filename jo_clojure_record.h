#pragma once

// Implement record type functionality for Clojure-like records

// Note: NODE_RECORD type should be added to the main enum in jo_clojure.cpp

// Check if a node is a record
static inline bool is_record(node_idx_t idx) {
    return get_node_type(idx) == NODE_RECORD;
}

// Create a record from a map
static node_idx_t map_to_record(node_idx_t map_idx, const jo_string& record_name) {
    if (get_node_type(map_idx) != NODE_HASH_MAP) {
        return new_node_exception("Cannot convert non-map to record");
    }
    
    hash_map_ptr_t map = get_node_map(map_idx);
    hash_map_ptr_t record_map = new_hash_map(*map);
    
    // Add type metadata
    record_map->assoc_inplace(new_node_keyword("type"), new_node_symbol(record_name), node_eq);
    
    // Create the record node (hash map with record type)
    node_idx_t record_node = new_node_hash_map(record_map);
    get_node(record_node)->type = NODE_RECORD;
    
    return record_node;
}

// Helper function to add a field to a record map with both symbol and keyword keys
static void add_field_to_record(hash_map_ptr_t record_map, node_idx_t field_sym, node_idx_t value) {
    // Add field with symbol key (original)
    record_map->assoc_inplace(field_sym, value, node_eq);
    
    // Add field with keyword key as well for better compatibility
    if (get_node_type(field_sym) == NODE_SYMBOL) {
        jo_string field_name = get_node_string(field_sym);
        node_idx_t field_kw = new_node_keyword(field_name);
        record_map->assoc_inplace(field_kw, value, node_eq);
    }
}

// (defrecord Name [field1 field2 ...])
// Defines a new record type with a constructor function and field accessors
static node_idx_t native_defrecord(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t name_node = *it++;
    node_idx_t fields_node = *it++;
    
    if (get_node_type(name_node) != NODE_SYMBOL) {
        return new_node_exception("defrecord: name must be a symbol");
    }
    
    if (get_node_type(fields_node) != NODE_VECTOR) {
        return new_node_exception("defrecord: fields must be a vector");
    }
    
    jo_string record_name = get_node_string(name_node);
    vector_ptr_t fields = get_node_vector(fields_node);
    
    // Create type metadata - store the field names and their positions
    hash_map_ptr_t fields_map = new_hash_map();
    for (size_t i = 0; i < fields->size(); i++) {
        node_idx_t field = fields->nth(i);
        if (get_node_type(field) != NODE_SYMBOL) {
            return new_node_exception("defrecord: field must be a symbol");
        }
        fields_map->assoc_inplace(field, new_node_int(i), node_eq);
    }
    
    // Store record type definition with a special name
    jo_string def_name = record_name + "-record-def";
    env->set(new_node_symbol(def_name), new_node_hash_map(fields_map));
    
    // Create constructor function with the same name as the record
    jo_string factory_name = record_name;
    env->set(new_node_symbol(factory_name), new_node_native_function(factory_name.c_str(), 
        [record_name, fields](env_ptr_t env, list_ptr_t args) -> node_idx_t {
            // Create a map with the field values
            hash_map_ptr_t record_map = new_hash_map();
            
            // Add record type metadata
            record_map->assoc_inplace(new_node_keyword("type"), new_node_symbol(record_name), node_eq);
            
            // Add each field with corresponding value
            for (size_t i = 0; i < fields->size() && i < args->size(); i++) {
                // Use helper to add both symbol and keyword keys
                add_field_to_record(record_map, fields->nth(i), args->nth(i));
            }
            
            // For any remaining fields, use nil
            for (size_t i = args->size(); i < fields->size(); i++) {
                add_field_to_record(record_map, fields->nth(i), NIL_NODE);
            }
            
            // Return the record as a map but mark it with NODE_RECORD type
            node_idx_t record_node = new_node_hash_map(record_map);
            get_node(record_node)->type = NODE_RECORD;
            
            return record_node;
        }, false));
    
    // Create constructor function with -> prefix
    jo_string constructor_name = "->" + record_name;
    env->set(new_node_symbol(constructor_name), new_node_native_function(constructor_name.c_str(),
        [record_name, fields](env_ptr_t env, list_ptr_t args) -> node_idx_t {
            // Create a new map that will become our record
            hash_map_ptr_t record_map = new_hash_map();
            
            // Add record type metadata
            record_map->assoc_inplace(new_node_keyword("type"), new_node_symbol(record_name), node_eq);
            
            // Add field values from args
            if (args->size() != fields->size()) {
                warnf("Record constructor %s expects %zu fields, got %zu", 
                      record_name.c_str(), fields->size(), args->size());
                return NIL_NODE;
            }
            
            list_t::iterator args_it(args);
            for (size_t i = 0; i < fields->size(); i++) {
                if (!args_it) break;
                node_idx_t field = fields->nth(i);
                node_idx_t value = *args_it++;
                
                // Use helper to add both symbol and keyword keys
                add_field_to_record(record_map, field, value);
            }
            
            // Create the record node
            node_idx_t record_node = new_node_hash_map(record_map);
            get_node(record_node)->type = NODE_RECORD;
            
            return record_node;
        }, false));
    
    // Create a map->record function
    env->set(new_node_symbol("map->" + record_name), new_node_native_function(("map->" + record_name).c_str(),
        [record_name, fields](env_ptr_t env, list_ptr_t args) -> node_idx_t {
            node_idx_t map_arg = args->first_value();
            
            if (get_node_type(map_arg) != NODE_HASH_MAP && get_node_type(map_arg) != NODE_RECORD) {
                warnf("map->%s expects a map, got %s", record_name.c_str(), get_node_type_string(map_arg));
                return NIL_NODE;
            }
            
            // Create a new map that will become our record
            hash_map_ptr_t record_map = new_hash_map();
            
            // Add record type metadata
            record_map->assoc_inplace(new_node_keyword("type"), new_node_symbol(record_name), node_eq);
            
            // Add values from the input map
            hash_map_ptr_t input_map = get_node_map(map_arg);
            for (auto it = input_map->begin(); it; it++) {
                record_map->assoc_inplace(it->first, it->second, node_eq);
                
                // If this is a field (symbol), also add the keyword version
                if (get_node_type(it->first) == NODE_SYMBOL) {
                    jo_string field_name = get_node_string(it->first);
                    node_idx_t field_kw = new_node_keyword(field_name);
                    record_map->assoc_inplace(field_kw, it->second, node_eq);
                }
                // If this is a keyword, also add the symbol version
                else if (get_node_type(it->first) == NODE_KEYWORD) {
                    jo_string field_name = get_node_string(it->first);
                    node_idx_t field_sym = new_node_symbol(field_name);
                    record_map->assoc_inplace(field_sym, it->second, node_eq);
                }
            }
            
            // Set any missing fields to nil
            for (size_t i = 0; i < fields->size(); i++) {
                node_idx_t field = fields->nth(i);
                if (!record_map->contains(field, node_eq)) {
                    // Use helper to add both symbol and keyword keys
                    add_field_to_record(record_map, field, NIL_NODE);
                }
            }
            
            // Return the record
            node_idx_t record_node = new_node_hash_map(record_map);
            get_node(record_node)->type = NODE_RECORD;
            
            return record_node;
        }, false));
    
    // Add dot constructor syntax (Record.)
    jo_string dot_constructor = record_name + ".";
    env->set(new_node_symbol(dot_constructor), new_node_native_function(dot_constructor.c_str(),
        [record_name, fields](env_ptr_t env, list_ptr_t args) -> node_idx_t {
            // This is the same as the record constructor function
            // Create a map with the field values
            hash_map_ptr_t record_map = new_hash_map();
            
            // Add record type metadata
            record_map->assoc_inplace(new_node_keyword("type"), new_node_symbol(record_name), node_eq);
            
            // Add each field with corresponding value
            for (size_t i = 0; i < fields->size() && i < args->size(); i++) {
                // Use helper to add both symbol and keyword keys
                add_field_to_record(record_map, fields->nth(i), args->nth(i));
            }
            
            // For any remaining fields, use nil
            for (size_t i = args->size(); i < fields->size(); i++) {
                add_field_to_record(record_map, fields->nth(i), NIL_NODE);
            }
            
            // Return the record as a map but mark it with NODE_RECORD type
            node_idx_t record_node = new_node_hash_map(record_map);
            get_node(record_node)->type = NODE_RECORD;
            
            return record_node;
        }, false));
    
    // Create predicate function to check if a value is of this record type
    jo_string pred_name = record_name + "?";
    env->set(new_node_symbol(pred_name), new_node_native_function(pred_name.c_str(),
        [record_name](env_ptr_t env, list_ptr_t args) -> node_idx_t {
            node_idx_t value = args->first_value();
            
            if (get_node_type(value) != NODE_RECORD) {
                return FALSE_NODE;
            }
            
            hash_map_ptr_t map = get_node_map(value);
            auto type_it = map->find(new_node_keyword("type"), node_eq);
            
            if (!type_it.third || get_node_string(type_it.second) != record_name) {
                return FALSE_NODE;
            }
            
            return TRUE_NODE;
        }, false));
    
    // Create keyword accessors for each field
    for (size_t i = 0; i < fields->size(); i++) {
        node_idx_t field = fields->nth(i);
        jo_string field_name = get_node_string(field);
        
        // Create the keyword accessor for this field
        node_idx_t keyword = new_node_keyword(field_name);
        env->set(keyword, new_node_native_function((":" + field_name).c_str(), 
            [field](env_ptr_t env, list_ptr_t args) -> node_idx_t {
                node_idx_t target = args->first_value();
                node_idx_t default_value = args->size() > 1 ? args->nth(1) : NIL_NODE;
                
                // Handle both records and maps
                int type = get_node_type(target);
                if (type == NODE_RECORD || type == NODE_HASH_MAP) {
                    hash_map_ptr_t map = get_node_map(target);
                    auto it = map->find(field, node_eq);
                    if (it.third) {
                        return it.second;
                    }
                    return default_value;
                }
                
                return default_value;
            }, false));
            
        // Create dot accessor for this field (.-field)
        jo_string dot_accessor_name = ".-" + field_name;
        env->set(new_node_symbol(dot_accessor_name), new_node_native_function(dot_accessor_name.c_str(),
            [field_name](env_ptr_t env, list_ptr_t args) -> node_idx_t {
                if (args->size() != 1) {
                    warnf("(%s record) requires 1 argument", (".-" + field_name).c_str());
                    return NIL_NODE;
                }
                
                node_idx_t record_idx = args->first_value();
                node_t *record = get_node(record_idx);
                
                if (!record->is_record() && !record->is_hash_map()) {
                    warnf("(%s record) requires a record or map as the argument", (".-" + field_name).c_str());
                    return NIL_NODE;
                }
                
                // Access field using keyword
                node_idx_t key = new_node_keyword(field_name);
                hash_map_ptr_t record_map = record->as_hash_map();
                auto entry = record_map->find(key, node_eq);
                if (entry.third) {
                    return entry.second;
                }
                
                // Look for a matching symbol as well
                node_idx_t sym_key = new_node_symbol(field_name);
                auto sym_entry = record_map->find(sym_key, node_eq);
                if (sym_entry.third) {
                    return sym_entry.second;
                }
                
                return NIL_NODE;
            }, false));
    }
    
    // Return the record name
    return name_node;
}

// Check if a value is any kind of record
static node_idx_t native_is_record(env_ptr_t env, list_ptr_t args) {
    node_idx_t value = args->first_value();
    return get_node_type(value) == NODE_RECORD ? TRUE_NODE : FALSE_NODE;
}

// Initialize the record functionality
void jo_clojure_record_init(env_ptr_t env) {
    // Register the defrecord macro
    env->set("defrecord", new_node_native_function("defrecord", &native_defrecord, true, NODE_FLAG_LITERAL_ARGS));
    
    // Register predicates
    env->set("record?", new_node_native_function("record?", &native_is_record, false, NODE_FLAG_PRERESOLVE));
    
    // Make sure NODE_RECORD type is properly handled in native_get, native_assoc, and native_dissoc
    // These functions should be modified in jo_clojure.cpp to handle NODE_RECORD in the same way as NODE_HASH_MAP
}