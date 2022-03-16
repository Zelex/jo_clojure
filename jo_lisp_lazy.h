#pragma once

// (range)
// (range end)
// (range start end)
// (range start end step)
// Returns a lazy seq of nums from start (inclusive) to end
// (exclusive), by step, where start defaults to 0, step to 1, and end to
// infinity. When step is equal to 0, returns an infinite sequence of
// start. When start is equal to end, returns empty list.
static node_idx_t native_range(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	int end = args->size(), start = 0, step = 1;
	if(end == 0) {
		end = INT_MAX; // "infinite" series
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
	// @ Maybe make 16 configurable
	if(end != INT_MAX && (end - start) / step < 16) {
		list_ptr_t ret = new_list();
		for(int i = start; i < end; i += step) {
			ret->push_back_inplace(new_node_int(i));
		}
		return new_node_list(ret);
	}
	// constructs a function which returns the next value in the range, and another function
	node_idx_t lazy_func_idx = new_node(NODE_LIST);
	get_node(lazy_func_idx)->t_list = new_list();
	get_node(lazy_func_idx)->t_list->push_back_inplace(new_node_symbol("range-next"));
	get_node(lazy_func_idx)->t_list->push_back_inplace(new_node_int(start));
	get_node(lazy_func_idx)->t_list->push_back_inplace(new_node_int(step));
	get_node(lazy_func_idx)->t_list->push_back_inplace(new_node_int(end));
	return new_node_lazy_list(lazy_func_idx);
}

static node_idx_t native_range_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	int start = get_node(*it++)->as_int();
	int step = get_node(*it++)->as_int();
	int end = get_node(*it++)->as_int();
	if(start >= end) {
		return NIL_NODE;
	}
	list_ptr_t ret = new_list();
	// this value
	ret->push_back_inplace(new_node_int(start));
	// next function
	ret->push_back_inplace(new_node_symbol("range-next"));
	ret->push_back_inplace(new_node_int(start+step));
	ret->push_back_inplace(new_node_int(step));
	ret->push_back_inplace(new_node_int(end));
	return new_node_list(ret);
}

// (repeat x)
// (repeat n x)
// Returns a lazy (infinite!, or length n if supplied) sequence of xs.
static node_idx_t native_repeat(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t x;
	int n = INT_MAX;
	if(args->size() == 1) {
		x = eval_node(env, *it++);
	} else if(args->size() == 2) {
		n = get_node(eval_node(env, *it++))->as_int();
		x = eval_node(env, *it++);
	} else {
		return NIL_NODE;
	}
	node_idx_t lazy_func_idx = new_node(NODE_LIST);
	get_node(lazy_func_idx)->t_list = new_list();
	get_node(lazy_func_idx)->t_list->push_back_inplace(new_node_symbol("repeat-next"));
	get_node(lazy_func_idx)->t_list->push_back_inplace(x);
	get_node(lazy_func_idx)->t_list->push_back_inplace(new_node_int(n));
	return new_node_lazy_list(lazy_func_idx);
}

static node_idx_t native_repeat_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t x = *it++;
	int n = get_node(*it++)->as_int();
	if(n <= 0) {
		return NIL_NODE;
	}
	list_ptr_t ret = new_list();
	ret->push_back_inplace(x);
	ret->push_back_inplace(new_node_symbol("repeat-next"));
	ret->push_back_inplace(x);
	ret->push_back_inplace(new_node_int(n-1));
	return new_node_list(ret);
}

// (concat)
// (concat x) 
// (concat x y) 
// (concat x y & zs)
// Returns a lazy seq representing the concatenation of the elements in the supplied colls.
static node_idx_t native_concat(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t x = *it++;
	node_idx_t lazy_func_idx = new_node(NODE_LIST);
	get_node(lazy_func_idx)->t_list = new_list();
	get_node(lazy_func_idx)->t_list->push_back_inplace(new_node_symbol("concat-next"));
	get_node(lazy_func_idx)->t_list->conj_inplace(*args.ptr);
	return new_node_lazy_list(lazy_func_idx);
}

