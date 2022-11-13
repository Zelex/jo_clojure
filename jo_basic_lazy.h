#pragma once

// TODO: redo all this stuff using native lambda functions.

// (range)
// (range end)
// (range start end)
// (range start end step)
// Returns a lazy seq of nums from start (inclusive) to end
// (exclusive), by step, where start defaults to 0, step to 1, and end to
// infinity. When step is equal to 0, returns an infinite sequence of
// start. When start is equal to end, returns empty list.
static node_idx_t native_range(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	long long end = args->size(), start = 0, step = 1;
	if(end == 0) {
		end = LLONG_MAX; // "infinite" series
	} else if(end == 1) {
		end = get_node(*it++)->as_int();
	} else if(end == 2) {
		start = get_node(*it++)->as_int();
		end = get_node(*it++)->as_int();
	} else if(end == 3) {
		start = get_node(*it++)->as_int();
		end = get_node(*it++)->as_int();
		step = get_node(*it++)->as_int();
	}
	// @ Maybe make 32 configurable
	if(end != INT_MAX && (end - start) / step < 32) {
		list_ptr_t ret = new_list();
		for(long long i = start; i < end; i += step) {
			ret->push_back_inplace(new_node_int(i));
		}
		return new_node_list(ret);
	}
	// constructs a function which returns the next value in the range, and another function
	return new_node_lazy_list(env, new_node_list(list_va(env->get("range-next"), new_node_int(start), new_node_int(step), new_node_int(end))));
}

static node_idx_t native_range_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	long long start = get_node(*it++)->as_int();
	long long step = get_node(*it++)->as_int();
	long long end = get_node(*it++)->as_int();
	if(start >= end) {
		return NIL_NODE;
	}
	return new_node_list(list_va(new_node_int(start), env->get("range-next"), new_node_int(start+step), new_node_int(step), new_node_int(end)));
}

// (repeat x)
// (repeat n x)
// Returns a lazy (infinite!, or length n if supplied) sequence of xs.
static node_idx_t native_repeat(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t x;
	long long n = LLONG_MAX;
	if(args->size() == 1) {
		x = eval_node(env, *it++);
	} else if(args->size() == 2) {
		n = get_node(eval_node(env, *it++))->as_int();
		x = eval_node(env, *it++);
	} else {
		return NIL_NODE;
	}
	// @ Maybe make 32 configurable
	if(n <= 32) {
		list_ptr_t ret = new_list();
		for(long long i = 0; i < n; i++) {
			ret->push_back_inplace(x);
		}
		return new_node_list(ret);
	}
	return new_node_lazy_list(env, new_node_list(list_va(env->get("repeat-next"), x, new_node_int(n))));
}

static node_idx_t native_repeat_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t x = *it++;
	long long n = get_node(*it++)->as_int();
	if(n <= 0) {
		return NIL_NODE;
	}
	return new_node_list(list_va(x, env->get("repeat-next"), x, new_node_int(n-1)));
}

// (concat)
// (concat x) 
// (concat x y) 
// (concat x y & zs)
// Returns a lazy seq representing the concatenation of the elements in the supplied colls.
static node_idx_t native_concat(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) {
		return NIL_NODE;
	}
	return new_node_lazy_list(env, new_node_list(args->push_front(env->get("concat-next"))));
}

static node_idx_t native_concat_next(env_ptr_t env, list_ptr_t args) {
	node_idx_t val = NIL_NODE;
	do {
		if(args->size() == 0) {
			return NIL_NODE;
		}
		node_idx_t nidx = eval_node(env, args->first_value());
		args = args->rest();
		node_t *n = get_node(nidx);
		auto fr = n->seq_first_rest();
		if(fr.third) {
			val = fr.first;
			args->cons_inplace(fr.second);
			break;
		}
	} while(true);
	list_ptr_t ret = new_list();
	ret->push_back_inplace(val);
	ret->push_back_inplace(env->get("concat-next"));
	ret->conj_inplace(*args.ptr);
	return new_node_list(ret);
}

// (iterate f x)
// Returns a lazy seq representing the infinite sequence of x, f(x), f(f(x)), etc.
// f must be free of side-effects
static node_idx_t native_iterate(env_ptr_t env, list_ptr_t args) {
	return new_node_lazy_list(env, new_node_list(list_va(env->get("iterate-next"), args->first_value(), args->second_value())));
}

static node_idx_t native_iterate_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f = *it++;
	node_idx_t x = *it++;
	list_ptr_t ret = new_list();
	ret->push_back_inplace(x);
	ret->push_back_inplace(env->get("iterate-next"));
	ret->push_back_inplace(f);
	ret->push_back_inplace(eval_va(env, f, x));
	return new_node_list(ret);
}

// (map f)
// (map f coll)
// (map f c1 c2)
// (map f c1 c2 c3)
// (map f c1 c2 c3 & colls)
// Returns a lazy sequence consisting of the result of applying f to
// the set of first items of each coll, followed by applying f to the
// set of second items in each coll, until any one of the colls is
// exhausted.  Any remaining items in other colls are ignored. Function
// f should accept number-of-colls arguments. Returns a transducer when
// no collection is provided.
static node_idx_t native_map(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f = *it++;
	list_ptr_t ret = new_list();
	ret->push_back_inplace(env->get("map-next"));
	ret->push_back_inplace(f);
	while(it) {
		ret->push_back_inplace(eval_node(env, *it++));
	}
	return new_node_lazy_list(env, new_node_list(ret));
}

static node_idx_t native_map_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f = *it++;
	// pull off the first element of each list and call f with it
	list_ptr_t next_list = new_list();
	list_ptr_t arg_list = new_list();
	arg_list->push_back_inplace(f);
	next_list->push_back_inplace(env->get("map-next"));
	next_list->push_back_inplace(f);
	for(; it; it++) {
		node_idx_t arg_idx = *it;
		node_t *arg = get_node(arg_idx);
		auto fr = arg->seq_first_rest();
		if(!fr.third) {
			return NIL_NODE;
		}
		arg_list->push_back_inplace(fr.first);
		next_list->push_back_inplace(fr.second);
	}
	// call f with the args
	node_idx_t ret = eval_list(env, arg_list);
	next_list->cons_inplace(ret);
	return new_node_list(next_list);
}

// (map-indexed f)(map-indexed f coll)
// Returns a lazy sequence consisting of the result of applying f to 0
// and the first item of coll, followed by applying f to 1 and the second
// item in coll, etc, until coll is exhausted. Thus function f should
// accept 2 arguments, index and item. Returns a stateful transducer when
// no collection is provided.
static node_idx_t native_map_indexed(env_ptr_t env, list_ptr_t args) {
	list_ptr_t ret = new_list();
	ret->push_back_inplace(env->get("map-indexed-next"));
	ret->push_back_inplace(args->first_value());
	ret->push_back_inplace(args->second_value());
	ret->push_back_inplace(INT_0_NODE);
	return new_node_lazy_list(env, new_node_list(ret));
}

static node_idx_t native_map_indexed_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f_idx = *it++;
	node_idx_t coll_idx = *it++;
	node_idx_t count_idx = *it++;
	long long count = get_node(count_idx)->t_int;
	// pull off the first element of each list and call f with it

	node_t *coll = get_node(coll_idx);
	auto fr = coll->seq_first_rest();
	if(!fr.third) return NIL_NODE;

	list_ptr_t arg_list = new_list();
	arg_list->push_back_inplace(f_idx);
	arg_list->push_back_inplace(count_idx);
	arg_list->push_back_inplace(fr.first);

	list_ptr_t next_list = new_list();
	next_list->push_back_inplace(env->get("map-indexed-next"));
	next_list->push_back_inplace(f_idx);
	next_list->push_back_inplace(fr.second);
	next_list->push_back_inplace(new_node_int(count + 1));

	// call f with the args
	node_idx_t ret = eval_list(env, arg_list);
	next_list->cons_inplace(ret);
	return new_node_list(next_list);
}

// (take n coll)
// Returns a lazy sequence of the first n items in coll, or all items if there are fewer than n.
static node_idx_t native_take(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t n = eval_node(env, *it++);
	node_idx_t coll = eval_node(env, *it++);
	if(get_node_type(coll) == NODE_LIST) {
		// don't do it lazily if not given lazy inputs... thats dumb
		long long N = get_node(n)->as_int();
		list_ptr_t list_list = get_node(coll)->as_list();
		if(list_list->size() <= N) {
			return coll;
		}
		return new_node_list(list_list->take(N));
	}
	if(get_node_type(coll) == NODE_VECTOR) {
		// don't do it lazily if not given lazy inputs... thats dumb
		long long N = get_node(n)->as_int();
		vector_ptr_t list_list = get_node(coll)->as_vector();
		if(list_list->size() <= N) {
			return coll;
		}
		return new_node_vector(list_list->take(N));
	}
	if(get_node_type(coll) == NODE_LAZY_LIST) {
		return new_node_lazy_list(env, new_node_list(list_va(env->get("take-next"), n, coll)));
	}
	return coll;
}

