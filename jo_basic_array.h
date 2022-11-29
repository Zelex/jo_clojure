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
            for(int i=0; i<size; i++) {
                array->data->assoc(i, init);
            }
        }
    }
    return new_node_array(array);
}

// byte-array
// char-array
// short-array
// int-array
// long-array
// double-array
// float-array

void jo_basic_array_init(env_ptr_t env) {
	env->set("boolean-array", new_node_native_function("boolean-array", &native_boolean_array, false, NODE_FLAG_PRERESOLVE));
}
