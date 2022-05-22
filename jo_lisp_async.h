#pragma once

#include <algorithm>

// (atom x)(atom x & options)
// Creates and returns an Atom with an initial value of x and zero or
// more options (in any order):
//  :meta metadata-map
//  :validator validate-fn
//  If metadata-map is supplied, it will become the metadata on the
// atom. validate-fn must be nil or a side-effect-free fn of one
// argument, which will be passed the intended new state on any state
// change. If the new state is unacceptable, the validate-fn should
// return false or throw an exception.
static node_idx_t native_atom(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 1) {
		warnf("(atom) requires at least 1 argument\n");
		return NIL_NODE;
	}

	node_idx_t x_idx = eval_node(env, args->first_value());
	list_ptr_t options = args->rest();

	node_idx_t meta_idx = NIL_NODE;
	node_idx_t validator_idx = NIL_NODE;

	for(auto it = options->begin(); it; it++) {
		if(get_node_type(*it) != NODE_KEYWORD) {
			warnf("(atom) options must be keywords\n");
			return NIL_NODE;
		}
		if(get_node_string(*it) == "meta") {
			meta_idx = eval_node(env, *++it);
		} else if(get_node_string(*it) == "validator") {
			validator_idx = eval_node(env, *++it);
		} else {
			warnf("(atom) unknown option: %s\n", get_node_string(*it).c_str());
			return NIL_NODE;
		}
	}

	return new_node_atom(x_idx);//, meta_idx, validator_idx);
}

// (deref ref)(deref ref timeout-ms timeout-val)
// Also reader macro: @ref/@agent/@var/@atom/@delay/@future/@promise. Within a transaction,
// returns the in-transaction-value of ref, else returns the
// most-recently-committed value of ref. When applied to a var, agent
// or atom, returns its current state. When applied to a delay, forces
// it if not already forced. When applied to a future, will block if
// computation not complete. When applied to a promise, will block
// until a value is delivered.  The variant taking a timeout can be
// used for blocking references (futures and promises), and will return
// timeout-val if the timeout (in milliseconds) is reached before a
// value is available. See also - realized?.
static node_idx_t native_deref(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 1) {
		warnf("(deref) requires at least 1 argument\n");
		return NIL_NODE;
	}

	node_idx_t ref_idx = args->first_value();
	node_idx_t timeout_ms_idx = args->second_value();
	node_idx_t timeout_val_idx = args->third_value();

	node_t *ref = get_node(ref_idx);
	int type = ref->type;
	if(type == NODE_VAR || type == NODE_AGENT) {
	} else if(type == NODE_ATOM) {
		if(env->tx.ptr) {
			env->tx->read(ref_idx);
		} else {
			node_idx_t ret = ref->t_atom;
			while(ret == TX_HOLD_NODE) {
				jo_yield();
				ret = ref->t_atom;
			}
			return ret;
		}
	} else if(type == NODE_DELAY) {
		return eval_node(env, ref_idx);
	} else if(type == NODE_FUTURE) {
		if(!ref->t_future.valid()) {
			return NIL_NODE;
		}
		if(timeout_ms_idx != NIL_NODE) {
			int timeout_ms = get_node_int(timeout_ms_idx);
			if(ref->t_future.wait_for(std::chrono::milliseconds(timeout_ms)) == std::future_status::timeout) {
				return timeout_val_idx;
			}
		} else {
			ref->t_future.wait();
		}
		return ref->t_future.get();
	}
	return NODE_NIL;
}

