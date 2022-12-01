#pragma once

typedef jo_persistent_vector<unsigned char> array_data_t;
template<> array_data_t::vector_node_alloc_t array_data_t::alloc_node = array_data_t::vector_node_alloc_t();
template<> array_data_t::vector_alloc_t array_data_t::alloc = array_data_t::vector_alloc_t();
typedef jo_shared_ptr_t<array_data_t> array_data_ptr_t;
template<typename...A> jo_shared_ptr_t<array_data_t> new_array_data(A... args) { return jo_shared_ptr_t<array_data_t>(array_data_t::alloc.emplace(args...)); }

struct jo_basic_array_t;

typedef jo_alloc_t<jo_basic_array_t> jo_basic_array_alloc_t;
jo_basic_array_alloc_t jo_basic_array_alloc;
typedef jo_shared_ptr_t<jo_basic_array_t> jo_basic_array_ptr_t;
template<typename...A>
jo_basic_array_ptr_t new_array(A...args) { return jo_basic_array_ptr_t(jo_basic_array_alloc.emplace(args...)); }

static node_idx_t new_node_array(jo_basic_array_ptr_t nodes, int flags=0) { return new_node_object(NODE_ARRAY, nodes.cast<jo_object>(), flags); }

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

    jo_basic_array_t(jo_basic_array_t const& other) {
        num_elements = other.num_elements;
        element_size = other.element_size;
        type = other.type;
        data = other.data;
    }

    jo_basic_array_t(const unsigned char *s, long long len) {
        num_elements = len;
        element_size = 1;
        type = TYPE_BYTE;
        data = new_array_data(num_elements*element_size);
        poke(0, s, len);
    }

    jo_basic_array_ptr_t clone() const {
        jo_basic_array_ptr_t A = new_array(*this);
        A->data = data->clone();
        return A;
    }

    jo_basic_array_ptr_t shallow_clone() const {
        return new_array(*this);
    }

    inline long long length() { return num_elements; }

    void poke(long long index, const void *value, int num) {
        const unsigned char *v = (const unsigned char *)value;
        for (int i = 0; i < num; i++) {
            data->assoc_inplace(index*element_size+i, v[i]);
        }
    }

    inline void poke_bool(long long index, bool value) { poke(index, &value, 1); }
    inline void poke_byte(long long index, unsigned char value) { poke(index, &value, 1); }
    inline void poke_char(long long index, char value) { poke(index, &value, 1); }
    inline void poke_short(long long index, short value) { poke(index, &value, sizeof(short)); }
    inline void poke_int(long long index, int value) { poke(index, &value, sizeof(int)); }
    inline void poke_long(long long index, long long value) { poke(index, &value, sizeof(long long)); }
    inline void poke_float(long long index, float value) { poke(index, &value, sizeof(float)); }
    inline void poke_double(long long index, double value) { poke(index, &value, sizeof(double)); }

    void poke_node(long long index, node_idx_t node_idx) {
        switch(type) {
            case TYPE_BOOL: poke_bool(index, get_node_bool(node_idx)); break;
            case TYPE_BYTE: poke_byte(index, get_node_int(node_idx)); break;
            case TYPE_CHAR: poke_char(index, get_node_int(node_idx)); break;
            case TYPE_SHORT: poke_short(index, get_node_int(node_idx)); break;
            case TYPE_INT: poke_int(index, get_node_int(node_idx)); break;
            case TYPE_LONG: poke_long(index, get_node_int(node_idx)); break;
            case TYPE_FLOAT: poke_float(index, get_node_float(node_idx)); break;
            case TYPE_DOUBLE: poke_double(index, get_node_float(node_idx)); break;
            default:
                warnf("jo_basic_array_t::poke_node: unknown type %d", type);
                break;
        }
    }

    inline void peek(long long index, void *value, int num) {
        // peek individual bytes and put them together
        unsigned char *v = (unsigned char *)value;
        for (int i = 0; i < num; i++) {
            v[i] = data->nth(index*element_size+i);
        }
    }

    inline bool peek_bool(long long index) { return data->nth(index*element_size); }
    inline unsigned char peek_byte(long long index) { return data->nth(index*element_size); }
    inline char peek_char(long long index) { return data->nth(index*element_size); }
    inline short peek_short(long long index) { short result; peek(index, &result, sizeof(short)); return result; }
    inline int peek_int(long long index) { int result; peek(index, &result, sizeof(int)); return result; }
    inline long long peek_long(long long index) { long long result; peek(index, &result, sizeof(long long)); return result; }
    inline float peek_float(long long index) { float result; peek(index, &result, sizeof(float)); return result; }
    inline double peek_double(long long index) { double result; peek(index, &result, sizeof(double)); return result; }

    // peek_node
    // returns a node_idx_t to a new node containing the value at the given index
    node_idx_t peek_node(long long index) {
        switch(type) {
            case TYPE_BOOL: return new_node_bool(peek_bool(index));
            case TYPE_BYTE: return new_node_int(peek_byte(index));
            case TYPE_CHAR: return new_node_int(peek_char(index));
            case TYPE_SHORT: return new_node_int(peek_short(index));
            case TYPE_INT: return new_node_int(peek_int(index));
            case TYPE_LONG: return new_node_int(peek_long(index));
            case TYPE_FLOAT: return new_node_float(peek_float(index));
            case TYPE_DOUBLE: return new_node_float(peek_double(index));
            default:
                warnf("jo_basic_array_t::peek_node: unknown type %d", type);
                return 0;
        }
    }

    void write(FILE *fp) const {
        // TODO: could be faster than fputc... but I don't care right now and this is simpler.
        for(int i = 0; i < num_elements*element_size; i++) {
            fputc(data->nth(i), fp);
        }
    }
};

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
            for(int i=0; i<size; ++i) {
                array->data->assoc(i, init);
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
            for(int i=0; i<size; ++i) {
                array->data->assoc(i, init);
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
            for(int i=0; i<size; ++i) {
                array->data->assoc(i, init);
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
            array->poke_short(i++, get_node_int(idx));
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
                array->poke_short(i++, get_node_int(idx));
                return true;
            });
        } else {
            long long v = init_or_seq->as_int();
            for(int i=0; i<size; ++i) {
                array->poke_short(i, v);
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
            array->poke_int(i++, get_node_int(idx));
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
                array->poke_int(i++, get_node_int(idx));
                return true;
            });
        } else {
            long long v = init_or_seq->as_int();
            for(int i=0; i<size; ++i) {
                array->poke_int(i, v);
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
            array->poke_long(i++, get_node_int(idx));
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
                array->poke_long(i++, get_node_int(idx));
                return true;
            });
        } else {
            long long v = init_or_seq->as_int();
            for(int i=0; i<size; ++i) {
                array->poke_long(i++, v);
            }
        }
    }
    return new_node_array(array);
}