static node_idx_t native_take_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	long long n = get_node(*it++)->as_int();
	if(n <= 0) {
		return NIL_NODE;
	}
	node_idx_t coll = *it++;
	lazy_list_iterator_t lit(coll);
	if(lit.done()) {
		return NIL_NODE;
	}
	return new_node_list(list_va(lit.val, env->get("take-next"), new_node_int(n-1), new_node_lazy_list(lit.env, lit.next_fn())));
}

// (take-nth n) (take-nth n coll)
// Returns a lazy seq of every nth item in coll.  Returns a stateful
// transducer when no collection is provided.
static node_idx_t native_take_nth(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t n = eval_node(env, *it++);
	if(!it) {
		// stateful transducer
		node_idx_t lazy_func_idx = new_node(NODE_LIST, 0);
		get_node(lazy_func_idx)->t_list = new_list();
		get_node(lazy_func_idx)->t_list->push_back_inplace(env->get("take-nth-next"));
		get_node(lazy_func_idx)->t_list->push_back_inplace(n);
		return new_node_lazy_list(env, lazy_func_idx);
	}
	long long N = get_node(n)->as_int();
	node_idx_t coll = eval_node(env, *it++);
	if(get_node_type(coll) == NODE_LIST) {
		// don't do it lazily if not given lazy inputs... thats dumb
		list_ptr_t list_list = get_node(coll)->as_list();
		if(N <= 0) {
			// (take-nth 0 coll) will return an infinite sequence repeating for first item from coll. A negative N is treated the same as 0.
			node_idx_t lazy_func_idx = new_node(NODE_LIST, 0);
			get_node(lazy_func_idx)->t_list = new_list();
			get_node(lazy_func_idx)->t_list->push_back_inplace(env->get("constantly-next"));
			get_node(lazy_func_idx)->t_list->push_back_inplace(list_list->first_value());
			return new_node_lazy_list(env, lazy_func_idx);
		}
		list_ptr_t list = new_list();
		if(list_list->size() <= N) {
			list->push_back_inplace(list_list->first_value());
			return new_node_list(list);
		}
		for(list_t::iterator it(list_list); it; it += N) {
			list->push_back_inplace(*it);
		}
		return new_node_list(list);
	}
	if(get_node_type(coll) == NODE_VECTOR) {
		// don't do it lazily if not given lazy inputs... thats dumb
		vector_ptr_t list_list = get_node(coll)->as_vector();
		if(N <= 0) {
			// (take-nth 0 coll) will return an infinite sequence repeating for first item from coll. A negative N is treated the same as 0.
			node_idx_t lazy_func_idx = new_node(NODE_LIST, 0);
			get_node(lazy_func_idx)->t_list = new_list();
			get_node(lazy_func_idx)->t_list->push_back_inplace(env->get("constantly-next"));
			get_node(lazy_func_idx)->t_list->push_back_inplace(list_list->first_value());
			return new_node_lazy_list(env, lazy_func_idx);
		}
		vector_ptr_t list = new_vector();
		if(list_list->size() <= N) {
			list->push_back_inplace(list_list->first_value());
			return new_node_vector(list);
		}
		for(vector_t::iterator it = list_list->begin(); it; it += N) {
			list->push_back_inplace(*it);
		}
		return new_node_vector(list);
	}
	if(get_node_type(coll) == NODE_LAZY_LIST) {
		if(N <= 0) {
			lazy_list_iterator_t lit(coll);
			// (take-nth 0 coll) will return an infinite sequence repeating for first item from coll. A negative N is treated the same as 0.
			node_idx_t lazy_func_idx = new_node(NODE_LIST, 0);
			get_node(lazy_func_idx)->t_list = new_list();
			get_node(lazy_func_idx)->t_list->push_back_inplace(env->get("constantly-next"));
			get_node(lazy_func_idx)->t_list->push_back_inplace(lit.val);
			return new_node_lazy_list(lit.env, lazy_func_idx);
		}
		node_idx_t lazy_func_idx = new_node(NODE_LIST, 0);
		get_node(lazy_func_idx)->t_list = new_list();
		get_node(lazy_func_idx)->t_list->push_back_inplace(env->get("take-nth-next"));
		get_node(lazy_func_idx)->t_list->push_back_inplace(n);
		get_node(lazy_func_idx)->t_list->push_back_inplace(coll);
		return new_node_lazy_list(env, lazy_func_idx);
	}
	return coll;
}

static node_idx_t native_take_nth_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	long long n = get_node(*it++)->as_int();
	if(n <= 0) return NIL_NODE;
	node_idx_t coll = *it++;
	lazy_list_iterator_t lit(coll);
	if(lit.done()) return NIL_NODE;
	return new_node_list(list_va(lit.val, env->get("take-nth-next"), new_node_int(n), new_node_lazy_list(lit.env, lit.next_fn(n))));
}

static node_idx_t native_constantly_next(env_ptr_t env, list_ptr_t args) {
	node_idx_t value = args->first_value();
	return new_node_list(list_va(value, env->get("constantly-next"), value));
}

// (take-last n coll)
// Returns a seq of the last n items in coll.  Depending on the type
// of coll may be no better than linear time.  For vectors, see also subvec.
static node_idx_t native_take_last(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t n = eval_node(env, *it++);
	node_idx_t coll = eval_node(env, *it++);
	long long N = get_node(n)->as_int();
	if(get_node_type(coll) == NODE_LIST) {
		list_ptr_t list_list = get_node(coll)->as_list();
		if(list_list->size() <= N) {
			return coll;
		}
		return new_node_list(list_list->take_last(N));
	}
	if(get_node_type(coll) == NODE_VECTOR) {
		vector_ptr_t list_list = get_node(coll)->as_vector();
		if(list_list->size() <= N) {
			return coll;
		}
		return new_node_vector(list_list->take_last(N));
	}
	if(get_node_type(coll) == NODE_LAZY_LIST) {
		lazy_list_iterator_t lit(coll);
		list_ptr_t ret = new_list();
		for(; lit; lit.next()) {
			node_idx_t value_idx = eval_node(env, lit.val);
			node_t *value = get_node(value_idx);
			ret->push_back_inplace(value_idx);
			if(ret->size() > N) {
				ret->pop_front_inplace();
			}
		}
		return new_node_list(ret);
	}
	return coll;
}

// (distinct)
// (distinct coll)
// Returns a lazy sequence of the elements of coll with duplicates removed.
// Returns a stateful transducer when no collection is provided.
static node_idx_t native_distinct(env_ptr_t env, list_ptr_t args) {
	// TODO: lazy? How is that a good idea for distinct? I think it's a bad idea.
	list_t::iterator it(args);
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		list_ptr_t ret = new_list();
		for(list_t::iterator it(list_list); it; it++) {
			node_idx_t value_idx = eval_node(env, *it);
			node_t *value = get_node(value_idx);
			if(!ret->contains([env,value_idx](node_idx_t idx) {
				return node_eq(idx, value_idx);
			})) {
				ret->push_back_inplace(value_idx);
			}
		}
		return new_node_list(ret, NODE_FLAG_LITERAL);
	}
	if(node->is_vector()) {
		vector_ptr_t vector_list = node->as_vector();
		vector_ptr_t ret = new_vector();
		for(vector_t::iterator it = vector_list->begin(); it; it++) {
			node_idx_t value_idx = eval_node(env, *it);
			node_t *value = get_node(value_idx);
			if(!ret->contains([env,value_idx](node_idx_t idx) {
				return node_eq(idx, value_idx);
			})) {
				ret->push_back_inplace(value_idx);
			}
		}
		return new_node_vector(ret, NODE_FLAG_LITERAL);
	}
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(node_idx);
		list_ptr_t ret = new_list();
		for(; lit; lit.next()) {
			node_idx_t value_idx = eval_node(env, lit.val);
			node_t *value = get_node(value_idx);
			if(!ret->contains([env,value_idx](node_idx_t idx) {
				return node_eq(idx, value_idx);
			})) {
				ret->push_back_inplace(value_idx);
			}
		}
		return new_node_list(ret);
	}
	return NIL_NODE;
}

