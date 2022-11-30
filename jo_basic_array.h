#pragma once

typedef jo_persistent_vector<unsigned char> array_data_t;
template<> array_data_t::vector_node_alloc_t array_data_t::alloc_node = array_data_t::vector_node_alloc_t();
template<> array_data_t::vector_alloc_t array_data_t::alloc = array_data_t::vector_alloc_t();
typedef jo_shared_ptr_t<array_data_t> array_data_ptr_t;
template<typename...A> jo_shared_ptr_t<array_data_t> new_array_data(A... args) { return jo_shared_ptr_t<array_data_t>(array_data_t::alloc.emplace(args...)); }

enum array_type_t {
    TYPE_BOOL = 0,
    TYPE_BYTE = 1,
    TYPE_CHAR = 2,
    TYPE_SHORT = 3,
    TYPE_INT = 4,
    TYPE_LONG = 5,
    TYPE_FLOAT = 6,
    TYPE_DOUBLE = 7,
};

// simple wrapper so we can blop it into the t_object generic container
struct jo_basic_array_t : jo_object {
    long long num_elements;
    int element_size;
    array_type_t type;
    array_data_ptr_t data;

    jo_basic_array_t(long long num, int size, array_type_t t) {
        num_elements = num;
        element_size = size;
        type = t;
        data = new_array_data(num_elements*element_size);
    }
};

typedef jo_alloc_t<jo_basic_array_t> jo_basic_array_alloc_t;
jo_basic_array_alloc_t jo_basic_array_alloc;
typedef jo_shared_ptr_t<jo_basic_array_t> jo_basic_array_ptr_t;
template<typename...A>
jo_basic_array_ptr_t new_array(A...args) { return jo_basic_array_ptr_t(jo_basic_array_alloc.emplace(args...)); }

//typedef jo_shared_ptr<jo_basic_array_t> jo_basic_array_ptr_t;
//static jo_basic_array_ptr_t new_array() { return jo_basic_array_ptr_t(new jo_basic_array_t()); }
static node_idx_t new_node_array(jo_basic_array_ptr_t nodes, int flags=0) { return new_node_object(NODE_ARRAY, nodes.cast<jo_object>(), flags); }

// boolean-array -- really a byte array in disguise
static node_idx_t native_boolean_array(env_ptr_t env, list_ptr_t args) {
    node_idx_t size_or_seq_idx = args->first_value();
    node_idx_t seq_idx = args->second_value();
    node_t *size_or_seq = get_node(size_or_seq_idx);
    if(size_or_seq->is_seq()) {
        long long size = size_or_seq->seq_size();
        jo_basic_array_ptr_t array = new_array(size, 1, TYPE_BOOL);
        long long i = 0;
        seq_iterate(size_or_seq_idx, [&](node_idx_t idx) {
            node_t *n = get_node(idx);
            array->data->assoc(i++, n->as_bool());
            return true;
        });
        return new_node_array(array);
    }
    long long size = size_or_seq->as_int();
    jo_basic_array_ptr_t array = new_array(size, 1, TYPE_BOOL);
    if(seq_idx != NIL_NODE) {
        node_t *init_or_seq = get_node(seq_idx);
        if(init_or_seq->is_seq()) {
            long long i = 0;
            seq_iterate(seq_idx, [&](node_idx_t idx) {
                node_t *n = get_node(idx);
                array->data->assoc(i++, n->as_bool());
                return true;
            });
        } else {
            bool init = init_or_seq->as_bool();
            for(int i=0; i<size; ) {
                array->data->assoc(i++, init);
            }
        }
    }
    return new_node_array(array);
}

// byte-array
static node_idx_t native_byte_array(env_ptr_t env, list_ptr_t args) {
    node_idx_t size_or_seq_idx = args->first_value();
    node_idx_t seq_idx = args->second_value();
    node_t *size_or_seq = get_node(size_or_seq_idx);
    if(size_or_seq->is_seq()) {
        long long size = size_or_seq->seq_size();
        jo_basic_array_ptr_t array = new_array(size, 1, TYPE_BYTE);
        long long i = 0;
        seq_iterate(size_or_seq_idx, [&](node_idx_t idx) {
            node_t *n = get_node(idx);
            array->data->assoc(i++, n->as_int() & 0xFF);
            return true;
        });
        return new_node_array(array);
    }
    long long size = size_or_seq->as_int();
    jo_basic_array_ptr_t array = new_array(size, 1, TYPE_BYTE);
    if(seq_idx != NIL_NODE) {
        node_t *init_or_seq = get_node(seq_idx);
        if(init_or_seq->is_seq()) {
            long long i = 0;
            seq_iterate(seq_idx, [&](node_idx_t idx) {
                node_t *n = get_node(idx);
                array->data->assoc(i++, n->as_int() & 0xFF);
                return true;
            });
        } else {
            unsigned char init = init_or_seq->as_int() & 0xFF;
            for(int i=0; i<size; ) {
                array->data->assoc(i++, init);
            }
        }
    }
    return new_node_array(array);
}

