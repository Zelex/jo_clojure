#pragma once

// Implement protocol functionality for Clojure-like protocols

// Protocol implementation will be based on hash maps stored in regular nodes
// rather than using a dedicated NODE_PROTOCOL type

// Helper function to get the type keyword for a node
static node_idx_t get_type_keyword(node_idx_t node_idx) {
    int node_type = get_node_type(node_idx);
    switch (node_type) {
        case NODE_NIL:      return new_node_keyword("nil");
        case NODE_BOOL:     return new_node_keyword("boolean");
        case NODE_INT:      return new_node_keyword("integer");
        case NODE_FLOAT:    return new_node_keyword("float");
        case NODE_STRING:   return new_node_keyword("string");
        case NODE_SYMBOL:   return new_node_keyword("symbol");
        case NODE_KEYWORD:  return new_node_keyword("keyword");
        case NODE_LIST:     return new_node_keyword("list");
        case NODE_LAZY_LIST:return new_node_keyword("lazy-seq");
        case NODE_VECTOR:   return new_node_keyword("vector");
        case NODE_HASH_MAP: return new_node_keyword("map");
        case NODE_HASH_SET: return new_node_keyword("set");
        case NODE_RECORD: {
            // For records, try to get the type name from the record
            hash_map_ptr_t record_map = get_node_map(node_idx);
            
            // First option: check for explicit :type key
            auto type_it = record_map->find(new_node_keyword("type"), node_eq);
            if (type_it.third) {
                return type_it.second;
            }
            
            // Second option: defrecord case - type is in the name map entry with key __type
            // For defrecord, we need to look at type as symbol, not keyword
            auto meta_it = record_map->find(new_node_symbol("__type"), node_eq);
            if (meta_it.third) {
                node_idx_t type_kw = new_node_keyword(get_node_string(meta_it.second));
                return type_kw;
            }
            
            // Fallback
            return new_node_keyword("record");
        }
        default:
            return new_node_keyword(get_node_type_string(node_idx));
    }
}


// Check if a node is a protocol
static inline bool is_protocol(node_idx_t idx) {
    if (get_node_type(idx) != NODE_HASH_MAP) {
        return false;
    }
    
    hash_map_ptr_t map = get_node_map(idx);
    auto type_it = map->find(new_node_keyword("type"), node_eq);
    return type_it.third && get_node_type(type_it.second) == NODE_KEYWORD && 
           get_node_string(type_it.second) == "protocol";
}