// (filter pred)(filter pred coll)
// Returns a lazy sequence of the items in coll for which
// (pred item) returns logical true. pred must be free of side-effects.
// Returns a transducer when no collection is provided.
static node_idx_t native_filter(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t pred_idx = eval_node(env, *it++);
	node_idx_t coll_idx = eval_node(env, *it++);
	//print_node(coll_idx);
	if(get_node_type(coll_idx) == NODE_LIST) {
		// don't do it lazily if not given lazy inputs... thats dumb
		list_ptr_t list_list = get_node(coll_idx)->as_list();
		list_ptr_t ret = new_list();
		list_ptr_t args = new_list();
		args->push_back_inplace(pred_idx);
		for(list_t::iterator it(list_list); it; it++) {
			node_idx_t item_idx = *it;//eval_node(env, *it);
			//print_node(item_idx);
			node_idx_t comp = eval_list(env, args->conj(item_idx));
			if(get_node_bool(comp)) {
				ret->push_back_inplace(item_idx);
			}
		}
		return new_node_list(ret);
	}
	if(get_node_type(coll_idx) == NODE_VECTOR) {
		vector_ptr_t vector_list = get_node(coll_idx)->as_vector();
		vector_ptr_t ret = new_vector();
		list_ptr_t args = new_list();
		args->push_back_inplace(pred_idx);
		for(vector_t::iterator it = vector_list->begin(); it; it++) {
			node_idx_t item_idx = *it;//eval_node(env, *it);
			//print_node(item_idx);
			node_idx_t comp = eval_list(env, args->conj(item_idx));
			if(get_node_bool(comp)) {
				ret->push_back_inplace(item_idx);
			}
		}
		return new_node_vector(ret);
	}
	if(get_node_type(coll_idx) == NODE_HASH_MAP) {
		// don't do it lazily if not given lazy inputs... thats dumb
		hash_map_ptr_t list_list = get_node(coll_idx)->as_hash_map();
		hash_map_ptr_t ret = new_hash_map();
		list_ptr_t args = new_list();
		args->push_back_inplace(pred_idx);
		for(hash_map_t::iterator it = list_list->begin(); it; it++) {
			list_ptr_t key_val = new_list();
			key_val->push_back_inplace(it->first);
			key_val->push_back_inplace(it->second);
			node_idx_t comp = eval_list(env, args->conj(new_node_list(key_val)));
			if(get_node_bool(comp)) {
				ret = ret->assoc(it->first, it->second, node_eq);
			}
		}
		return new_node_hash_map(ret);
	}
	if(get_node_type(coll_idx) == NODE_HASH_SET) {
		// don't do it lazily if not given lazy inputs... thats dumb
		hash_set_ptr_t list_list = get_node(coll_idx)->as_hash_set();
		hash_set_ptr_t ret = new_hash_set();
		list_ptr_t args = new_list();
		args->push_back_inplace(pred_idx);
		for(auto it = list_list->begin(); it; it++) {
			node_idx_t comp = eval_list(env, args->conj(it->first));
			if(get_node_bool(comp)) {
				ret = ret->assoc(it->first, node_eq);
			}
		}
		return new_node_hash_set(ret);
	}
	if(get_node_type(coll_idx) == NODE_LAZY_LIST) {
		return new_node_lazy_list(env, new_node_list(list_va(env->get("filter-next"), pred_idx, coll_idx)));
	}
	if(get_node_type(coll_idx) == NODE_STRING) {
		jo_string str = get_node(coll_idx)->t_string;
		jo_string ret;
		list_ptr_t args = new_list();
		args->push_back_inplace(pred_idx);
		size_t str_len = str.length();
		const char *str_ptr = str.c_str();
		for(long long i = 0; i < str_len; i++) {
			node_idx_t item_idx = new_node_int(str_ptr[i], NODE_FLAG_CHAR);
			node_idx_t comp = eval_list(env, args->conj(item_idx));
			if(get_node_bool(comp)) {
				ret += (char)str_ptr[i];
			}
		}
		return new_node_string(ret);
	}
	return NIL_NODE;
}

static node_idx_t native_filter_next(env_ptr_t env, list_ptr_t args) {
	node_idx_t pred_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	list_ptr_t e = new_list();
	e->push_back_inplace(pred_idx);
	for(lazy_list_iterator_t lit(coll_idx); !lit.done(); lit.next()) {
		node_idx_t comp = eval_list(env, e->conj(lit.val));
		if(get_node_bool(comp)) {
			return new_node_list(list_va(lit.val, env->get("filter-next"), pred_idx, new_node_lazy_list(lit.env, lit.next_fn())));
		}
	}
	return NIL_NODE;
}

// (keep f)(keep f coll)
// Returns a lazy sequence of the non-nil results of (f item). Note,
// this means false return values will be included.  f must be free of
// side-effects.  Returns a transducer when no collection is provided.
static node_idx_t native_keep(env_ptr_t env, list_ptr_t args) {
	node_idx_t f_idx = eval_node(env, args->first_value());
	node_idx_t coll_idx = eval_node(env, args->second_value());
	return new_node_lazy_list(env, new_node_list(list_va(env->get("keep-next"), f_idx, coll_idx)));
}

static node_idx_t native_keep_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f_idx = *it++;
	node_idx_t coll_idx = *it++;
	int coll_type = get_node_type(coll_idx);
	list_ptr_t e = new_list();
	e->push_back_inplace(f_idx);
	if(coll_type == NODE_LIST) {
		list_ptr_t list_list = get_node(coll_idx)->t_list;
		while(!list_list->empty()) {
			node_idx_t item_idx = list_list->first_value();
			list_list = list_list->pop_front();
			node_idx_t comp = eval_list(env, e->conj(item_idx));
			if(comp != NIL_NODE) {
				return new_node_list(list_va(comp, env->get("keep-next"), f_idx, new_node_list(list_list)));
			}
		}
	}
	if(coll_type == NODE_VECTOR) {
		vector_ptr_t vec_list = get_node(coll_idx)->as_vector();
		while(!vec_list->empty()) {
			node_idx_t item_idx = vec_list->first_value();
			vec_list = vec_list->pop_front();
			node_idx_t comp = eval_list(env, e->conj(item_idx));
			if(comp != NIL_NODE) {
				return new_node_list(list_va(comp, env->get("keep-next"), f_idx, new_node_vector(vec_list)));
			}
		}
	}
	if(coll_type == NODE_LAZY_LIST) {
		for(lazy_list_iterator_t lit(coll_idx); !lit.done(); lit.next()) {
			node_idx_t item_idx = lit.val;
			node_idx_t comp = eval_list(env, e->conj(item_idx));
			if(comp != NIL_NODE) {
				return new_node_list(list_va(comp, env->get("keep-next"), f_idx, new_node_lazy_list(lit.env, lit.next_fn())));
			}
		}
	}
	return NIL_NODE;
}

// (repeatedly f)(repeatedly n f)
// Takes a function of no args, presumably with side effects, and
// returns an infinite (or length n if supplied) lazy sequence of calls
// to it
static node_idx_t native_repeatedly(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t n_idx, f_idx;
	if(args->size() == 1) {
		n_idx = new_node_int(INT_MAX);
		f_idx = eval_node(env, *it++);
	} else {
		n_idx = eval_node(env, *it++);
		f_idx = eval_node(env, *it++);
	}
	return new_node_lazy_list(env, new_node_list(list_va(env->get("repeatedly-next"), f_idx, n_idx)));
}

static node_idx_t native_repeatedly_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f_idx = *it++;
	node_idx_t n_idx = *it++;
	long long n = get_node_int(n_idx);
	if(n > 0) {
		return new_node_list(list_va(eval_va(env, f_idx), env->get("repeatedly-next"), f_idx, new_node_int(n - 1)));
	}
	return NIL_NODE;
}

// (partition n coll)(partition n step coll)(partition n step pad coll)
// Returns a lazy sequence of lists of n items each, at offsets step
// apart. If step is not supplied, defaults to n, i.e. the partitions
// do not overlap. If a pad collection is supplied, use its elements as
// necessary to complete last partition upto n items. In case there are
// not enough padding elements, return a partition with less than n items.
static node_idx_t native_partition(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t n_idx, step_idx, pad_idx, coll_idx;
	if(args->size() == 1) {
		return *it;
	} else if(args->size() == 2) {
		n_idx = *it++;
		step_idx = n_idx;
		pad_idx = NIL_NODE;
		coll_idx = *it++;
	} else if(args->size() == 3) {
		n_idx = *it++;
		step_idx = *it++;
		pad_idx = NIL_NODE;
		coll_idx = *it++;
	} else {
		n_idx = *it++;
		step_idx = *it++;
		pad_idx = *it++;
		coll_idx = *it++;
	}
	return new_node_lazy_list(env, new_node_list(list_va(env->get("partition-next"), n_idx, step_idx, pad_idx, coll_idx)));
}

static node_idx_t native_partition_all(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t n_idx, step_idx, coll_idx;
	if(args->size() == 1) {
		return *it;
	} else if(args->size() == 2) {
		n_idx = *it++;
		step_idx = n_idx;
		coll_idx = *it++;
	} else if(args->size() == 3) {
		n_idx = *it++;
		step_idx = *it++;
		coll_idx = *it++;
	}
	return new_node_lazy_list(env, new_node_list(list_va(env->get("partition-next"), n_idx, step_idx, K_ALL_NODE, coll_idx)));
}