// (swap! atom f)(swap! atom f x)(swap! atom f x y)(swap! atom f x y & args)
// Atomically swaps the value of atom to be:
// (apply f current-value-of-atom args). Note that f may be called
// multiple times, and thus should be free of side effects.  Returns
// the value that was swapped in.
static node_idx_t native_swap_e(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		warnf("(swap!) requires at least 2 arguments\n");
		return NIL_NODE;
	}

    auto it = args->begin();

	node_idx_t atom_idx = *it++;
	node_idx_t f_idx = *it++;
	list_ptr_t args2 = args->rest(it);

	node_t *atom = get_node(atom_idx);
	if(atom->type != NODE_ATOM) {
		warnf("(swap!) requires an atom\n");
		return NIL_NODE;
	}

	node_t *f = get_node(f_idx);
	if(!f->is_func() && !f->is_native_func()) {
		warnf("(swap!) requires a function\n");
		return NIL_NODE;
	}

	node_idx_t old_val, new_val;

	if(env->tx.ptr) {
		old_val = env->tx->read(atom_idx);
		new_val = eval_list(env, args2->push_front2(f_idx, old_val));
		env->tx->write(atom_idx, new_val);
		return new_val;
	}

	do {
		old_val = atom->t_atom.load();
		while(old_val == TX_HOLD_NODE) {
			jo_yield();
			old_val = atom->t_atom.load();
		}
		new_val = eval_list(env, args2->push_front2(f_idx, old_val));
	} while(!atom->t_atom.compare_exchange_weak(old_val, new_val));
	return new_val;
}

// (reset! atom newval)
// Sets the value of atom to newval without regard for the
// current value. Returns newval.
static node_idx_t native_reset_e(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		warnf("(reset!) requires at least 2 arguments\n");
		return NIL_NODE;
	}

	node_idx_t atom_idx = args->first_value();
	node_t *atom = get_node(atom_idx);
	if(atom->type != NODE_ATOM) {
		warnf("(reset!) requires an atom\n");
		return NIL_NODE;
	}

	node_idx_t old_val, new_val = args->second_value();

	if(env->tx.ptr) {
		env->tx->write(atom_idx, new_val);
		return new_val;
	}

	do {
		old_val = atom->t_atom.load();
		while(old_val == TX_HOLD_NODE) {
			jo_yield();
			old_val = atom->t_atom.load();
		}
	} while(!atom->t_atom.compare_exchange_weak(old_val, new_val));
	return new_val;
}

// (compare-and-set! atom oldval newval)
// Atomically sets the value of atom to newval if and only if the
// current value of the atom is identical to oldval. Returns true if
// set happened, else false
static node_idx_t native_compare_and_set_e(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 3) {
		warnf("(compare-and-set!) requires at least 3 arguments\n");
		return NIL_NODE;
	}

	node_idx_t atom_idx = args->first_value();
	node_idx_t old_val_idx = args->second_value();
	node_idx_t new_val_idx = args->third_value();

	node_t *atom = get_node(atom_idx);
	if(atom->type != NODE_ATOM) {
		warnf("(compare-and-set!) requires an atom\n");
		return NIL_NODE;
	}

	if(env->tx.ptr) {
		node_idx_t old_val = env->tx->read(atom_idx);
		if(old_val == old_val_idx) {
			env->tx->write(atom_idx, new_val_idx);
			return TRUE_NODE;
		}
		return FALSE_NODE;
	}

	return atom->t_atom.compare_exchange_strong(old_val_idx, new_val_idx) ? TRUE_NODE : FALSE_NODE;
}