// Define a new protocol
// (defprotocol ProtocolName
//   "Optional docstring"
//   (method-name [this arg1 arg2] "Optional doc")
//   (another-method [this] "Optional doc"))
static node_idx_t native_defprotocol(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    
    // Get protocol name (should be a symbol)
    if (!it) {
        warnf("defprotocol: expected protocol name");
        return NIL_NODE;
    }
    
    node_idx_t proto_name = *it++;
    if (get_node_type(proto_name) != NODE_SYMBOL) {
        warnf("defprotocol: protocol name must be a symbol");
        return NIL_NODE;
    }
    
    // Check for docstring
    node_idx_t doc_string = NIL_NODE;
    if (it && get_node_type(*it) == NODE_STRING) {
        doc_string = *it++;
    }
    
    // Create a map to hold the protocol definition
    hash_map_ptr_t proto_map = new_hash_map();
    
    // Add protocol metadata
    proto_map->assoc_inplace(new_node_keyword("type"), new_node_keyword("protocol"), node_eq);
    proto_map->assoc_inplace(new_node_keyword("name"), proto_name, node_eq);
    
    if (doc_string != NIL_NODE) {
        proto_map->assoc_inplace(new_node_keyword("doc"), doc_string, node_eq);
    }
    
    // Create a map to hold method signatures
    hash_map_ptr_t methods_map = new_hash_map();
    
    // Process each method definition
    while (it) {
        // First item should be a list representing the method signature
        if (get_node_type(*it) != NODE_LIST) {
            warnf("defprotocol: expected method signature list");
            return NIL_NODE;
        }
        
        list_ptr_t method_list = get_node_list(*it++);
        list_t::iterator method_it(method_list);
        
        // First item in the list should be the method name (a symbol)
        if (!method_it || get_node_type(*method_it) != NODE_SYMBOL) {
            warnf("defprotocol: expected method name symbol");
            return NIL_NODE;
        }
        
        node_idx_t method_name = *method_it++;
        
        // Next item should be the arglist (a vector)
        if (!method_it || get_node_type(*method_it) != NODE_VECTOR) {
            warnf("defprotocol: expected method argument vector");
            return NIL_NODE;
        }
        
        node_idx_t arg_vector = *method_it++;
        
        // Check for docstring
        node_idx_t method_doc = NIL_NODE;
        if (method_it && get_node_type(*method_it) == NODE_STRING) {
            method_doc = *method_it++;
        }
        
        // Create method signature map
        hash_map_ptr_t method_map = new_hash_map();
        method_map->assoc_inplace(new_node_keyword("name"), method_name, node_eq);
        method_map->assoc_inplace(new_node_keyword("args"), arg_vector, node_eq);
        
        if (method_doc != NIL_NODE) {
            method_map->assoc_inplace(new_node_keyword("doc"), method_doc, node_eq);
        }
        
        // Add method to methods map
        methods_map->assoc_inplace(method_name, new_node_hash_map(method_map), node_eq);
    }
    
    // Add methods to the protocol map
    proto_map->assoc_inplace(new_node_keyword("methods"), new_node_hash_map(methods_map), node_eq);
    
    // Create a map to hold implementations (by type)
    hash_map_ptr_t impls_map = new_hash_map();
    proto_map->assoc_inplace(new_node_keyword("impls"), new_node_hash_map(impls_map), node_eq);
    
    // Store the protocol in the environment
    node_idx_t proto_node = new_node_hash_map(proto_map);
    env->set(proto_name, proto_node);
    
    // Create dispatcher functions for each method
    for (auto method_it = methods_map->begin(); method_it; ++method_it) {
        node_idx_t method_name = method_it->first;
        node_idx_t method_sig = method_it->second;
        
        hash_map_ptr_t method_sig_map = get_node_map(method_sig);
        auto args_it = method_sig_map->find(new_node_keyword("args"), node_eq);
        if (!args_it.third) continue;
        
        node_idx_t args_vector = args_it.second;
        vector_ptr_t args_vec = get_node_vector(args_vector);
        
        // The first argument is 'this', which is the target object
        if (args_vec->size() == 0) {
            warnf("defprotocol: method %s has no arguments", get_node_string(method_name).c_str());
            continue;
        }
        
        // Create a dispatcher function that will look up the implementation based on the type
        env->set(method_name, new_node_native_function(get_node_string(method_name).c_str(), 
            [proto_node, method_name](env_ptr_t env, list_ptr_t args) -> node_idx_t {
                if (args->empty()) {
                    warnf("Protocol method requires at least one argument");
                    return NIL_NODE;
                }
                
                // Get the target object and determine its type
                node_idx_t target = args->first_value();
                int target_type = get_node_type(target); // Keep target_type for later check

                // Use get_type_keyword helper function
                node_idx_t type_kw = get_type_keyword(target);
                
                // Look up the implementation for this type
                hash_map_ptr_t proto_map = get_node_map(proto_node);
                auto impls_it = proto_map->find(new_node_keyword("impls"), node_eq);
                if (!impls_it.third) {
                    warnf("Protocol %s not properly initialized (no impls map)", 
                          get_node_string(proto_map->find(new_node_keyword("name"), node_eq).second).c_str());
                    return NIL_NODE;
                }
                
                hash_map_ptr_t impls_map = get_node_map(impls_it.second);
                auto type_it = impls_map->find(type_kw, node_eq);
                if (!type_it.third) {
                    // Find protocol name for error message (assuming it exists)
                    node_idx_t proto_name_node = NIL_NODE;
                    auto name_it = proto_map->find(new_node_keyword("name"), node_eq);
                    if (name_it.third) proto_name_node = name_it.second;

                    warnf("No implementation of protocol %s for type %s", 
                          (proto_name_node != NIL_NODE) ? get_node_string(proto_name_node).c_str() : "<unknown>",
                          get_node_string(type_kw).c_str()); // Use only two %s specifiers
                    return NIL_NODE;
                }
                
                hash_map_ptr_t type_impls = get_node_map(type_it.second);
                auto method_it = type_impls->find(method_name, node_eq);
                if (!method_it.third) {
                    warnf("No implementation of method %s for type %s in protocol %s", 
                          get_node_string(method_name).c_str(),
                          get_node_string(type_kw).c_str(),
                          get_node_string(proto_map->find(new_node_keyword("name"), node_eq).second).c_str());
                    return NIL_NODE;
                }
                
                node_idx_t method_impl = method_it.second;
                
                // Construct the call arguments: (method-implementation target arg1 arg2 ...)
                list_ptr_t call_args = new_list();
                call_args = call_args->push_back(method_impl);
                call_args = call_args->push_back(target); // Pass the original target
                
                // Add any additional arguments from the original call
                list_t::iterator args_it(args);
                args_it++; // Skip the target/this argument in the original call
                while (args_it) {
                    call_args = call_args->push_back(*args_it++);
                }
                
                // Evaluate the implementation call in the current environment
                return eval_list(env, call_args);
            }, false));
    }
    
    return proto_name;
}