static node_idx_t native_concat_next(env_ptr_t env, list_ptr_t args) {
concat_next:
	if(args->size() == 0) {
		return NIL_NODE;
	}
	node_idx_t nidx = eval_node(env, args->first_value());
	node_idx_t val = NIL_NODE;
	int ntype = get_node(nidx)->type;
	args = args->rest();
	if(ntype == NODE_NIL) {
		goto concat_next;
	} else if(ntype == NODE_LIST) {
		list_ptr_t n = get_node(nidx)->t_list;
		if(n->size() == 0) {
			goto concat_next;
		}
		val = n->first_value();
		args->cons_inplace(new_node_list(n->pop()));
	} else if(ntype == NODE_LAZY_LIST) {
		// call the t_lazy_fn, and grab the first element of the return and return that.
		node_idx_t reti = eval_node(env, get_node(nidx)->t_lazy_fn);
		if(reti == NIL_NODE) {
			goto concat_next;
		}
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			val = list_list->first_value();
			args->cons_inplace(new_node_lazy_list(new_node_list(list_list->rest())));
		}
	} else if(ntype == NODE_STRING) {
		// pull off the first character of the string
		jo_string str = get_node(nidx)->t_string;
		if(str.size() == 0) {
			goto concat_next;
		}
		val = new_node_string(str.substr(0, 1));
		args->cons_inplace(new_node_string(str.substr(1)));
	} else {
		val = nidx;
	}
	list_ptr_t ret = new_list();
	ret->push_back_inplace(val);
	ret->push_back_inplace(new_node_symbol("concat-next"));
	ret->conj_inplace(*args.ptr);
	return new_node_list(ret);
}

// (iterate f x)
// Returns a lazy seq representing the infinite sequence of x, f(x), f(f(x)), etc.
// f must be free of side-effects
static node_idx_t native_iterate(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t f = *it++;
	node_idx_t x = *it++;
	node_idx_t lazy_func_idx = new_node(NODE_LIST);
	get_node(lazy_func_idx)->t_list = new_list();
	get_node(lazy_func_idx)->t_list->push_back_inplace(new_node_symbol("iterate-next"));
	get_node(lazy_func_idx)->t_list->push_back_inplace(f);
	get_node(lazy_func_idx)->t_list->push_back_inplace(x);
	return new_node_lazy_list(lazy_func_idx);
}

static node_idx_t native_iterate_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t f = *it++;
	node_idx_t x = *it++;
	list_ptr_t ret = new_list();
	ret->push_back_inplace(x);
	ret->push_back_inplace(new_node_symbol("iterate-next"));
	ret->push_back_inplace(f);
	list_ptr_t f_x_fn = new_list();
	f_x_fn->push_back_inplace(f);
	f_x_fn->push_back_inplace(x);
	node_idx_t f_x = eval_list(env, f_x_fn);
	ret->push_back_inplace(f_x);
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
	list_t::iterator it = args->begin();
	node_idx_t f = *it++;
	if(args->size() == 1) {
		node_idx_t lazy_func_idx = new_node(NODE_LIST);
		node_t *lazy_func = get_node(lazy_func_idx);
		lazy_func->t_list = new_list();
		lazy_func->t_list->push_back_inplace(new_node_symbol("map-next"));
		lazy_func->t_list->push_back_inplace(f);
		return new_node_lazy_list(lazy_func_idx);
	}
	node_idx_t lazy_func_idx = new_node(NODE_LIST);
	get_node(lazy_func_idx)->t_list = new_list();
	get_node(lazy_func_idx)->t_list->push_back_inplace(new_node_symbol("map-next"));
	get_node(lazy_func_idx)->t_list->push_back_inplace(f);
	while(it) {
		get_node(lazy_func_idx)->t_list->push_back_inplace(eval_node(env, *it++));
	}
	return new_node_lazy_list(lazy_func_idx);
}

static node_idx_t native_map_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t f = *it++;
	// pull off the first element of each list and call f with it
	list_ptr_t next_list = new_list();
	list_ptr_t arg_list = new_list();
	arg_list->push_back_inplace(f);
	next_list->push_back_inplace(new_node_symbol("map-next"));
	next_list->push_back_inplace(f);
	for(; it; it++) {
		node_idx_t arg_idx = *it;
		node_t *arg = get_node(arg_idx);
		if(arg->is_list()) {
			list_ptr_t list_list = arg->as_list();
			if(list_list->size() == 0) {
				return NIL_NODE;
			}
			arg_list->push_back_inplace(list_list->first_value());
			next_list->push_back_inplace(new_node_list(list_list->rest()));
		} else if(arg->is_lazy_list()) {
			lazy_list_iterator_t lit(env, arg_idx);
			if(lit.done()) {
				return NIL_NODE;
			}
			arg_list->push_back_inplace(lit.val);
			next_list->push_back_inplace(new_node_lazy_list(lit.next_fn()));
		}
	}
	// call f with the args
	node_idx_t ret = eval_list(env, arg_list);
	next_list->cons_inplace(ret);
	return new_node_list(next_list);
}