// (swap-vals! atom f)(swap-vals! atom f x)(swap-vals! atom f x y)(swap-vals! atom f x y & args)
// Atomically swaps the value of atom to be:
// (apply f current-value-of-atom args). Note that f may be called
// multiple times, and thus should be free of side effects.
// Returns [old new], the value of the atom before and after the swap.
static node_idx_t native_swap_vals_e(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		warnf("(swap-vals!) requires at least 2 arguments\n");
		return NIL_NODE;
	}

    auto it = args->begin();
	node_idx_t atom_idx = *it++;
	node_idx_t f_idx = *it++;
	list_ptr_t args2 = args->rest(it);

	node_t *atom = get_node(atom_idx);
	if(atom->type != NODE_ATOM) {
		warnf("(swap-vals!) requires an atom\n");
		return NIL_NODE;
	}

	node_t *f = get_node(f_idx);
	if(!f->is_func() && !f->is_native_func()) {
		warnf("(swap-vals!) requires a function\n");
		return NIL_NODE;
	}

	node_idx_t old_val, new_val;

	if(env->tx.ptr) {
		old_val = env->tx->read(atom_idx);
		new_val = eval_list(env, args2->push_front2(f_idx, old_val));
		env->tx->write(atom_idx, new_val);
		vector_ptr_t ret = new_vector();
		ret->push_back_inplace(old_val);
		ret->push_back_inplace(new_val);
		return new_node_vector(ret);
	}

	do {
		old_val = atom->t_atom.load();
		while(old_val == TX_HOLD_NODE) {
			jo_yield();
			old_val = atom->t_atom.load();
		}
		new_val = eval_list(env, args2->push_front2(f_idx, old_val));
	} while(!atom->t_atom.compare_exchange_weak(old_val, new_val));
	vector_ptr_t ret = new_vector();
	ret->push_back_inplace(old_val);
	ret->push_back_inplace(new_val);
	return new_node_vector(ret);
}

// (reset-vals! atom newval)
// Sets the value of atom to newval. Returns [old new], the value of the
// atom before and after the reset.
static node_idx_t native_reset_vals_e(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		warnf("(reset-vals!) requires at least 2 arguments\n");
		return NIL_NODE;
	}

	node_idx_t atom_idx = args->first_value();
	node_t *atom = get_node(atom_idx);
	if(atom->type != NODE_ATOM) {
		warnf("(reset-vals!) requires an atom\n");
		return NIL_NODE;
	}

	node_idx_t old_val, new_val = args->second_value();

	if(env->tx.ptr) {
		old_val = env->tx->read(atom_idx);
		env->tx->write(atom_idx, new_val);
		return new_node_vector(new_vector()->push_back_inplace(old_val)->push_back_inplace(new_val));
	}

	do {
		old_val = atom->t_atom.load();
		while(old_val == TX_HOLD_NODE) {
			jo_yield();
			old_val = atom->t_atom.load();
		}
	} while(!atom->t_atom.compare_exchange_weak(old_val, new_val));
	return new_node_vector(new_vector()->push_back_inplace(old_val)->push_back_inplace(new_val));
}

// (multi-swap! (atom f args) (atom f args) (atom f args) ...)
// Atomically swaps the values of atoms to be:
// (apply f current-value-of-atom args). Note that f may be called
// multiple times, and thus should be free of side effects.  Returns
// the values that were swapped in.
// atoms are updated in the order they are specified.
static node_idx_t native_multi_swap_e(env_ptr_t env, list_ptr_t args) {
	jo_vector<list_ptr_t> lists;
	jo_vector<node_idx_t> old_vals(args->size());
	jo_vector<node_idx_t> new_vals(args->size());

	lists.reserve(args->size());
	
	// Gather lists
	for(auto it = args->begin(); it; it++) {
		list_ptr_t list = get_node_list(*it);
		if(list->size() < 2) {
			warnf("(multi-swap!) requires at least 2 arguments\n");
			return NIL_NODE;
		}	
		node_idx_t atom = list->first_value();
		node_idx_t f = list->second_value();
		if(get_node_type(atom) != NODE_ATOM) {
			warnf("(multi-swap!) requires an atom\n");
			return NIL_NODE;
		}
		if(!get_node(f)->is_func() && !get_node(f)->is_native_func()) {
			warnf("(multi-swap!) requires a function\n");
			return NIL_NODE;
		}
		lists.push_back(list);
	}

	env_ptr_t env2 = new_env(env);
	env2->begin_transaction();

	do {
		// First get the old values and calc new values
		for(size_t i = 0; i < lists.size(); i++) {
			list_ptr_t list = lists[i];
			auto it2 = list->begin();
			node_idx_t atom_idx = *it2++;
			node_idx_t f_idx = *it2++;
			list_ptr_t args2 = list->rest(it2);
			node_idx_t old_val = env2->tx->read(atom_idx);
			node_idx_t new_val = eval_list(env2, args2->push_front2(f_idx, old_val));
			old_vals[i] = old_val;
			new_vals[i] = new_val;
			env2->tx->write(atom_idx, new_val);
		}
	} while(!env2->end_transaction());

	// return a list of new values
	list_ptr_t ret = new_list();
	for(size_t i = 0; i < lists.size(); i++) {
		ret->push_back_inplace(new_vals[i]);
	}
	return new_node_list(ret);
}