// Extend a protocol with implementations for one or more types
// (extend-protocol Protocol
//   Type1
//     (method1 [this arg] impl1)
//     (method2 [this] impl2)
//   Type2
//     (method1 [this arg] impl3))
static node_idx_t native_extend_protocol(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    
    // Get protocol
    if (!it) {
        warnf("extend-protocol: expected protocol and at least one type implementation");
        return NIL_NODE;
    }
    
    node_idx_t proto_idx = *it++;
    
    // If it's a symbol, look it up in the environment
    if (get_node_type(proto_idx) == NODE_SYMBOL) {
        proto_idx = env->get(proto_idx);
    }
    
    // Make sure it's a protocol
    if (!is_protocol(proto_idx)) {
        warnf("extend-protocol: first argument must be a protocol");
        return NIL_NODE;
    }
    
    hash_map_ptr_t proto_map = get_node_map(proto_idx);
    
    // Get the protocol's implementations map
    auto impls_it = proto_map->find(new_node_keyword("impls"), node_eq);
    if (!impls_it.third) {
        warnf("extend-protocol: invalid protocol structure");
        return NIL_NODE;
    }
    
    hash_map_ptr_t impls_map = get_node_map(impls_it.second);
    
    // Get the protocol's methods map
    auto methods_it = proto_map->find(new_node_keyword("methods"), node_eq);
    if (!methods_it.third) {
        warnf("extend-protocol: invalid protocol structure (no methods)");
        return NIL_NODE;
    }
    
    hash_map_ptr_t methods_map = get_node_map(methods_it.second);
    
    // Process each type implementation
    while (it) {
        // Get the type to extend
        if (!it) break;
        node_idx_t type_idx = *it++;
        
        // Convert type if needed (symbol to keyword)
        node_idx_t type_key = type_idx;
        if (get_node_type(type_idx) == NODE_SYMBOL) {
            type_key = new_node_keyword(get_node_string(type_idx));
        }
        
        // Create a map to hold implementations for this type
        hash_map_ptr_t type_impls;
        auto existing_it = impls_map->find(type_key, node_eq);
        if (existing_it.third) {
            // Create a new map and copy existing entries
            type_impls = new_hash_map();
            hash_map_ptr_t existing_map = get_node_map(existing_it.second);
            for (auto entry = existing_map->begin(); entry; ++entry) {
                type_impls->assoc_inplace(entry->first, entry->second, node_eq);
            }
        } else {
            type_impls = new_hash_map();
        }
        
        // Process each method implementation
        while (it && get_node_type(*it) == NODE_LIST) {
            list_ptr_t method_list = get_node_list(*it++);
            list_t::iterator method_it(method_list);
            
            // Get method name
            if (!method_it || get_node_type(*method_it) != NODE_SYMBOL) {
                warnf("extend-protocol: expected method name symbol");
                continue;
            }
            
            node_idx_t method_name = *method_it++;
            
            // Verify method is part of protocol
            if (!methods_map->contains(method_name, node_eq)) {
                warnf("extend-protocol: method %s not defined in protocol", 
                      get_node_string(method_name).c_str());
                continue;
            }
            
            // Get argument vector
            if (!method_it || get_node_type(*method_it) != NODE_VECTOR) {
                warnf("extend-protocol: expected method argument vector");
                continue;
            }
            
            node_idx_t arg_vector = *method_it++;
            
            // Get function body
            node_idx_t method_body = NIL_NODE;
            while (method_it) {
                node_idx_t expr = *method_it++;
                if (method_it) {
                    // Not the last expression, so evaluate it in sequence
                    if (method_body == NIL_NODE) {
                        list_ptr_t body_list = new_list();
                        body_list = body_list->push_back(expr);
                        method_body = new_node_list(body_list);
                    } else {
                        list_ptr_t body_list = get_node_list(method_body);
                        body_list = body_list->push_back(expr);
                        method_body = new_node_list(body_list);
                    }
                } else {
                    // Last expression, the return value
                    if (method_body == NIL_NODE) {
                        method_body = expr;
                    } else {
                        list_ptr_t body_list = get_node_list(method_body);
                        body_list = body_list->push_back(expr);
                        method_body = new_node_list(body_list);
                    }
                }
            }
            
            // Create the function for the method implementation
            list_ptr_t fn_args = new_list();
            fn_args = fn_args->push_back(arg_vector);
            
            // Force wrapping the method body in a (do ...) block
            // even if it's already a list, to ensure proper sequential evaluation.
            list_ptr_t body_list = new_list();
            body_list = body_list->push_back(env->get("do")); // Use env->get("do") for safety
            if (get_node_type(method_body) == NODE_LIST) {
                // If body is already a list like ((expr1) (expr2)), prepend do
                body_list = body_list->conj(*get_node_list(method_body));
            } else {
                // If body is a single expression, add it after do
                body_list = body_list->push_back(method_body);
            }
            method_body = new_node_list(body_list);

            fn_args = fn_args->push_back(method_body);
            node_idx_t fn = native_fn(env, fn_args);
            
            // Add to type implementations
            type_impls->assoc_inplace(method_name, fn, node_eq);
        }
        
        // Update protocol's implementations for this type
        impls_map->assoc_inplace(type_key, new_node_hash_map(type_impls), node_eq);
    }
    
    // Update the protocol with the new implementations
    proto_map->assoc_inplace(new_node_keyword("impls"), new_node_hash_map(impls_map), node_eq);
    
    return proto_idx;
}