// char-array
static node_idx_t native_char_array(env_ptr_t env, list_ptr_t args) {
    node_idx_t size_or_seq_idx = args->first_value();
    node_idx_t seq_idx = args->second_value();
    node_t *size_or_seq = get_node(size_or_seq_idx);
    if(size_or_seq->is_seq()) {
        long long size = size_or_seq->seq_size();
        jo_basic_array_ptr_t array = new_array(size, 1, TYPE_CHAR);
        long long i = 0;
        seq_iterate(size_or_seq_idx, [&](node_idx_t idx) {
            node_t *n = get_node(idx);
            array->data->assoc(i++, n->as_int() & 0xFF);
            return true;
        });
        return new_node_array(array);
    }
    long long size = size_or_seq->as_int();
    jo_basic_array_ptr_t array = new_array(size, 1, TYPE_CHAR);
    if(seq_idx != NIL_NODE) {
        node_t *init_or_seq = get_node(seq_idx);
        if(init_or_seq->is_seq()) {
            long long i = 0;
            seq_iterate(seq_idx, [&](node_idx_t idx) {
                node_t *n = get_node(idx);
                array->data->assoc(i++, n->as_int() & 0xFF);
                return true;
            });
        } else {
            unsigned char init = init_or_seq->as_int() & 0xFF;
            for(int i=0; i<size; ) {
                array->data->assoc(i++, init);
            }
        }
    }
    return new_node_array(array);
}

// short-array
static node_idx_t native_short_array(env_ptr_t env, list_ptr_t args) {
    node_idx_t size_or_seq_idx = args->first_value();
    node_idx_t seq_idx = args->second_value();
    node_t *size_or_seq = get_node(size_or_seq_idx);
    if(size_or_seq->is_seq()) {
        long long size = size_or_seq->seq_size();
        jo_basic_array_ptr_t array = new_array(size, 2, TYPE_SHORT);
        long long i = 0;
        seq_iterate(size_or_seq_idx, [&](node_idx_t idx) {
            node_t *n = get_node(idx);
            int v = n->as_int();
            array->data->assoc(i++, v & 0xFF);
            array->data->assoc(i++, (v >> 8) & 0xFF);
            return true;
        });
        return new_node_array(array);
    }
    long long size = size_or_seq->as_int();
    jo_basic_array_ptr_t array = new_array(size, 2, TYPE_SHORT);
    if(seq_idx != NIL_NODE) {
        node_t *init_or_seq = get_node(seq_idx);
        if(init_or_seq->is_seq()) {
            long long i = 0;
            seq_iterate(seq_idx, [&](node_idx_t idx) {
                node_t *n = get_node(idx);
                int v = n->as_int();
                array->data->assoc(i++, v & 0xFF);
                array->data->assoc(i++, (v >> 8) & 0xFF);
                return true;
            });
        } else {
            int v = init_or_seq->as_int();
            unsigned char v0 = v & 0xFF;
            unsigned char v1 = (v >> 8) & 0xFF;
            for(int i=0; i<size*2; ) {
                array->data->assoc(i++, v0);
                array->data->assoc(i++, v1);
            }
        }
    }
    return new_node_array(array);
}

// int-array
static node_idx_t native_int_array(env_ptr_t env, list_ptr_t args) {
    node_idx_t size_or_seq_idx = args->first_value();
    node_idx_t seq_idx = args->second_value();
    node_t *size_or_seq = get_node(size_or_seq_idx);
    if(size_or_seq->is_seq()) {
        long long size = size_or_seq->seq_size();
        jo_basic_array_ptr_t array = new_array(size, 4, TYPE_INT);
        long long i = 0;
        seq_iterate(size_or_seq_idx, [&](node_idx_t idx) {
            node_t *n = get_node(idx);
            int v = n->as_int();
            array->data->assoc(i++, v & 0xFF);
            array->data->assoc(i++, (v >> 8) & 0xFF);
            array->data->assoc(i++, (v >> 16) & 0xFF);
            array->data->assoc(i++, (v >> 24) & 0xFF);
            return true;
        });
        return new_node_array(array);
    }
    long long size = size_or_seq->as_int();
    jo_basic_array_ptr_t array = new_array(size, 4, TYPE_INT);
    if(seq_idx != NIL_NODE) {
        node_t *init_or_seq = get_node(seq_idx);
        if(init_or_seq->is_seq()) {
            long long i = 0;
            seq_iterate(seq_idx, [&](node_idx_t idx) {
                node_t *n = get_node(idx);
                int v = n->as_int();
                array->data->assoc(i++, v & 0xFF);
                array->data->assoc(i++, (v >> 8) & 0xFF);
                array->data->assoc(i++, (v >> 16) & 0xFF);
                array->data->assoc(i++, (v >> 24) & 0xFF);
                return true;
            });
        } else {
            int v = init_or_seq->as_int();
            unsigned char v0 = v & 0xFF;
            unsigned char v1 = (v >> 8) & 0xFF;
            unsigned char v2 = (v >> 16) & 0xFF;
            unsigned char v3 = (v >> 24) & 0xFF;
            for(int i=0; i<size*4; ) {
                array->data->assoc(i++, v0);
                array->data->assoc(i++, v1);
                array->data->assoc(i++, v2);
                array->data->assoc(i++, v3);
            }
        }
    }
    return new_node_array(array);
}