// (multi-reset! (atom new-val) (atom new-val) (atom new-val) ...)
// Atomically resets the values of atoms to be new-vals. Returns the
// values that were reset.
// atoms are updated in the order they are specified.
static node_idx_t native_multi_reset_e(env_ptr_t env, list_ptr_t args) {
	jo_vector<list_ptr_t> lists;
	jo_vector<node_idx_t> old_vals(args->size());
	jo_vector<node_idx_t> new_vals(args->size());

	lists.reserve(args->size());

	// Gather lists
	for(auto it = args->begin(); it; it++) {
		list_ptr_t list = get_node_list(*it);
		if(list->size() < 2) {
			warnf("(multi-reset!) requires at least 2 arguments\n");
			return NIL_NODE;
		}	
		node_idx_t atom = list->first_value();
		if(get_node_type(atom) != NODE_ATOM) {
			warnf("(multi-swap!) requires an atom\n");
			return NIL_NODE;
		}
		lists.push_back(list);
	}
	
	env_ptr_t env2 = new_env(env);
	env2->begin_transaction();

	do {
		// First get the old values and calc new values
		for(size_t i = 0; i < lists.size(); i++) {
			list_ptr_t list = lists[i];
			auto it2 = list->begin();
			node_idx_t atom_idx = *it2++;
			node_idx_t new_val = *it2++;
			node_idx_t old_val = env2->tx->read(atom_idx);
			env2->tx->write(atom_idx, new_val);
			old_vals[i] = old_val;
			new_vals[i] = new_val;
		}
	} while(!env2->end_transaction());

	// return a list of new values
	list_ptr_t ret = new_list();
	for(size_t i = 0; i < lists.size(); i++) {
		ret->push_back_inplace(new_vals[i]);
	}
	return new_node_list(ret);
}

// (multi-swap-vals! (atom f args) (atom f args) (atom f args) ...)
static node_idx_t native_multi_swap_vals_e(env_ptr_t env, list_ptr_t args) {
	jo_vector<list_ptr_t> lists;
	jo_vector<node_idx_t> old_vals(args->size());
	jo_vector<node_idx_t> new_vals(args->size());

	lists.reserve(args->size());

	// Gather lists
	for(auto it = args->begin(); it; it++) {
		list_ptr_t list = get_node_list(*it);
		if(list->size() < 2) {
			warnf("(multi-swap!) requires at least 2 arguments\n");
			return NIL_NODE;
		}	
		node_idx_t atom = list->first_value();
		node_idx_t f = list->second_value();
		if(get_node_type(atom) != NODE_ATOM) {
			warnf("(multi-swap!) requires an atom\n");
			return NIL_NODE;
		}
		if(!get_node(f)->is_func() && !get_node(f)->is_native_func()) {
			warnf("(multi-swap!) requires a function\n");
			return NIL_NODE;
		}
		lists.push_back(list);
	}
	
	env_ptr_t env2 = new_env(env);
	env2->begin_transaction();

	do {
		// First get the old values and calc new values
		for(size_t i = 0; i < lists.size(); i++) {
			list_ptr_t list = lists[i];
			auto it2 = list->begin();
			node_idx_t atom_idx = *it2++;
			node_idx_t f_idx = *it2++;
			list_ptr_t args2 = list->rest(it2);
			node_idx_t old_val = env2->tx->read(atom_idx);
			node_idx_t new_val = eval_list(env2, args2->push_front2(f_idx, old_val));
			old_vals[i] = old_val;
			new_vals[i] = new_val;
			env2->tx->write(atom_idx, new_val);
		}
	} while(!env2->end_transaction());

	// return a list of new values
	list_ptr_t ret = new_list();
	for(size_t i = 0; i < lists.size(); i++) {
		vector_ptr_t ret2 = new_vector();
		ret2->push_back_inplace(old_vals[i]);
		ret2->push_back_inplace(new_vals[i]);
		ret->push_back_inplace(new_node_vector(ret2));
	}
	return new_node_list(ret);
}