static node_idx_t native_partition_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t n_idx = *it++;
	node_idx_t step_idx = *it++;
	node_idx_t pad_idx = *it++;
	node_idx_t coll_idx = *it++;
	node_t *coll = get_node(coll_idx);
	long long n = get_node_int(n_idx);
	long long step = get_node_int(step_idx);
	auto t = coll->seq_take(n);
	if(!t.second) {
		return NIL_NODE;
	}
	node_idx_t ret_idx = t.first;
	node_idx_t new_coll = coll->seq_drop(step);
	node_t *ret = get_node(ret_idx);
	size_t ret_size = ret->seq_size();
	if(ret_size < n) {
		if(pad_idx == K_ALL_NODE) {
			// let the last one go without pad...
		} else if(pad_idx != NIL_NODE) {
			long long pad_n = n - ret_size;
			seq_iterate(pad_idx, [&](node_idx_t idx) {
				ret->seq_push_back(idx);
				return --pad_n > 0;
			});
		} else {
			return NIL_NODE;
		}
	}
	return new_node_list(list_va(ret_idx, env->get("partition-next"), n_idx, step_idx, pad_idx, new_coll));
}

// (partition-by f)(partition-by f coll)
// Applies f to each value in coll, splitting it each time f returns a
// new value.  Returns a lazy seq of partitions.  Returns a stateful
// transducer when no collection is provided.
static node_idx_t native_partition_by(env_ptr_t env, list_ptr_t args) {
	node_idx_t f_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	return new_node_lazy_list(env, new_node_list(list_va(env->get("partition-by-next"), f_idx, coll_idx)));
}

static node_idx_t native_partition_by_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f_idx = *it++;
	node_idx_t coll_idx = *it++;
	node_t *coll = get_node(coll_idx);
	auto t = coll->seq_take(1);
	if(!t.second) return NIL_NODE;
	node_idx_t ret_idx = t.first;
	node_t *ret = get_node(ret_idx);
	node_idx_t first_idx = eval_va(env, f_idx, ret->seq_first().first);
	int num_drop = 0;
	seq_iterate(coll_idx, [&](node_idx_t idx) {
		node_idx_t pred_idx = eval_va(env, f_idx, idx);
		if(!node_eq(pred_idx, first_idx)) {
			return false;
		}
		if(num_drop > 0) {
			ret->seq_push_back(idx);
		}
		++num_drop;
		return true;
	});
	return new_node_list(list_va(ret_idx, env->get("partition-by-next"), f_idx, coll->seq_drop(num_drop)));
}

// (interleave)(interleave c1)(interleave c1 c2)(interleave c1 c2 & colls)
// Returns a lazy seq of the first item in each coll, then the second etc.
// (interleave [:a :b :c] [1 2 3]) => (:a 1 :b 2 :c 3)
static node_idx_t native_interleave(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) {
		return NIL_NODE;
	}
	list_ptr_t list = new_list();
	list->push_back_inplace(env->get("interleave-next"));
	list->push_back_inplace(ZERO_NODE);
	list->conj_inplace(*args.ptr);
	return new_node_lazy_list(env, new_node_list(list));

}

static node_idx_t native_interleave_next(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) {
		return NIL_NODE;
	}
	node_idx_t coll_idx = args->first_value();
	if(coll_idx == ZERO_NODE) {
		// check if any of args are done
		list_t::iterator it(args);
		++it;
		for(; it; ++it) {
			node_t *n = get_node(*it);
			int ntype = n->type;
			if(ntype == NODE_NIL) {
				return NIL_NODE;
			} else if(ntype == NODE_LIST) {
				if(n->t_list->size() == 0) {
					return NIL_NODE;
				}
			} else if(ntype == NODE_VECTOR) {
				if(n->as_vector()->size() == 0) {
					return NIL_NODE;
				}
			} else if(ntype == NODE_LAZY_LIST) {
				if(eval_node(env, n->t_extra) == NIL_NODE) {
					return NIL_NODE;
				}
			} else if(ntype == NODE_STRING) {
				if(n->t_string.size() == 0) {
					return NIL_NODE;
				}
			} else {
				return NIL_NODE;
			}
		}
	}
	node_idx_t nidx = args->second_value();
	node_idx_t val = NIL_NODE;
	int ntype = get_node(nidx)->type;
	list_t::iterator it(args);
	args = args->rest(it+2);
	if(ntype == NODE_LIST) {
		list_ptr_t n = get_node(nidx)->t_list;
		val = n->first_value();
		args->cons_inplace(new_node_list(n->pop()));
	} else if(ntype == NODE_VECTOR) {
		vector_ptr_t n = get_node(nidx)->as_vector();
		val = n->first_value();
		args->cons_inplace(new_node_vector(n->pop_front()));
	} else if(ntype == NODE_LAZY_LIST) {
		// call the t_extra, and grab the first element of the return and return that.
		node_idx_t reti = eval_node(env, get_node(nidx)->t_extra);
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			val = list_list->first_value();
			args->cons_inplace(new_node_lazy_list(env, new_node_list(list_list->rest())));
		}
	} else if(ntype == NODE_STRING) {
		// pull off the first character of the string
		jo_string str = get_node(nidx)->t_string;
		val = new_node_string(str.substr(0, 1));
		args->cons_inplace(new_node_string(str.substr(1)));
	}
	list_ptr_t ret = new_list();
	ret->push_back_inplace(val);
	ret->push_back_inplace(env->get("interleave-next"));
	long long next_coll_it = get_node_int(coll_idx)+1;
	if(args->size() == next_coll_it) {
		ret->push_back_inplace(ZERO_NODE);
	} else {
		ret->push_back_inplace(new_node_int(next_coll_it));
	}
	ret->conj_inplace(*args->rest()->clone());
	ret->push_back_inplace(args->first_value());
	return new_node_list(ret);
}

// (flatten x)
// Takes any nested combination of sequential things (lists, vectors,
// etc.) and returns their contents as a single, flat lazy sequence.
// (flatten nil) returns an empty sequence.
static node_idx_t native_flatten(env_ptr_t env, list_ptr_t args) {
	node_idx_t x = args->first_value();
	if(x <= NIL_NODE) {
		return EMPTY_LIST_NODE;
	}
	node_t *n = get_node(x);
	if(!n->is_seq()) {
		return x;
	}
	node_idx_t lazy_func_idx = new_node(NODE_LIST, 0);
	node_t *lazy_func = get_node(lazy_func_idx);
	lazy_func->t_list = new_list();
	lazy_func->t_list->push_front_inplace(get_node(x)->seq_rest().first);
	while(n->is_seq()) {
		x = n->seq_first().first;
		n = get_node(x);
		if(n->is_seq()) {
			lazy_func->t_list->push_front_inplace(get_node(x)->seq_rest().first);
		} else {
			lazy_func->t_list->push_front_inplace(x);
		}
	}
	lazy_func->t_list->push_front_inplace(env->get("flatten-next"));
	return new_node_lazy_list(env, lazy_func_idx);
}

static node_idx_t native_flatten_next(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) {
		return NIL_NODE;
	}

	list_ptr_t ret = new_list();
	node_idx_t ret_value = NIL_NODE;
	
	// first value should always not be a sequence
	ret_value = args->first_value();
	args = args->pop_front();
	assert(!get_node(ret_value)->is_seq());

	node_idx_t seq_idx = args->first_value();
	node_t *seq = get_node(seq_idx);

	ret = ret->push_front(args->clone());

	if(!seq->is_seq()) {
		ret->push_front_inplace(env->get("flatten-next"));
		ret->push_front_inplace(ret_value);
		return new_node_list(ret);
	}

	do {
		ret = ret->pop();
		auto seq_p = seq->seq_first_rest();
		if(!get_node(seq_p.second)->seq_empty()) ret->push_front_inplace(seq_p.second);
		if(seq_p.first != NIL_NODE) ret->push_front_inplace(seq_p.first);
		seq_idx = seq_p.first;
		seq = get_node(seq_idx);
	} while(seq->is_seq());

	ret->push_front_inplace(env->get("flatten-next"));
	ret->push_front_inplace(ret_value);
	return new_node_list(ret);
}

// (lazy-seq & body)
// Takes a body of expressions that returns an ISeq or nil, and yields
// a Seqable object that will invoke the body only the first time seq
// is called, and will cache the result and return it on all subsequent
// seq calls. See also - realized?
static node_idx_t native_lazy_seq(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) {
		return NIL_NODE;
	}
	node_idx_t lazy_func_idx = new_node(NODE_LIST, 0);
	node_t *lazy_func = get_node(lazy_func_idx);
	lazy_func->t_list = args->push_front(new_node_native_function("lazy-seq-first", 
	[=](env_ptr_t sub_env, list_ptr_t args) -> node_idx_t {
		node_idx_t ll_idx = eval_node_list(env, args);
		node_t *ll = get_node(ll_idx);
		if(!ll->is_seq()) {
			return NIL_NODE;
		}
		auto fr = ll->seq_first_rest();
		return new_node_list(list_va(fr.first, env->get("lazy-seq-next"), fr.second));
	}, true));
	return new_node_lazy_list(env, lazy_func_idx);
}