// float-array
static node_idx_t native_float_array(env_ptr_t env, list_ptr_t args) {
    node_idx_t size_or_seq_idx = args->first_value();
    node_idx_t seq_idx = args->second_value();
    node_t *size_or_seq = get_node(size_or_seq_idx);
    if(size_or_seq->is_seq()) {
        long long size = size_or_seq->seq_size();
        jo_basic_array_ptr_t array = new_array(size, 4, TYPE_FLOAT);
        long long i = 0;
        seq_iterate(size_or_seq_idx, [&](node_idx_t idx) {
            array->poke_float(i++, get_node_float(idx));
            return true;
        });
        return new_node_array(array);
    }
    long long size = size_or_seq->as_int();
    jo_basic_array_ptr_t array = new_array(size, 4, TYPE_FLOAT);
    if(seq_idx != NIL_NODE) {
        node_t *init_or_seq = get_node(seq_idx);
        if(init_or_seq->is_seq()) {
            long long i = 0;
            seq_iterate(seq_idx, [&](node_idx_t idx) {
                array->poke_float(i++, get_node_float(idx));
                return true;
            });
        } else {
            float v = init_or_seq->as_float();
            for(int i=0; i<size; ++i) {
                array->poke_float(i++, v);
            }
        }
    }
    return new_node_array(array);
}