// Extend one or more protocols for a single type
// (extend-type Type
//   Protocol1
//     (method1 [this arg] impl1)
//     (method2 [this] impl2)
//   Protocol2
//     (method3 [this arg] impl3))
static node_idx_t native_extend_type(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    
    // Get the type to extend
    if (!it) {
        warnf("extend-type: expected type and at least one protocol implementation");
        return NIL_NODE;
    }
    
    node_idx_t type_idx = *it++;
    
    // Convert type if needed (symbol to keyword)
    node_idx_t type_key = type_idx;
    if (get_node_type(type_idx) == NODE_SYMBOL) {
        type_key = new_node_keyword(get_node_string(type_idx));
    }
    
    node_idx_t result = type_idx;
    
    // Process each protocol
    while (it) {
        // Get the protocol
        if (!it) break;
        node_idx_t proto_idx = *it++;
        
        // If it's a symbol, look it up in the environment
        if (get_node_type(proto_idx) == NODE_SYMBOL) {
            proto_idx = env->get(proto_idx);
        }
        
        // Make sure it's a protocol
        if (!is_protocol(proto_idx)) {
            warnf("extend-type: %s is not a protocol", get_node_string(proto_idx).c_str());
            continue;
        }
        
        // Create args for extend-protocol call
        list_ptr_t extend_args = new_list();
        extend_args = extend_args->push_back(proto_idx);
        extend_args = extend_args->push_back(type_key);
        
        // Add method implementations
        while (it && get_node_type(*it) == NODE_LIST) {
            extend_args = extend_args->push_back(*it++);
        }
        
        // Call extend-protocol
        native_extend_protocol(env, extend_args);
    }
    
    return result;
}

// Check if a value satisfies a protocol
// (satisfies? Protocol value)
static node_idx_t native_satisfies(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    
    // Get protocol
    if (!it) {
        warnf("satisfies?: expected protocol and value");
        return FALSE_NODE;
    }
    
    node_idx_t proto_idx = *it++;
    
    // If it's a symbol, look it up in the environment
    if (get_node_type(proto_idx) == NODE_SYMBOL) {
        proto_idx = env->get(proto_idx);
    }
    
    // Make sure it's a protocol
    if (!is_protocol(proto_idx)) {
        warnf("satisfies?: first argument must be a protocol");
        return FALSE_NODE;
    }
    
    // Get value to check
    if (!it) {
        warnf("satisfies?: missing value argument");
        return FALSE_NODE;
    }
    
    node_idx_t value_idx = *it++;
    
    // Get the type of the value
    node_idx_t type_key = get_type_keyword(value_idx);
    
    // Check if the protocol has an implementation for this type
    hash_map_ptr_t proto_map = get_node_map(proto_idx);
    auto impls_it = proto_map->find(new_node_keyword("impls"), node_eq);
    if (!impls_it.third) {
        return FALSE_NODE;
    }
    
    hash_map_ptr_t impls_map = get_node_map(impls_it.second);
    return impls_map->contains(type_key, node_eq) ? TRUE_NODE : FALSE_NODE;
}

// Check if a value is a protocol
// (protocol? value)
static node_idx_t native_is_protocol(env_ptr_t env, list_ptr_t args) {
    node_idx_t value = args->first_value();
    return is_protocol(value) ? TRUE_NODE : FALSE_NODE;
}

// Initialize protocol functionality
void jo_clojure_protocol_init(env_ptr_t env) {
    // Register protocol-related functions
    env->set("defprotocol", new_node_native_function("defprotocol", &native_defprotocol, true));
    env->set("extend-protocol", new_node_native_function("extend-protocol", &native_extend_protocol, true));
    env->set("extend-type", new_node_native_function("extend-type", &native_extend_type, true));
    env->set("satisfies?", new_node_native_function("satisfies?", &native_satisfies, false));
    env->set("protocol?", new_node_native_function("protocol?", &native_is_protocol, false));
} 