static node_idx_t native_lazy_seq_next(env_ptr_t env, list_ptr_t args) {
	node_idx_t ll_idx = args->first_value();
	node_t *ll = get_node(ll_idx);
	auto fr = ll->seq_first_rest();
	if(!fr.third) return NIL_NODE;
	return new_node_list(list_va(fr.first, env->get("lazy-seq-next"), fr.second));
}

// (seq coll)
// Returns a seq on the collection. If the collection is
// empty, returns nil.  (seq nil) returns nil. seq also works on
// Strings, native Java arrays (of reference types) and any objects
// that implement Iterable. Note that seqs cache values, thus seq
// should not be used on any Iterable whose iterator repeatedly
// returns the same mutable object.
static node_idx_t native_seq(env_ptr_t env, list_ptr_t args) {
	node_idx_t x = args->first_value();
	if(x <= NIL_NODE) {
		return NIL_NODE;
	}
	node_t *n = get_node(x);
	if(!n->is_seq()) {
		return NIL_NODE;
	}
	return new_node_lazy_list(env, new_node_list(list_va(env->get("seq-next"), x)));
}

static node_idx_t native_seq_next(env_ptr_t env, list_ptr_t args) {
	node_idx_t x = args->first_value();
	if(x <= NIL_NODE) {
		return NIL_NODE;
	}
	node_t *n = get_node(x);
	auto fr = n->seq_first_rest();
	if(!fr.third) return NIL_NODE;
	return new_node_list(list_va(fr.first, env->get("seq-next"), fr.second));
}

// (cons x seq)
// Returns a new seq where x is the first element and seq is the rest.
// Note: cons is not actually lazy, but I think this implementation 
//   could benefit from that in the case of cat'ing to a lazy list.
static node_idx_t native_cons(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t first_idx = *it++;
	node_idx_t second_idx = *it++;
	if(first_idx == NIL_NODE && second_idx == NIL_NODE) {
		list_ptr_t ret = new_list();
		ret->cons_inplace(NIL_NODE);
		return new_node_list(ret);
	}
	if(first_idx == NIL_NODE) {
		list_ptr_t ret = new_list();
		ret->cons_inplace(second_idx);
		return new_node_list(ret);
	}
	if(second_idx == NIL_NODE) {
		list_ptr_t ret = new_list();
		ret->cons_inplace(first_idx);
		return new_node_list(ret);
	}
	node_t *first = get_node(first_idx);
	node_t *second = get_node(second_idx);
	if(second->type == NODE_STRING) {
		jo_string s = first->is_int() ? jo_string(first->t_int) : first->as_string();
		jo_string s2 = second->as_string();
		jo_string s3 = s + s2;
		return new_node_string(s3);
	}
	if(second->type == NODE_LIST) {
		list_ptr_t second_list = second->t_list;
		return new_node_list(second_list->cons(first_idx));
	}
	if(second->type == NODE_VECTOR) {
		vector_ptr_t second_vector = second->as_vector();
		return new_node_vector(second_vector->cons(first_idx));
	}
	if(second->type == NODE_LAZY_LIST) {
		list_ptr_t lazy_func_args = new_list();
		lazy_func_args->push_front_inplace(second_idx);
		lazy_func_args->push_front_inplace(first_idx);
		lazy_func_args->push_front_inplace(env->get("cons-first"));
		return new_node_lazy_list(env, new_node_list(lazy_func_args));
	}
	return new_node_list(list_va(first_idx, second_idx));
}

static node_idx_t native_cons_first(env_ptr_t env, list_ptr_t args) {
	return new_node_list(list_va(args->first_value(), env->get("cons-next"), args->second_value()));
}

static node_idx_t native_cons_next(env_ptr_t env, list_ptr_t args) {
	lazy_list_iterator_t lit(args->first_value());
	return lit.cur;
}

// (take-while pred)(take-while pred coll)
// Returns a lazy sequence of the first n items in coll, or all items if there are fewer than n.
static node_idx_t native_take_while(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t pred = eval_node(env, *it++);
	node_idx_t coll = eval_node(env, *it++);
	if(get_node_type(coll) == NODE_LIST) {
		// don't do it lazily if not given lazy inputs... thats dumb
		list_ptr_t coll_list = get_node(coll)->as_list();
		list_ptr_t out_list = new_list();
		for(list_t::iterator it(coll_list); it; ++it) {
			node_idx_t item = *it;
			if(eval_va(env, pred, item) != TRUE_NODE) {
				break;
			}
			out_list->push_back_inplace(item);
		}
		return new_node_list(out_list);
	}
	if(get_node_type(coll) == NODE_VECTOR) {
		// don't do it lazily if not given lazy inputs... thats dumb
		vector_ptr_t coll_vec = get_node(coll)->as_vector();
		vector_ptr_t out_vec = new_vector();
		for(vector_t::iterator it = coll_vec->begin(); it; ++it) {
			node_idx_t item = *it;
			if(eval_va(env, pred, item) != TRUE_NODE) {
				break;
			}
			out_vec->push_back_inplace(item);
		}
		return new_node_vector(out_vec);
	}
	if(get_node_type(coll) == NODE_LAZY_LIST) {
		return new_node_lazy_list(env, new_node_list(list_va(env->get("take-while-next"), pred, coll)));
	}
	return coll;
}

static node_idx_t native_take_while_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t pred = *it++;
	node_idx_t coll = *it++;
	lazy_list_iterator_t lit(coll);
	if(lit.done() || eval_va(env, pred, coll) != TRUE_NODE) {
		return NIL_NODE;
	}
	return new_node_list(list_va(lit.val, env->get("take-while-next"), pred, new_node_lazy_list(lit.env, lit.next_fn())));
}

// (drop-while pred)(drop-while pred coll)
// Returns a lazy sequence of the items in coll starting from the
// first item for which (pred item) returns logical false.  Returns a
// stateful transducer when no collection is provided.
static node_idx_t native_drop_while(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t pred = eval_node(env, *it++);
	node_idx_t coll = eval_node(env, *it++);
	if(get_node_type(coll) == NODE_LIST) {
		// don't do it lazily if not given lazy inputs... thats dumb
		list_ptr_t coll_list = get_node(coll)->as_list();
		list_ptr_t out_list = new_list();
		list_t::iterator it(coll_list);
		for(; it; ++it) {
			if(eval_va(env, pred, *it) != TRUE_NODE) {
				break;
			}
		}
		for(; it; ++it) {
			out_list->push_back_inplace(*it);
		}
		return new_node_list(out_list);
	}
	if(get_node_type(coll) == NODE_VECTOR) {
		// don't do it lazily if not given lazy inputs... thats dumb
		vector_ptr_t coll_vec = get_node(coll)->as_vector();
		vector_ptr_t out_vec = new_vector();
		vector_t::iterator it = coll_vec->begin();
		for(; it; ++it) {
			if(eval_va(env, pred, *it) != TRUE_NODE) {
				break;
			}
		}
		for(; it; ++it) {
			out_vec->push_back_inplace(*it);
		}
		return new_node_vector(out_vec);
	}
	if(get_node_type(coll) == NODE_LAZY_LIST) {
		return new_node_lazy_list(env, new_node_list(list_va(env->get("drop-while-first"), pred, coll)));
	}
	return coll;
}

static node_idx_t native_drop_while_first(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t pred = *it++;
	node_idx_t coll = *it++;
	lazy_list_iterator_t lit(coll);
	for(; !lit.done(); lit.next()) {
		if(eval_va(env, pred, lit.val) != TRUE_NODE) {
			break;
		}
	}
	if(lit.done()) {
		return NIL_NODE;
	}
	return new_node_list(list_va(lit.val, env->get("drop-while-next"), pred, new_node_lazy_list(lit.env, lit.next_fn())));
}

static node_idx_t native_drop_while_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t pred = *it++;
	node_idx_t coll = *it++;
	lazy_list_iterator_t lit(coll);
	if(lit.done()) {
		return NIL_NODE;
	}
	return new_node_list(list_va(lit.val, env->get("drop-while-next"), pred, new_node_lazy_list(lit.env, lit.next_fn())));
}

static node_idx_t native_drop(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t n_idx = *it++;
	long long n = get_node(n_idx)->as_int();
	node_idx_t list_idx = *it++;
	if(n <= 0) {
		return list_idx;
	}
	node_t *list = get_node(list_idx);
	list_ptr_t list_list = list->as_list();
	if(list->is_string()) {
		jo_string &str = list->t_string;
		if(n < 0) {
			n = str.size() + n;
		}
		if(n < 0 || n >= str.size()) {
			return new_node_string("");
		}
		return new_node_string(str.substr(n));
	}
	if(list->is_list()) {
		return new_node_list(list_list->drop(n));
	}
	if(list->is_vector()) {
		return new_node_vector(list->as_vector()->drop(n));
	}
	if(list->is_lazy_list()) {
		return new_node_lazy_list(env, new_node_list(list_va(env->get("drop-first"), n_idx, list_idx)));
	}
	return NIL_NODE;
}