// (multi-reset-vals! (atom new-val) (atom new-val) (atom new-val) ...)
// Atomically resets the values of atoms to be new-vals. Returns the
// values that were reset.
// atoms are updated in the order they are specified.
static node_idx_t native_multi_reset_vals_e(env_ptr_t env, list_ptr_t args) {
	jo_vector<list_ptr_t> lists;
	jo_vector<node_idx_t> old_vals(args->size());
	jo_vector<node_idx_t> new_vals(args->size());

	lists.reserve(args->size());
	
	// Gather lists
	for(auto it = args->begin(); it; it++) {
		list_ptr_t list = get_node_list(*it);
		if(list->size() < 2) {
			warnf("(multi-reset!) requires at least 2 arguments\n");
			return NIL_NODE;
		}	
		node_idx_t atom = list->first_value();
		if(get_node_type(atom) != NODE_ATOM) {
			warnf("(multi-swap!) requires an atom\n");
			return NIL_NODE;
		}
		lists.push_back(list);
	}

	env_ptr_t env2 = new_env(env);
	env2->begin_transaction();

	do {
		// First get the old values and calc new values
		for(size_t i = 0; i < lists.size(); i++) {
			list_ptr_t list = lists[i];
			auto it2 = list->begin();
			node_idx_t atom_idx = *it2++;
			node_idx_t new_val = *it2++;
			node_idx_t old_val = env2->tx->read(atom_idx);
			env2->tx->write(atom_idx, new_val);
			old_vals[i] = old_val;
			new_vals[i] = new_val;
		}
	} while(!env2->end_transaction());

	// return a list of new values
	list_ptr_t ret = new_list();
	for(size_t i = 0; i < lists.size(); i++) {
		vector_ptr_t ret2 = new_vector();
		ret2->push_back_inplace(old_vals[i]);
		ret2->push_back_inplace(new_vals[i]);
		ret->push_back_inplace(new_node_vector(ret2));
	}
	return new_node_list(ret);
}

// (dosync & exprs)
// Runs the exprs (in an implicit do) in a transaction that encompasses
// exprs and any nested calls.  Starts a transaction if none is already
// running on this thread. Any uncaught exception will abort the
// transaction and flow out of dosync. The exprs may be run more than
// once, but any effects on Atoms will be atomic.
static node_idx_t native_dosync(env_ptr_t env, list_ptr_t args) {
	env_ptr_t env2 = new_env(env);
	env2->begin_transaction();
	node_idx_t ret;
	do {
		ret = eval_list(env2, args);
	} while(!env2->end_transaction());
	return ret;
}

// (future & body)
// Takes a body of expressions and yields a future object that will
// invoke the body in another thread, and will cache the result and
// return it on all subsequent calls to deref/@. If the computation has
// not yet finished, calls to deref/@ will block, unless the variant of
// deref with timeout is used. See also - realized?.
static node_idx_t native_future(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 1) {
		warnf("(future) requires at least 1 argument\n");
		return NIL_NODE;
	}
	node_idx_t f_idx = new_node(NODE_FUTURE, 0);
	node_t *f = get_node(f_idx);
	f->t_future = std::async(std::launch::async, [env,args]() { return eval_node_list(env, args); });
	return f_idx;
}

// (future-call f)
// Takes a function of no args and yields a future object that will
// invoke the function in another thread, and will cache the result and
// return it on all subsequent calls to deref/@. If the computation has
// not yet finished, calls to deref/@ will block, unless the variant
// of deref with timeout is used. See also - realized?.
static node_idx_t native_future_call(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 1) {
		warnf("(future-call) requires at least 1 argument\n");
		return NIL_NODE;
	}
	node_idx_t f_idx = new_node(NODE_FUTURE, 0);
	node_t *f = get_node(f_idx);
	f->t_future = std::async(std::launch::async, [env,args]() { return eval_list(env, args); });
	return f_idx;
}