// double-array
static node_idx_t native_double_array(env_ptr_t env, list_ptr_t args) {
    node_idx_t size_or_seq_idx = args->first_value();
    node_idx_t seq_idx = args->second_value();
    node_t *size_or_seq = get_node(size_or_seq_idx);
    if(size_or_seq->is_seq()) {
        long long size = size_or_seq->seq_size();
        jo_basic_array_ptr_t array = new_array(size, 8, TYPE_DOUBLE);
        long long i = 0;
        seq_iterate(size_or_seq_idx, [&](node_idx_t idx) {
            array->poke_double(i++, get_node_float(idx));
            return true;
        });
        return new_node_array(array);
    }
    long long size = size_or_seq->as_int();
    jo_basic_array_ptr_t array = new_array(size, 8, TYPE_DOUBLE);
    if(seq_idx != NIL_NODE) {
        node_t *init_or_seq = get_node(seq_idx);
        if(init_or_seq->is_seq()) {
            long long i = 0;
            seq_iterate(seq_idx, [&](node_idx_t idx) {
                array->poke_double(i++, get_node_float(idx));
                return true;
            });
        } else {
            double v = init_or_seq->as_float();
            for(int i=0; i<size; ++i) {
                array->poke_double(i++, v);
            }
        }
    }
    return new_node_array(array);
}

// (aset array idx val)(aset array idx idx2 & idxv)
// Sets the value at the index/indices. Works on Java arrays of
// reference types. Returns val.
static node_idx_t native_aset(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    node_idx_t val_idx = args->last_value();
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    for(; it.has_next(); ++it) {
        A->poke_node(get_node_int(*it), val_idx);
    }
    return val_idx;
}

// aset-boolean
static node_idx_t native_aset_boolean(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    node_idx_t val_idx = args->last_value();
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    for(; it.has_next(); ++it) {
        A->poke_bool(get_node_int(*it), get_node_bool(val_idx));
    }
    return val_idx;
}

// aset-byte
static node_idx_t native_aset_byte(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    node_idx_t val_idx = args->last_value();
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    for(; it.has_next(); ++it) {
        A->poke_byte(get_node_int(*it), get_node_int(val_idx));
    }
    return val_idx;
}

// aset-char
static node_idx_t native_aset_char(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    node_idx_t val_idx = args->last_value();
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    for(; it.has_next(); ++it) {
        A->poke_char(get_node_int(*it), get_node_int(val_idx));
    }
    return val_idx;
}

// aset-short
static node_idx_t native_aset_short(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    node_idx_t val_idx = args->last_value();
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    for(; it.has_next(); ++it) {
        A->poke_short(get_node_int(*it), get_node_int(val_idx));
    }
    return val_idx;
}

// aset-int
static node_idx_t native_aset_int(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    node_idx_t val_idx = args->last_value();
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    for(; it.has_next(); ++it) {
        A->poke_int(get_node_int(*it), get_node_int(val_idx));
    }
    return val_idx;
}

// aset-long
static node_idx_t native_aset_long(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    node_idx_t val_idx = args->last_value();
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    for(; it.has_next(); ++it) {
        A->poke_long(get_node_int(*it), get_node_int(val_idx));
    }
    return val_idx;
}

// aset-float
static node_idx_t native_aset_float(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    node_idx_t val_idx = args->last_value();
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    for(; it.has_next(); ++it) {
        A->poke_float(get_node_int(*it), get_node_float(val_idx));
    }
    return val_idx;
}

// aset-double
static node_idx_t native_aset_double(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    node_idx_t val_idx = args->last_value();
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    for(; it.has_next(); ++it) {
        A->poke_double(get_node_int(*it), get_node_float(val_idx));
    }
    return val_idx;
}

// (aget array idx)(aget array idx & idxs)
// Returns the value at the index/indices. Works on Java arrays of all
// types.
static node_idx_t native_aget(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    node_idx_t idx = *it++;
    // TODO: multidimensional arrays
    return A->peek_node(get_node_int(idx));
}

static node_idx_t native_alength(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    return new_node_int(A->length());
}

static node_idx_t native_aclone(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    return new_node_array(A->clone());
}

// (amap a idx ret expr)
// Maps an expression across an array a, using an index named idx, and
// return value named ret, initialized to a clone of a, then setting 
// each element of ret to the evaluation of expr, returning the new 
// array ret.
static node_idx_t native_amap(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = eval_node(env, *it++);
    node_idx_t key_idx = *it++; 
    node_idx_t ret_idx = *it++; 
    node_idx_t expr_idx = *it++; 
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    A = A->clone();
    node_idx_t ret_A = new_node_array(A);
	env_ptr_t env2 = new_env(env);
    node_let(env2, ret_idx, ret_A);
    for(long long i = 0; i < A->length(); ++i) {
        node_let(env2, key_idx, new_node_int(i));
        node_idx_t v = eval_node(env2, expr_idx);
        A->poke_node(i, v);
    }
    return ret_A;
}