static node_idx_t native_drop_first(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	long long n = get_node_int(*it++);
	node_idx_t coll = *it++;
	lazy_list_iterator_t lit(coll);
	for(; !lit.done(); lit.next()) {
		if(n-- <= 0) {
			break;
		}
	}
	if(lit.done()) return NIL_NODE;
	return new_node_list(list_va(lit.val, env->get("drop-next"), new_node_lazy_list(lit.env, lit.next_fn())));
}

static node_idx_t native_drop_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t coll = *it++;
	lazy_list_iterator_t lit(coll);
	if(lit.done()) return NIL_NODE;
	return new_node_list(list_va(lit.val, env->get("drop-next"), new_node_lazy_list(lit.env, lit.next_fn())));
}

// (cycle coll)
// Returns a lazy (infinite!) sequence of repetitions of the items in coll.
static node_idx_t native_cycle(env_ptr_t env, list_ptr_t args) {
	node_idx_t coll_idx = args->first_value();
	jo_shared_ptr<seq_iterator_t> seq = new seq_iterator_t(coll_idx);
	node_idx_t func_idx = new_node(NODE_NATIVE_FUNC, 0);
	node_t *func = get_node(func_idx);
	func->t_native_function = new native_func_t([seq,func_idx,coll_idx](env_ptr_t env2, list_ptr_t args2) {
		list_ptr_t list = new_list();
		list->push_front_inplace(func_idx);
		list->push_front_inplace(seq->val);
		seq.ptr->next();
		if(seq->done()) {
			*seq.ptr = seq_iterator_t(coll_idx);
		}
		return new_node_list(list);
	});
	list_ptr_t list = new_list();
	list->push_front_inplace(func_idx);
	return new_node_lazy_list(env, new_node_list(list));
}

// (dedupe)(dedupe coll)
// Returns a lazy sequence removing consecutive duplicates in coll.
// Returns a transducer when no collection is provided.
static node_idx_t native_dedupe(env_ptr_t env, list_ptr_t args) {
	node_idx_t coll_idx = args->first_value();
	jo_shared_ptr<seq_iterator_t> seq = new seq_iterator_t(coll_idx);
	node_idx_t func_idx = new_node(NODE_NATIVE_FUNC, 0);
	node_t *func = get_node(func_idx);
	func->t_native_function = new native_func_t([seq,func_idx,coll_idx](env_ptr_t env2, list_ptr_t args2) -> node_idx_t {
		if(seq->done()) {
			return NIL_NODE; 
		}
		node_idx_t val = seq->val;
		list_ptr_t list = new_list();
		list->push_front_inplace(func_idx);
		list->push_front_inplace(val);
		do {
			seq.ptr->next();
			if(seq->done()) {
				if(node_eq(seq->val, val)) {
					return NIL_NODE;
				}
			}
		} while(node_eq(seq->val, val));
		return new_node_list(list);
	});
	list_ptr_t list = new_list();
	list->push_front_inplace(func_idx);
	return new_node_lazy_list(env, new_node_list(list));
}

// (for seq-exprs body-expr)
// List comprehension. Takes a vector of one or more
// binding-form/collection-expr pairs, each followed by zero or more
// modifiers, and yields a lazy sequence of evaluations of expr.
// Collections are iterated in a nested fashion, rightmost fastest,
// and nested coll-exprs can refer to bindings created in prior
// binding-forms.  Supported modifiers are: :let [binding-form expr ...],
// :while test, :when test.
// (take 100 (for [x (range 100000000) y (range 1000000) :while (< y x)] [x y]))
static node_idx_t native_for(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		warnf("(for) requires at least 2 arguments\n");
		return NIL_NODE;
	}

	node_idx_t seq_exprs_idx = args->first_value();
	node_idx_t body_expr_idx = args->second_value();

	node_t *seq_exprs = get_node(seq_exprs_idx);
	if(seq_exprs->type != NODE_VECTOR) {
		warnf("(for) first argument must be a vector\n");
		return NIL_NODE;
	}

	// pre-calculate where the loop points are
	vector_ptr_t PC_list = new_vector();
	vector_ptr_t exprs = seq_exprs->as_vector();
	auto expr_it = exprs->begin();
	for(int PC = 0; expr_it; PC++) {
		node_idx_t expr_idx = *expr_it++;
		if(expr_idx != K_WHILE_NODE && expr_idx != K_WHEN_NODE && expr_idx != K_LET_NODE) {
			PC_list->push_back_inplace(new_node_int(PC));
		}
		expr_it++;
		PC++;
	}

	// for storing the state of the iterators
	node_idx_t state_first_idx = new_node_hash_map(new_hash_map()->assoc(K_PC_NODE, INT_0_NODE, node_eq));
	node_idx_t state_rest_idx = new_node_hash_map(new_hash_map());
	node_idx_t nfn_idx = new_node(NODE_NATIVE_FUNC, NODE_FLAG_MACRO);
	node_t *nfn = get_node(nfn_idx);
	nfn->t_native_function = new native_func_t([nfn_idx,PC_list,seq_exprs_idx,body_expr_idx](env_ptr_t env2, list_ptr_t args2) -> node_idx_t {

		list_t::iterator it(args2);
		node_idx_t state_first_idx = *it++;
		node_idx_t state_rest_idx = *it++;
		hash_map_ptr_t state_first = get_node_map(state_first_idx);
		hash_map_ptr_t state_rest = get_node_map(state_rest_idx);

		int PC = get_node_int(state_first->get(K_PC_NODE, node_eq));

		// Setup the initial sub env with current values
		// TODO: Ideally we can persist this across calls - however there's problems with that
		// in that going backwards gets complicated.
		env_ptr_t E = new_env(env2);
		for(auto it = state_first->begin(); it; ++it) {
			node_let(E, it->first, it->second);
		}

		if(PC > PC_list->size()-1) {
			PC = PC_list->size()-1;
		}

		// Evaluate all exprs (moving PC around while we do it) until we run out of exprs
		vector_ptr_t seq_exprs = get_node_vector(seq_exprs_idx);
		auto expr_it = seq_exprs->begin() + (size_t)get_node_int(PC_list->nth_clamp(PC));
		while(expr_it) {
			node_idx_t binding_idx = *expr_it++;
			node_t *binding = get_node(binding_idx);
			if(binding_idx == K_LET_NODE) {
				node_idx_t bind_list_idx = *expr_it++;
				node_t *bind_list = get_node(bind_list_idx);
				vector_ptr_t bind_list_exprs = get_node_vector(bind_list_idx);
				auto bind_it = bind_list_exprs->begin();
				while(bind_it) {
					node_idx_t binding_idx = *bind_it++;
					node_idx_t val_idx = eval_node(E, *bind_it++);
					node_let(E, binding_idx, val_idx);
					state_first = state_first->assoc(binding_idx, val_idx, node_eq);
				}
			} else if(binding_idx == K_WHILE_NODE) {
				node_idx_t test_idx = eval_node(E, *expr_it++);
				if(test_idx == FALSE_NODE) {
					PC -= 1;
					if(PC < 0) {
						return NIL_NODE;
					}
					expr_it = seq_exprs->begin() + (size_t)get_node_int(PC_list->nth_clamp(PC));
					state_rest = state_rest->dissoc(*expr_it, node_eq);
					PC -= 1;
					if(PC < 0) {
						return NIL_NODE;
					}
					expr_it = seq_exprs->begin() + (size_t)get_node_int(PC_list->nth_clamp(PC));
				}
			} else if(binding_idx == K_WHEN_NODE) {
				node_idx_t test_idx = eval_node(E, *expr_it++);
				if(test_idx == FALSE_NODE) {
					PC -= 1;
					if(PC < 0) PC = 0;
					expr_it = seq_exprs->begin() + (size_t)get_node_int(PC_list->nth_clamp(PC));
				}
			} else {
				node_idx_t val_idx = eval_node(E, *expr_it++);
				node_t *val = get_node(val_idx);
				auto cur = state_rest->find(binding_idx, node_eq);
				if(cur.third) {
					val_idx = cur.second;
					val = get_node(val_idx);
				}
				auto fr = val->seq_first_rest();
				if(!fr.third) {
					// Drop back to the last loop instruction
					PC -= 1;
					if(PC < 0) return NIL_NODE;
					expr_it = seq_exprs->begin() + (size_t)get_node_int(PC_list->nth_clamp(PC));
					state_rest = state_rest->dissoc(binding_idx, node_eq);
				} else {
					node_let(E, binding_idx, fr.first);
					state_first = state_first->assoc(binding_idx, fr.first, node_eq);
					state_rest = state_rest->assoc(binding_idx, fr.second, node_eq);
					PC += 1;
				}
			}
		}

		state_first = state_first->assoc(K_PC_NODE, new_node_int(PC), node_eq);

		// Evaluate the body
		node_idx_t result = eval_node(E, body_expr_idx);

		return new_node_list(list_va(result, nfn_idx, new_node_hash_map(state_first), new_node_hash_map(state_rest)));
	});

	return new_node_lazy_list(env, new_node_list(list_va(nfn_idx, state_first_idx, state_rest_idx)));
}