// (take n coll)
// Returns a lazy sequence of the first n items in coll, or all items if there are fewer than n.
static node_idx_t native_take(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t n = eval_node(env, *it++);
	node_idx_t coll = eval_node(env, *it++);
	if(get_node_type(coll) == NODE_LIST) {
		// don't do it lazily if not given lazy inputs... thats dumb
		int N = get_node(n)->as_int();
		list_ptr_t list_list = get_node(coll)->as_list();
		if(list_list->size() <= N) {
			return coll;
		}
		return new_node_list(list_list->take(N));
	}
	if(get_node_type(coll) == NODE_LAZY_LIST) {
		node_idx_t lazy_func_idx = new_node(NODE_LIST);
		get_node(lazy_func_idx)->t_list = new_list();
		get_node(lazy_func_idx)->t_list->push_back_inplace(new_node_symbol("take-next"));
		get_node(lazy_func_idx)->t_list->push_back_inplace(n);
		get_node(lazy_func_idx)->t_list->push_back_inplace(coll);
		return new_node_lazy_list(lazy_func_idx);
	}
	return coll;
}

static node_idx_t native_take_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	int n = get_node(*it++)->as_int();
	if(n <= 0) {
		return NIL_NODE;
	}
	node_idx_t coll = *it++;
	lazy_list_iterator_t lit(env, coll);
	if(lit.done()) {
		return NIL_NODE;
	}
	list_ptr_t list = new_list();
	list->push_back_inplace(lit.val);
	list->push_back_inplace(new_node_symbol("take-next"));
	list->push_back_inplace(new_node_int(n-1));
	list->push_back_inplace(new_node_lazy_list(lit.next_fn()));
	return new_node_list(list);
}

// (distinct)
// (distinct coll)
// Returns a lazy sequence of the elements of coll with duplicates removed.
// Returns a stateful transducer when no collection is provided.
static node_idx_t native_distinct(env_ptr_t env, list_ptr_t args) {
	// TODO: lazy? How is that a good idea for distinct? I think it's a bad idea.
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		list_ptr_t ret = new_list();
		for(list_t::iterator it = list_list->begin(); it; it++) {
			node_idx_t value_idx = eval_node(env, *it);
			node_t *value = get_node(value_idx);
			if(!ret->contains([env,value_idx](node_idx_t idx) {
				return node_eq(env, idx, value_idx);
			})) {
				ret->push_back_inplace(value_idx);
			}
		}
		return new_node_list(ret);
	}
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, node_idx);
		list_ptr_t ret = new_list();
		for(; lit; lit.next()) {
			node_idx_t value_idx = eval_node(env, lit.val);
			node_t *value = get_node(value_idx);
			if(!ret->contains([env,value_idx](node_idx_t idx) {
				return node_eq(env, idx, value_idx);
			})) {
				ret->push_back_inplace(value_idx);
			}
		}
		return new_node_list(ret);
	}
	return NIL_NODE;
}

void jo_lisp_lazy_init(env_ptr_t env) {
	env->set_inplace("range", new_node_native_function(&native_range, false));
	env->set_inplace("range-next", new_node_native_function(&native_range_next, false));
	env->set_inplace("repeat", new_node_native_function(&native_repeat, true));
	env->set_inplace("repeat-next", new_node_native_function(&native_repeat_next, true));
	env->set_inplace("concat", new_node_native_function(&native_concat, true));
	env->set_inplace("concat-next", new_node_native_function(&native_concat_next, true));
	env->set_inplace("iterate", new_node_native_function(&native_iterate, true));
	env->set_inplace("iterate-next", new_node_native_function(&native_iterate_next, true));
	env->set_inplace("map", new_node_native_function(&native_map, true));
	env->set_inplace("map-next", new_node_native_function(&native_map_next, true));
	env->set_inplace("take", new_node_native_function(&native_take, true));
	env->set_inplace("take-next", new_node_native_function(&native_take_next, true));
	env->set_inplace("distinct", new_node_native_function(&native_distinct, false));
}