// long-array
static node_idx_t native_long_array(env_ptr_t env, list_ptr_t args) {
    node_idx_t size_or_seq_idx = args->first_value();
    node_idx_t seq_idx = args->second_value();
    node_t *size_or_seq = get_node(size_or_seq_idx);
    if(size_or_seq->is_seq()) {
        long long size = size_or_seq->seq_size();
        jo_basic_array_ptr_t array = new_array(size, 8, TYPE_LONG);
        long long i = 0;
        seq_iterate(size_or_seq_idx, [&](node_idx_t idx) {
            node_t *n = get_node(idx);
            long long v = n->as_int();
            array->data->assoc(i++, v & 0xFF);
            array->data->assoc(i++, (v >> 8) & 0xFF);
            array->data->assoc(i++, (v >> 16) & 0xFF);
            array->data->assoc(i++, (v >> 24) & 0xFF);
            array->data->assoc(i++, (v >> 32) & 0xFF);
            array->data->assoc(i++, (v >> 40) & 0xFF);
            array->data->assoc(i++, (v >> 48) & 0xFF);
            array->data->assoc(i++, (v >> 56) & 0xFF);
            return true;
        });
        return new_node_array(array);
    }
    long long size = size_or_seq->as_int();
    jo_basic_array_ptr_t array = new_array(size, 8, TYPE_LONG);
    if(seq_idx != NIL_NODE) {
        node_t *init_or_seq = get_node(seq_idx);
        if(init_or_seq->is_seq()) {
            long long i = 0;
            seq_iterate(seq_idx, [&](node_idx_t idx) {
                node_t *n = get_node(idx);
                long long v = n->as_int();
                array->data->assoc(i++, v & 0xFF);
                array->data->assoc(i++, (v >> 8) & 0xFF);
                array->data->assoc(i++, (v >> 16) & 0xFF);
                array->data->assoc(i++, (v >> 24) & 0xFF);
                array->data->assoc(i++, (v >> 32) & 0xFF);
                array->data->assoc(i++, (v >> 40) & 0xFF);
                array->data->assoc(i++, (v >> 48) & 0xFF);
                array->data->assoc(i++, (v >> 56) & 0xFF);
                return true;
            });
        } else {
            long long v = init_or_seq->as_int();
            unsigned char v0 = v & 0xFF;
            unsigned char v1 = (v >> 8) & 0xFF;
            unsigned char v2 = (v >> 16) & 0xFF;
            unsigned char v3 = (v >> 24) & 0xFF;
            unsigned char v4 = (v >> 32) & 0xFF;
            unsigned char v5 = (v >> 40) & 0xFF;
            unsigned char v6 = (v >> 48) & 0xFF;
            unsigned char v7 = (v >> 56) & 0xFF;
            for(int i=0; i<size*8; ) {
                array->data->assoc(i++, v0);
                array->data->assoc(i++, v1);
                array->data->assoc(i++, v2);
                array->data->assoc(i++, v3);
                array->data->assoc(i++, v4);
                array->data->assoc(i++, v5);
                array->data->assoc(i++, v6);
                array->data->assoc(i++, v7);
            }
        }
    }
    return new_node_array(array);
}

// double-array
// float-array

void jo_basic_array_init(env_ptr_t env) {
	env->set("boolean-array", new_node_native_function("boolean-array", &native_boolean_array, false, NODE_FLAG_PRERESOLVE));
	env->set("byte-array", new_node_native_function("byte-array", &native_byte_array, false, NODE_FLAG_PRERESOLVE));
	env->set("char-array", new_node_native_function("char-array", &native_char_array, false, NODE_FLAG_PRERESOLVE));
	env->set("short-array", new_node_native_function("short-array", &native_short_array, false, NODE_FLAG_PRERESOLVE));
	env->set("int-array", new_node_native_function("int-array", &native_int_array, false, NODE_FLAG_PRERESOLVE));
	env->set("long-array", new_node_native_function("long-array", &native_long_array, false, NODE_FLAG_PRERESOLVE));
}