// (interpose sep)(interpose sep coll)
// Returns a lazy seq of the elements of coll separated by sep.
// Returns a stateful transducer when no collection is provided.
static node_idx_t native_interpose(env_ptr_t env, list_ptr_t args) {
	node_idx_t sep_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	return new_node_lazy_list(env, new_node_list(list_va(env->get("interpose-next-elem"), sep_idx, coll_idx)));
}

static node_idx_t native_interpose_next_elem(env_ptr_t env, list_ptr_t args) {
	node_idx_t sep_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	auto fr = get_node(coll_idx)->seq_first_rest();
	if(!fr.third) return NIL_NODE;
	return new_node_list(list_va(fr.first, env->get("interpose-next-sep"), sep_idx, fr.second));
}

static node_idx_t native_interpose_next_sep(env_ptr_t env, list_ptr_t args) {
	node_idx_t sep_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	if(get_node(coll_idx)->seq_empty()) {
		return NIL_NODE;
	}
	return new_node_list(list_va(sep_idx, env->get("interpose-next-elem"), sep_idx, coll_idx));
}

// (keep-indexed f)(keep-indexed f coll)
// Returns a lazy sequence of the non-nil results of (f index item). Note,
// this means false return values will be included.  f must be free of
// side-effects.  Returns a stateful transducer when no collection is
// provided.
static node_idx_t native_keep_indexed(env_ptr_t env, list_ptr_t args) {
	node_idx_t f_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	return new_node_lazy_list(env, new_node_list(list_va(env->get("keep-indexed-next"), f_idx, coll_idx, INT_0_NODE)));
}

static node_idx_t native_keep_indexed_next(env_ptr_t env, list_ptr_t args) {
	node_idx_t f_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	long long cnt = get_node_int(args->third_value());
	node_idx_t result_idx;
	do {
		node_t *coll = get_node(coll_idx);
		auto fr = coll->seq_first_rest();
		if(!fr.third) return NIL_NODE;
		result_idx = eval_va(env, f_idx, new_node_int(cnt), fr.first);
		coll_idx = fr.second;
		++cnt;
	} while(result_idx == NIL_NODE);
	return new_node_list(list_va(result_idx, env->get("keep-indexed-next"), f_idx, coll_idx, new_node_int(cnt)));
}

// (lazy-cat & colls)
// Expands to code which yields a lazy sequence of the concatenation
// of the supplied colls.  Each coll expr is not evaluated until it is
// needed. 
// (lazy-cat xs ys zs) === (concat (lazy-seq xs) (lazy-seq ys) (lazy-seq zs))
static node_idx_t native_lazy_cat(env_ptr_t env, list_ptr_t args) {
	list_ptr_t lzseqs = new_list();
	for(list_t::iterator it(args); it; ++it) {
		node_idx_t lzseq = native_lazy_seq(env, list_va(*it));
		lzseqs->push_back_inplace(lzseq);
	}
	return native_concat(env, lzseqs);
}

// (mapcat f)(mapcat f & colls)
// Returns the result of applying concat to the result of applying map
// to f and colls.  Thus function f should return a collection. Returns
// a transducer when no collections are provided
static node_idx_t native_mapcat(env_ptr_t env, list_ptr_t args) {
	node_idx_t map = native_map(env, args);
	return new_node_lazy_list(env, new_node_list(list_va(env->get("mapcat-next"), map, NIL_NODE)));
}

static node_idx_t native_mapcat_next(env_ptr_t env, list_ptr_t args) {
	node_idx_t map_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	node_t *coll = get_node(coll_idx);
	while(coll->seq_empty()) {
		// advance map
		node_t *map = get_node(map_idx);
		auto mapfr = map->seq_first_rest();
		if(!mapfr.third) return NIL_NODE;
		coll_idx = mapfr.first;
		map_idx = mapfr.second;
		coll = get_node(coll_idx);
	}

	auto collfr = coll->seq_first_rest();
	node_idx_t val = collfr.first;

	list_ptr_t ret = new_list();
	ret->push_back_inplace(val);
	ret->push_back_inplace(env->get("mapcat-next"));
	ret->push_back_inplace(map_idx);
	ret->push_back_inplace(collfr.second);
	return new_node_list(ret);
}

// (keys map)
// Returns a sequence of the map's keys, in the same order as (seq map).
static node_idx_t native_keys(env_ptr_t env, list_ptr_t args) {
	return native_map(env, list_va(env->get("key"), args->first_value()));
}

// (vals map)
// Returns a sequence of the map's values, in the same order as (seq map).
static node_idx_t native_vals(env_ptr_t env, list_ptr_t args) {
	return native_map(env, list_va(env->get("val"), args->first_value()));
}

// (reductions f coll)(reductions f init coll)
// Returns a lazy seq of the intermediate values of the reduction (as
// per reduce) of coll by f, starting with init.
static node_idx_t native_reductions(env_ptr_t env, list_ptr_t args) {
	node_idx_t f_idx, init_idx, coll_idx;
	if(args->size() == 2) {
		f_idx = args->first_value();
		coll_idx = args->second_value();
		node_t *coll = get_node(coll_idx);
		auto fr = coll->seq_first_rest();
		init_idx = INV_NODE;
	} else if(args->size() == 3) {
		f_idx = args->first_value();
		init_idx = args->second_value();
		coll_idx = args->third_value();
	} else {
		warnf("(reductions f coll) or (reductions f init coll) expected.\n");
		return NIL_NODE;
	}
	return new_node_lazy_list(env, new_node_list(list_va(env->get("reductions-next"), f_idx, init_idx, coll_idx)));
}

static node_idx_t native_reductions_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f_idx = *it++;
	node_idx_t init_idx = *it++;
	node_idx_t coll_idx = *it++;
	node_t *coll = get_node(coll_idx);
	auto fr = coll->seq_first_rest();
	if(!fr.third) {
		return NIL_NODE;
	}
	node_idx_t reti;
	if(init_idx == INV_NODE) {
		reti = fr.first;
	} else {
		reti = eval_va(env, f_idx, init_idx, fr.first);
	}
	return new_node_list(list_va(reti, env->get("reductions-next"), f_idx, reti, fr.second));
}

// (remove pred)(remove pred coll)
// Returns a lazy sequence of the items in coll for which
// (pred item) returns logical false. pred must be free of side-effects.
// Returns a transducer when no collection is provided.
static node_idx_t native_remove(env_ptr_t env, list_ptr_t args) {
	if(args->size() != 2) {
		warnf("(remove pred coll) expected.\n");
		return NIL_NODE;
	}
	node_idx_t pred_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	return new_node_lazy_list(env, new_node_list(list_va(env->get("remove-next"), pred_idx, coll_idx)));
}

static node_idx_t native_remove_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t pred_idx = *it++;
	node_idx_t coll_idx = *it++;
	node_t *coll = get_node(coll_idx);
	auto collfr = coll->seq_first_rest();

	node_idx_t reti;
	do {
		if(!collfr.third) return NIL_NODE;
		reti = eval_va(env, pred_idx, collfr.first);
		if(!get_node_bool(reti)) break;
		coll_idx = collfr.second;
		coll = get_node(coll_idx);
		collfr = coll->seq_first_rest();
	} while(true);

	list_ptr_t ret = new_list();
	ret->push_back_inplace(collfr.first);
	ret->push_back_inplace(env->get("remove-next"));
	ret->push_back_inplace(pred_idx);
	ret->push_back_inplace(collfr.second);
	return new_node_list(ret);
}

// (rseq rev)
// Returns, in constant time, a seq of the items in rev (which
// can be a vector or sorted-map), in reverse order. If rev is empty returns nil
static node_idx_t native_rseq(env_ptr_t env, list_ptr_t args) {
	node_idx_t rev_idx = args->first_value();
	node_t *rev = get_node(rev_idx);
	if(!rev->is_vector()) {
		warnf("(rseq rev) expected a vector or sorted-map.\n");
		return NIL_NODE;
	}
	size_t sz = rev->as_vector()->size();
	return new_node_lazy_list(env, new_node_list(list_va(env->get("rseq-next"), rev_idx, new_node_int(sz))));
}

static node_idx_t native_rseq_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t rev_idx = *it++;
	int sz = get_node_int(*it++);
	if(sz == 0) return NIL_NODE;
	node_t *rev = get_node(rev_idx);
	node_idx_t reti = rev->as_vector()->nth(sz - 1);
	return new_node_list(list_va(reti, env->get("rseq-next"), rev_idx, new_node_int(sz - 1)));
}