// (future-cancel f)
// Cancels the future, if possible.
static node_idx_t native_future_cancel(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 1) {
		warnf("(future-cancel) requires at least 1 argument\n");
		return NIL_NODE;
	}
	node_idx_t f_idx = args->first_value();
	node_t *f = get_node(f_idx);
	if(f->type != NODE_FUTURE) {
		warnf("(future-cancel) requires a future\n");
		return NIL_NODE;
	}
	// TODO? Currently this is a no-op.
	return NIL_NODE;
}

// (future-cancelled? f)
// Returns true if future f is cancelled
static node_idx_t native_future_cancelled(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 1) {
		warnf("(future-cancelled?) requires at least 1 argument\n");
		return NIL_NODE;
	}
	node_idx_t f_idx = args->first_value();
	node_t *f = get_node(f_idx);
	if(f->type != NODE_FUTURE) {
		warnf("(future-cancelled?) requires a future\n");
		return NIL_NODE;
	}
	// TODO? Currently this is a no-op.
	return FALSE_NODE;
}

// (future-done? f)
// Returns true if future f is done
static node_idx_t native_future_done(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 1) {
		warnf("(future-done?) requires at least 1 argument\n");
		return NIL_NODE;
	}
	node_idx_t f_idx = args->first_value();
	node_t *f = get_node(f_idx);
	if(f->type != NODE_FUTURE) {
		warnf("(future-done?) requires a future\n");
		return NIL_NODE;
	}
	return f->t_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_is_future(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_FUTURE ? TRUE_NODE : FALSE_NODE; }

static node_idx_t native_thread_sleep(env_ptr_t env, list_ptr_t args) {
	std::this_thread::sleep_for(std::chrono::milliseconds(get_node_int(args->first_value())));
	return NIL_NODE;
}


void jo_lisp_async_init(env_ptr_t env) {
	// atoms
    env->set("atom", new_node_native_function("atom", &native_atom, true));
    env->set("deref", new_node_native_function("deref", &native_deref, false));
    env->set("swap!", new_node_native_function("swap!", &native_swap_e, false));
    env->set("reset!", new_node_native_function("reset!", &native_reset_e, false));
    env->set("compare-and-set!", new_node_native_function("compare-and-set!", &native_compare_and_set_e, false));
    env->set("swap-vals!", new_node_native_function("swap-vals!", &native_swap_vals_e, false));
    env->set("reset-vals!", new_node_native_function("reset-vals!", &native_reset_vals_e, false));

    // STM like stuff
    env->set("multi-swap!", new_node_native_function("multi-swap!", &native_multi_swap_e, false));
    env->set("multi-reset!", new_node_native_function("multi-reset!", &native_multi_reset_e, false));
    env->set("multi-swap-vals!", new_node_native_function("multi-swap-vals!", &native_multi_swap_vals_e, false));
    env->set("multi-reset-vals!", new_node_native_function("multi-reset-vals!", &native_multi_reset_vals_e, false));
	env->set("dosync", new_node_native_function("dosync", &native_dosync, true));

	// threads
	env->set("Thread/sleep", new_node_native_function("Thread/sleep", &native_thread_sleep, false));

	// futures
	env->set("future", new_node_native_function("future", &native_future, true));
	env->set("future-call", new_node_native_function("future-call", &native_future_call, true));
	env->set("future-cancel", new_node_native_function("future-cancel", &native_future_cancel, false));
	env->set("future-cancelled?", new_node_native_function("future-cancelled?", &native_future_cancelled, false));
	env->set("future-done?", new_node_native_function("future-done?", &native_future_done, false));
	env->set("future?", new_node_native_function("future?", &native_is_future, false));

}