// (areduce a idx ret init expr)
// Reduces an expression across an array a, using an index named idx,
// and return value named ret, initialized to init, setting ret to the 
// evaluation of expr at each step, returning ret.
static node_idx_t native_areduce(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = eval_node(env, *it++);
    node_idx_t key_idx = *it++; 
    node_idx_t ret_idx = *it++; 
    node_idx_t init_idx = eval_node(env, *it++); 
    node_idx_t expr_idx = *it++; 
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    env_ptr_t env2 = new_env(env);
    node_idx_t ret = init_idx;
    node_let(env2, ret_idx, ret);
    for(long long i = 0; i < A->length(); ++i) {
        node_let(env2, key_idx, new_node_int(i));
        ret = eval_node(env2, expr_idx);
        node_let(env2, ret_idx, ret);
    }
    return ret;
}

static node_idx_t native_booleans(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    if(get_node_type(array_idx) != NODE_ARRAY) {
        warnf("bytes: expected array\n");
        return NIL_NODE;
    }
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    if(A->type != TYPE_BOOL) {
        A = A->shallow_clone();
        A->type = TYPE_BOOL;
        A->num_elements = A->data->size();
        A->element_size = 1;
        return new_node_array(A);
    }
    return array_idx;
}

static node_idx_t native_bytes(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    if(get_node_type(array_idx) != NODE_ARRAY) {
        warnf("bytes: expected array\n");
        return NIL_NODE;
    }
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    if(A->type != TYPE_BYTE) {
        A = A->shallow_clone();
        A->type = TYPE_BYTE;
        A->num_elements = A->data->size();
        A->element_size = 1;
        return new_node_array(A);
    }
    return array_idx;
}

static node_idx_t native_chars(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    if(get_node_type(array_idx) != NODE_ARRAY) {
        warnf("bytes: expected array\n");
        return NIL_NODE;
    }
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    if(A->type != TYPE_CHAR) {
        A = A->shallow_clone();
        A->type = TYPE_CHAR;
        A->num_elements = A->data->size();
        A->element_size = 1;
        return new_node_array(A);
    }
    return array_idx;
}

static node_idx_t native_shorts(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    if(get_node_type(array_idx) != NODE_ARRAY) {
        warnf("bytes: expected array\n");
        return NIL_NODE;
    }
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    if(A->type != TYPE_SHORT) {
        A = A->shallow_clone();
        A->type = TYPE_SHORT;
        A->num_elements = A->data->size();
        A->element_size = 2;
        A->num_elements /= A->element_size;
        return new_node_array(A);
    }
    return array_idx;
}

static node_idx_t native_ints(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    if(get_node_type(array_idx) != NODE_ARRAY) {
        warnf("bytes: expected array\n");
        return NIL_NODE;
    }
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    if(A->type != TYPE_INT) {
        A = A->shallow_clone();
        A->type = TYPE_INT;
        A->num_elements = A->data->size();
        A->element_size = 4;
        A->num_elements /= A->element_size;
        return new_node_array(A);
    }
    return array_idx;
}

static node_idx_t native_longs(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    if(get_node_type(array_idx) != NODE_ARRAY) {
        warnf("bytes: expected array\n");
        return NIL_NODE;
    }
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    if(A->type != TYPE_LONG) {
        A = A->shallow_clone();
        A->type = TYPE_LONG;
        A->num_elements = A->data->size();
        A->element_size = 8;
        A->num_elements /= A->element_size;
        return new_node_array(A);
    }
    return array_idx;
}

static node_idx_t native_floats(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    if(get_node_type(array_idx) != NODE_ARRAY) {
        warnf("bytes: expected array\n");
        return NIL_NODE;
    }
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    if(A->type != TYPE_FLOAT) {
        A = A->shallow_clone();
        A->type = TYPE_FLOAT;
        A->num_elements = A->data->size();
        A->element_size = 4;
        A->num_elements /= A->element_size;
        return new_node_array(A);
    }
    return array_idx;
}