void jo_basic_lazy_init(env_ptr_t env) {
	env->set("range", new_node_native_function("range", &native_range, false, NODE_FLAG_PRERESOLVE));
	env->set("range-next", new_node_native_function("range-next", &native_range_next, true, NODE_FLAG_PRERESOLVE));
	env->set("repeat", new_node_native_function("repeat", &native_repeat, true, NODE_FLAG_PRERESOLVE));
	env->set("repeat-next", new_node_native_function("repeat-next", &native_repeat_next, true, NODE_FLAG_PRERESOLVE));
	env->set("concat", new_node_native_function("concat", &native_concat, true, NODE_FLAG_PRERESOLVE));
	env->set("concat-next", new_node_native_function("concat-next", &native_concat_next, true, NODE_FLAG_PRERESOLVE));
	env->set("iterate", new_node_native_function("iterate", &native_iterate, true, NODE_FLAG_PRERESOLVE));
	env->set("iterate-next", new_node_native_function("iterate-next", &native_iterate_next, true, NODE_FLAG_PRERESOLVE));
	env->set("map", new_node_native_function("map", &native_map, false, NODE_FLAG_PRERESOLVE));
	env->set("map-next", new_node_native_function("map-next", &native_map_next, true, NODE_FLAG_PRERESOLVE));
	env->set("mapcat", new_node_native_function("mapcat", &native_mapcat, false, NODE_FLAG_PRERESOLVE));
	env->set("mapcat-next", new_node_native_function("mapcat-next", &native_mapcat_next, true, NODE_FLAG_PRERESOLVE));
	env->set("map-indexed", new_node_native_function("map-indexed", &native_map_indexed, false, NODE_FLAG_PRERESOLVE));
	env->set("map-indexed-next", new_node_native_function("map-indexed-next", &native_map_indexed_next, true, NODE_FLAG_PRERESOLVE));
	env->set("take", new_node_native_function("take", &native_take, true, NODE_FLAG_PRERESOLVE));
	env->set("take-next", new_node_native_function("take-next", &native_take_next, true, NODE_FLAG_PRERESOLVE));
	env->set("take-while", new_node_native_function("take-while", &native_take_while, true, NODE_FLAG_PRERESOLVE));
	env->set("take-while-next", new_node_native_function("take-while-next", &native_take_while_next, true, NODE_FLAG_PRERESOLVE));
	env->set("take-nth", new_node_native_function("take-nth", &native_take_nth, true, NODE_FLAG_PRERESOLVE));
	env->set("take-nth-next", new_node_native_function("take-nth-next", &native_take_nth_next, true, NODE_FLAG_PRERESOLVE));
	env->set("take-last", new_node_native_function("take-last", &native_take_last, true, NODE_FLAG_PRERESOLVE));
	env->set("distinct", new_node_native_function("distinct", &native_distinct, false, NODE_FLAG_PRERESOLVE));
	env->set("filter", new_node_native_function("filter", &native_filter, true, NODE_FLAG_PRERESOLVE));
	env->set("filterv", new_node_native_function("filterv", &native_filter, true, NODE_FLAG_PRERESOLVE));  // same thing as filter?
	env->set("filter-next", new_node_native_function("filter-next", &native_filter_next, true, NODE_FLAG_PRERESOLVE));
	env->set("constantly-next", new_node_native_function("constantly-next", &native_constantly_next, true, NODE_FLAG_PRERESOLVE));
	env->set("keep", new_node_native_function("keep", &native_keep, true, NODE_FLAG_PRERESOLVE));
	env->set("keep-next", new_node_native_function("keep-next", &native_keep_next, true, NODE_FLAG_PRERESOLVE));
	env->set("repeatedly", new_node_native_function("repeatedly", &native_repeatedly, true, NODE_FLAG_PRERESOLVE));
	env->set("repeatedly-next", new_node_native_function("repeatedly-next", &native_repeatedly_next, true, NODE_FLAG_PRERESOLVE));
	env->set("partition", new_node_native_function("partition", &native_partition, false, NODE_FLAG_PRERESOLVE));
	env->set("partition-all", new_node_native_function("partition-all", &native_partition_all, false, NODE_FLAG_PRERESOLVE));
	env->set("partition-next", new_node_native_function("partition-next", &native_partition_next, true, NODE_FLAG_PRERESOLVE));
	env->set("partition-by", new_node_native_function("partition-by", &native_partition_by, false, NODE_FLAG_PRERESOLVE));
	env->set("partition-by-next", new_node_native_function("partition-by-next", &native_partition_by_next, true, NODE_FLAG_PRERESOLVE));
	env->set("drop", new_node_native_function("drop", &native_drop, false, NODE_FLAG_PRERESOLVE));
	env->set("drop-first", new_node_native_function("drop-first", &native_drop_first, false, NODE_FLAG_PRERESOLVE));
	env->set("drop-next", new_node_native_function("drop-next", &native_drop_next, false, NODE_FLAG_PRERESOLVE));
	env->set("drop-while", new_node_native_function("drop-while", &native_drop_while, true, NODE_FLAG_PRERESOLVE));
	env->set("drop-while-first", new_node_native_function("drop-while-first", &native_drop_while_first, true, NODE_FLAG_PRERESOLVE));
	env->set("drop-while-next", new_node_native_function("drop-while-next", &native_drop_while_next, true, NODE_FLAG_PRERESOLVE));
	env->set("interleave", new_node_native_function("interleave", &native_interleave, false, NODE_FLAG_PRERESOLVE));
	env->set("interleave-next", new_node_native_function("interleave-next", &native_interleave_next, true, NODE_FLAG_PRERESOLVE));
	env->set("flatten", new_node_native_function("flatten", &native_flatten, false, NODE_FLAG_PRERESOLVE));
	env->set("flatten-next", new_node_native_function("flatten-next", &native_flatten_next, true, NODE_FLAG_PRERESOLVE));
	env->set("seq", new_node_native_function("seq", &native_seq, true, NODE_FLAG_PRERESOLVE));
	env->set("seq-next", new_node_native_function("seq-next", &native_seq_next, true, NODE_FLAG_PRERESOLVE));
	env->set("lazy-seq", new_node_native_function("lazy-seq", &native_lazy_seq, true, NODE_FLAG_PRERESOLVE));
	env->set("lazy-seq-next", new_node_native_function("lazy-seq-next", &native_lazy_seq_next, true, NODE_FLAG_PRERESOLVE));
	env->set("lazy-cat", new_node_native_function("lazy-cat", &native_lazy_cat, true, NODE_FLAG_PRERESOLVE));
	env->set("cons", new_node_native_function("cons", &native_cons, false, NODE_FLAG_PRERESOLVE));
	env->set("cons-first", new_node_native_function("cons-first", &native_cons_first, true, NODE_FLAG_PRERESOLVE));
	env->set("cons-next", new_node_native_function("cons-next", &native_cons_next, true, NODE_FLAG_PRERESOLVE));
	env->set("cycle", new_node_native_function("cycle", &native_cycle, false, NODE_FLAG_PRERESOLVE));
	env->set("dedupe", new_node_native_function("dedupe", &native_dedupe, false, NODE_FLAG_PRERESOLVE));
	env->set("for", new_node_native_function("for", &native_for, true, NODE_FLAG_PRERESOLVE));
	env->set("interpose", new_node_native_function("interpose", &native_interpose, false, NODE_FLAG_PRERESOLVE));
	env->set("interpose-next-elem", new_node_native_function("interpose-next-elem", &native_interpose_next_elem, true, NODE_FLAG_PRERESOLVE));
	env->set("interpose-next-sep", new_node_native_function("interpose-next-sep", &native_interpose_next_sep, true, NODE_FLAG_PRERESOLVE));
	env->set("keep-indexed", new_node_native_function("keep-indexed", &native_keep_indexed, false, NODE_FLAG_PRERESOLVE));
	env->set("keep-indexed-next", new_node_native_function("keep-indexed-next", &native_keep_indexed_next, true, NODE_FLAG_PRERESOLVE));
	env->set("keys", new_node_native_function("keys", &native_keys, false, NODE_FLAG_PRERESOLVE));
	env->set("vals", new_node_native_function("vals", &native_vals, false, NODE_FLAG_PRERESOLVE));
	env->set("reductions", new_node_native_function("reductions", &native_reductions, false, NODE_FLAG_PRERESOLVE));
	env->set("reductions-next", new_node_native_function("reductions-next", &native_reductions_next, true, NODE_FLAG_PRERESOLVE));
	env->set("remove", new_node_native_function("remove", &native_remove, false, NODE_FLAG_PRERESOLVE));
	env->set("remove-next", new_node_native_function("remove-next", &native_remove_next, true, NODE_FLAG_PRERESOLVE));
	env->set("rseq", new_node_native_function("rseq", &native_rseq, false, NODE_FLAG_PRERESOLVE));
	env->set("rseq-next", new_node_native_function("rseq-next", &native_rseq_next, true, NODE_FLAG_PRERESOLVE));
}