static node_idx_t native_doubles(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t array_idx = *it++;
    if(get_node_type(array_idx) != NODE_ARRAY) {
        warnf("bytes: expected array\n");
        return NIL_NODE;
    }
    jo_basic_array_ptr_t A = get_node(*it++)->t_object.cast<jo_basic_array_t>();
    if(A->type != TYPE_DOUBLE) {
        A = A->shallow_clone();
        A->type = TYPE_DOUBLE;
        A->num_elements = A->data->size();
        A->element_size = 8;
        A->num_elements /= A->element_size;
        return new_node_array(A);
    }
    return array_idx;
}

void jo_basic_array_init(env_ptr_t env) {
	env->set("boolean-array", new_node_native_function("boolean-array", &native_boolean_array, false, NODE_FLAG_PRERESOLVE));
	env->set("byte-array", new_node_native_function("byte-array", &native_byte_array, false, NODE_FLAG_PRERESOLVE));
	env->set("char-array", new_node_native_function("char-array", &native_char_array, false, NODE_FLAG_PRERESOLVE));
	env->set("short-array", new_node_native_function("short-array", &native_short_array, false, NODE_FLAG_PRERESOLVE));
	env->set("int-array", new_node_native_function("int-array", &native_int_array, false, NODE_FLAG_PRERESOLVE));
	env->set("long-array", new_node_native_function("long-array", &native_long_array, false, NODE_FLAG_PRERESOLVE));
	env->set("float-array", new_node_native_function("float-array", &native_float_array, false, NODE_FLAG_PRERESOLVE));
	env->set("double-array", new_node_native_function("double-array", &native_double_array, false, NODE_FLAG_PRERESOLVE));
	env->set("aset", new_node_native_function("aset", &native_aset, false, NODE_FLAG_PRERESOLVE));
	env->set("aset-boolean", new_node_native_function("aset-boolean", &native_aset_boolean, false, NODE_FLAG_PRERESOLVE));
	env->set("aset-byte", new_node_native_function("aset-byte", &native_aset_byte, false, NODE_FLAG_PRERESOLVE));
	env->set("aset-char", new_node_native_function("aset-char", &native_aset_char, false, NODE_FLAG_PRERESOLVE));
	env->set("aset-short", new_node_native_function("aset-short", &native_aset_short, false, NODE_FLAG_PRERESOLVE));
	env->set("aset-int", new_node_native_function("aset-int", &native_aset_int, false, NODE_FLAG_PRERESOLVE));
	env->set("aset-long", new_node_native_function("aset-long", &native_aset_long, false, NODE_FLAG_PRERESOLVE));
	env->set("aset-float", new_node_native_function("aset-float", &native_aset_float, false, NODE_FLAG_PRERESOLVE));
	env->set("aset-double", new_node_native_function("aset-double", &native_aset_double, false, NODE_FLAG_PRERESOLVE));
	env->set("aget", new_node_native_function("aget", &native_aget, false, NODE_FLAG_PRERESOLVE));
	env->set("alength", new_node_native_function("alength", &native_alength, false, NODE_FLAG_PRERESOLVE));
	env->set("aclone", new_node_native_function("aclone", &native_aclone, false, NODE_FLAG_PRERESOLVE));
	env->set("amap", new_node_native_function("amap", &native_amap, true, NODE_FLAG_PRERESOLVE));
	env->set("areduce", new_node_native_function("areduce", &native_areduce, true, NODE_FLAG_PRERESOLVE));
	env->set("booleans", new_node_native_function("booleans", &native_booleans, false, NODE_FLAG_PRERESOLVE));
	env->set("bytes", new_node_native_function("bytes", &native_bytes, false, NODE_FLAG_PRERESOLVE));
	env->set("chars", new_node_native_function("chars", &native_chars, false, NODE_FLAG_PRERESOLVE));
	env->set("shorts", new_node_native_function("shorts", &native_shorts, false, NODE_FLAG_PRERESOLVE));
	env->set("ints", new_node_native_function("ints", &native_ints, false, NODE_FLAG_PRERESOLVE));
	env->set("longs", new_node_native_function("longs", &native_longs, false, NODE_FLAG_PRERESOLVE));
	env->set("floats", new_node_native_function("floats", &native_floats, false, NODE_FLAG_PRERESOLVE));
	env->set("doubles", new_node_native_function("doubles", &native_doubles, false, NODE_FLAG_PRERESOLVE));
}
