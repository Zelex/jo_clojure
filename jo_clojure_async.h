#pragma once

// simple wrapper so we can blop it into the t_object generic container
struct jo_clojure_agent_t : jo_object {
	node_idx_t validate = NIL_NODE;
	node_idx_t handler = NIL_NODE;
	node_idx_t mode = NIL_NODE;
	node_idx_t pending = NIL_NODE;
};

typedef jo_alloc_t<jo_clojure_agent_t> jo_clojure_agent_alloc_t;
jo_clojure_agent_alloc_t jo_clojure_agent_alloc;
typedef jo_shared_ptr_t<jo_clojure_agent_t> jo_clojure_agent_ptr_t;
template<typename...A>
jo_clojure_agent_ptr_t new_agent(A...args) { return jo_clojure_agent_ptr_t(jo_clojure_agent_alloc.emplace(args...)); }

static node_idx_t new_node_agent(jo_clojure_agent_ptr_t agent, int flags=0) { return new_node_object(NODE_AGENT, agent.cast<jo_object>(), flags); }

typedef std::function<node_idx_t()> jo_task_t;
typedef jo_shared_ptr<jo_task_t> jo_task_ptr_t;

class jo_threadpool {
	jo_vector<std::thread> pool;
	typedef std::function<void()> task_t;
	jo_mpmcq<task_t*, nullptr, 4096> tasks;

public:
	jo_threadpool(int nr = 1) : pool(), tasks() {
		while(nr-- > 0) {
			std::thread t([this]() {
				tmProfileThread(0,0,0);
				while(true) {
					task_t *task = tasks.pop();
					if(task == nullptr) {
						break;
					}
					(*task)();
					delete task;
				}
			});
			pool.emplace_back(std::move(t));
		}
	}

	~jo_threadpool() {
		tasks.close();
		for(std::thread &t : pool) {
			t.join();
		}
		pool.clear();
	}

	void add_task(jo_task_ptr_t pt) {
		tasks.push(new task_t([pt]{(*pt.ptr)();}));
	}
};

#define USE_THREADPOOL 1

static jo_semaphore thread_limiter(processor_count);
static jo_threadpool *thread_pool = new jo_threadpool(processor_count);
static jo_threadpool *thread_pool2 = new jo_threadpool(processor_count);

static node_idx_t node_swap(env_ptr_t env, node_idx_t atom_idx, node_idx_t f_idx, list_ptr_t args
	, node_idx_t v_idx = NIL_NODE // validator
	) 
{
	node_t *atom = get_node(atom_idx);
	node_idx_t old_val, new_val;

	if(env->tx.ptr) {
		old_val = env->tx->read(atom_idx);
		new_val = eval_list(env, args->push_front2(f_idx, old_val));
		if(v_idx != NIL_NODE) {
			node_idx_t valid = eval_list(env, list_va(v_idx, new_val));
			if(valid == FALSE_NODE) {
				return new_node_exception("swap: validation failed");
			}
			if(get_node_type(valid) == NODE_EXCEPTION) {
				return valid;
			}
		}
		env->tx->write(atom_idx, new_val);
		return new_val;
	}

	int count = 0;
	do {
		old_val = atom->t_atom.load();
		while(old_val <= TX_HOLD_NODE) {
			jo_yield_backoff(&count);
			old_val = atom->t_atom.load();
		}
		new_val = eval_list(env, args->push_front2(f_idx, old_val));
		if(v_idx != NIL_NODE) {
			node_idx_t valid = eval_va(env, v_idx, new_val);
			if(valid == FALSE_NODE) {
				return new_node_exception("swap: validation failed");
			}
			if(get_node_type(valid) == NODE_EXCEPTION) {
				return valid;
			}
		}
		if(atom->t_atom.compare_exchange_weak(old_val, new_val)) {
			break;
		}
		jo_yield_backoff(&count);
		atom_retries++;
	} while(true);
	return new_val;
}

static node_idx_t node_reset(env_ptr_t env, node_idx_t atom_idx, node_idx_t new_val) {
	node_t *atom = get_node(atom_idx);
	if(env->tx.ptr) {
		env->tx->write(atom_idx, new_val);
	} else {
		node_idx_t old_val;
		do {
			old_val = atom->t_atom.load();
			int count = 0;
			while(old_val <= TX_HOLD_NODE) {
				jo_yield_backoff(&count);
				old_val = atom->t_atom.load();
			}
		} while(!atom->t_atom.compare_exchange_weak(old_val, new_val));
	}
	return new_val;
}

static bool node_compare_and_set(env_ptr_t env, node_idx_t atom_idx, node_idx_t old_val, node_idx_t new_val) {
	node_t *atom = get_node(atom_idx);
	if(env->tx.ptr) {
		if(env->tx->read(atom_idx) == old_val) {
			env->tx->write(atom_idx, new_val);
			return true;
		}
		return false;
	}
	return atom->t_atom.compare_exchange_weak(old_val, new_val);
}

static node_idx_t node_deref(env_ptr_t env, node_idx_t atom_idx) {
	node_t *atom = get_node(atom_idx);
	if(env->tx.ptr) {
		return env->tx->read(atom_idx);
	}
	node_idx_t ret = atom->t_atom.load();
	int count = 0;
	while(ret < 0) {
		jo_yield_backoff(&count);
		ret = atom->t_atom;
	}
	return ret;
}

static node_idx_t node_try_deref(env_ptr_t env, node_idx_t atom_idx) {
	node_t *atom = get_node(atom_idx);
	if(env->tx.ptr) {
		return env->tx->read(atom_idx);
	}
	return atom->t_atom.load();
}

static node_idx_t native_thread_workers(env_ptr_t env, list_ptr_t args) {
	delete thread_pool;
	delete thread_pool2;
	thread_pool = new jo_threadpool(get_node_int(args->first_value()));
	thread_pool2 = new jo_threadpool(get_node_int(args->first_value()));
	return NIL_NODE;
}


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

	node_idx_t x_idx = args->first_value();
	list_ptr_t options = args->rest();

	node_idx_t meta_idx = NIL_NODE;
	node_idx_t validator_idx = NIL_NODE;

	for(list_t::iterator it(options); it; it++) {
		if(get_node_type(*it) != NODE_KEYWORD) {
			warnf("(atom) options must be keywords\n");
			return NIL_NODE;
		}
		if(get_node_string(*it) == "meta") {
			meta_idx = *++it;
		} else if(get_node_string(*it) == "validator") {
			validator_idx = *++it;
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

	node_idx_t ref_idx = eval_node(env, args->first_value());
	node_idx_t timeout_ms_idx = eval_node(env, args->second_value());
	node_idx_t timeout_val_idx = eval_node(env, args->third_value());

	node_t *ref = get_node(ref_idx);
	int type = ref->type;
	if(type == NODE_VAR) {
	} else if(type == NODE_ATOM || type == NODE_AGENT) {
		if(env->tx.ptr) {
			return env->tx->read(ref_idx);
		} else {
			node_idx_t ret = ref->t_atom.load();
			int count = 0;
			while(ret <= TX_HOLD_NODE) {
				jo_yield_backoff(&count);
				ret = ref->t_atom.load();
			}
			return ret;
		}
	} else if(type == NODE_DELAY) {
		return eval_node(env, ref_idx);
	} else if(type == NODE_FUTURE || type == NODE_PROMISE) {
		node_idx_t ret = ref->t_atom.load();
		if(timeout_ms_idx != NIL_NODE) {
			double A = jo_time();
			long long timeout_ms = get_node_int(timeout_ms_idx);
			int count = 0;
			while(ret < 0) {
				if(jo_time() - A >= timeout_ms * 0.001) {
					return timeout_val_idx;
				}
				jo_yield_backoff(&count);
				ret = ref->t_atom;
			}
		} else {
			int count = 0;
			while(ret < 0) {
				jo_yield_backoff(&count);
				ret = ref->t_atom;
			}
		}
		return ret;
	}
	return NODE_NIL;
}

// (swap! atom f)(swap! atom f x)(swap! atom f x y)(swap! atom f x y & args)
// Atomically swaps the value of atom to be:
// (apply f current-value-of-atom args). Note that f may be called
// multiple times, and thus should be free of side effects.  Returns
// the value that was swapped in.
static node_idx_t native_swap_e(env_ptr_t env, list_ptr_t args) {
	return node_swap(env, args->first_value(), args->second_value(), args->drop(2));
}

// (reset! atom newval)
// Sets the value of atom to newval without regard for the
// current value. Returns newval.
static node_idx_t native_reset_e(env_ptr_t env, list_ptr_t args) {
	return node_reset(env, args->first_value(), args->second_value());
}

// (compare-and-set! atom oldval newval)
// Atomically sets the value of atom to newval if and only if the
// current value of the atom is identical to oldval. Returns true if
// set happened, else false
static node_idx_t native_compare_and_set_e(env_ptr_t env, list_ptr_t args) {
	return node_compare_and_set(env, args->first_value(), args->second_value(), args->third_value()) ? TRUE_NODE : FALSE_NODE;
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

    list_t::iterator it(args);
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
		int count = 0;
		while(old_val <= TX_HOLD_NODE) {
			jo_yield_backoff(&count);
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
		return new_node_vector(vector_va(old_val, new_val));
	}

	do {
		old_val = atom->t_atom.load();
		int count = 0;
		while(old_val <= TX_HOLD_NODE) {
			jo_yield_backoff(&count);
			old_val = atom->t_atom.load();
		}
	} while(!atom->t_atom.compare_exchange_weak(old_val, new_val));
	return new_node_vector(vector_va(old_val, new_val));
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
	for(list_t::iterator it(args); it; it++) {
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

	int count = 0;
	do {
		// First get the old values and calc new values
		for(size_t i = 0; i < lists.size(); i++) {
			list_ptr_t list = lists[i];
			list_t::iterator it2(list);
			node_idx_t atom_idx = *it2++;
			node_idx_t f_idx = *it2++;
			list_ptr_t args2 = list->rest(it2);
			node_idx_t old_val = env2->tx->read(atom_idx);
			node_idx_t new_val = eval_list(env2, args2->push_front2(f_idx, old_val));
			old_vals[i] = old_val;
			new_vals[i] = new_val;
			env2->tx->write(atom_idx, new_val);
		}
		if(env2->end_transaction()) {
			break;
		}
		for(size_t i = 0; i < lists.size(); i++) {
			old_vals[i] = NIL_NODE;
			new_vals[i] = NIL_NODE;
		}
		jo_yield_backoff(&count);
	} while(true);

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
	for(list_t::iterator it(args); it; it++) {
		list_ptr_t list = get_node_list(*it);
		if(list->size() < 2) {
			warnf("(multi-reset!) requires at least 2 arguments\n");
			return NIL_NODE;
		}	
		node_idx_t atom = list->first_value();
		if(get_node_type(atom) != NODE_ATOM) {
			warnf("(multi-reset!) requires an atom\n");
			return NIL_NODE;
		}
		lists.push_back(list);
	}
	
	env_ptr_t env2 = new_env(env);
	env2->begin_transaction();

	int count = 0;
	do {
		// First get the old values and calc new values
		for(size_t i = 0; i < lists.size(); i++) {
			list_ptr_t list = lists[i];
			list_t::iterator it2(list);
			node_idx_t atom_idx = *it2++;
			node_idx_t new_val = *it2++;
			env2->tx->write(atom_idx, new_val);
			new_vals[i] = new_val;
		}
		if(env2->end_transaction()) {
			break;
		}
		for(size_t i = 0; i < lists.size(); i++) {
			new_vals[i] = NIL_NODE;
		}
		jo_yield_backoff(&count);
	} while(true);

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
	for(list_t::iterator it(args); it; it++) {
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

	int count = 0;
	do {
		// First get the old values and calc new values
		for(size_t i = 0; i < lists.size(); i++) {
			list_ptr_t list = lists[i];
			list_t::iterator it2(list);
			node_idx_t atom_idx = *it2++;
			node_idx_t f_idx = *it2++;
			list_ptr_t args2 = list->rest(it2);
			node_idx_t old_val = env2->tx->read(atom_idx);
			node_idx_t new_val = eval_list(env2, args2->push_front2(f_idx, old_val));
			old_vals[i] = old_val;
			new_vals[i] = new_val;
			env2->tx->write(atom_idx, new_val);
		}
		if(env2->end_transaction()) {
			break;
		}
		for(size_t i = 0; i < lists.size(); i++) {
			old_vals[i] = NIL_NODE;
			new_vals[i] = NIL_NODE;
		}
		jo_yield_backoff(&count);
	} while(true);

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
	for(list_t::iterator it(args); it; it++) {
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

	int count = 0;
	do {
		// First get the old values and calc new values
		for(size_t i = 0; i < lists.size(); i++) {
			list_ptr_t list = lists[i];
			list_t::iterator it2(list);
			node_idx_t atom_idx = *it2++;
			node_idx_t new_val = *it2++;
			node_idx_t old_val = env2->tx->read(atom_idx);
			env2->tx->write(atom_idx, new_val);
			old_vals[i] = old_val;
			new_vals[i] = new_val;
		}
		if(env2->end_transaction()) {
			break;
		}
		for(size_t i = 0; i < lists.size(); i++) {
			old_vals[i] = NIL_NODE;
			new_vals[i] = NIL_NODE;
		}
		jo_yield_backoff(&count);
	} while(true);

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
	int count = 0;
	do {
		ret = eval_node_list(env2, args);
		if(env2->end_transaction()) {
			break;
		}
		ret = NIL_NODE;
		jo_yield_backoff(&count);
	} while(true);
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
	node_reset(env, f_idx, INV_NODE);
	jo_task_ptr_t task = new jo_task_t([env,args,f_idx]() -> node_idx_t {
		node_reset(env, f_idx, eval_node_list(env, args));
		return NIL_NODE;
	});
	thread_pool->add_task(task);
	return f_idx;
}

// (auto-future & body)
// like future, but deref is automatic
static node_idx_t native_auto_future(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 1) {
		warnf("(auto-future) requires at least 1 argument\n");
		return NIL_NODE;
	}
	node_idx_t f_idx = new_node(NODE_FUTURE, NODE_FLAG_AUTO_DEREF);
	node_reset(env, f_idx, INV_NODE);
	jo_task_ptr_t task = new jo_task_t([env,args,f_idx]() -> node_idx_t {
		node_reset(env, f_idx, eval_node_list(env, args));
		return NIL_NODE;
	});
	thread_pool->add_task(task);
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
	node_reset(env, f_idx, INV_NODE);
	jo_task_ptr_t task = new jo_task_t([env,args,f_idx]() -> node_idx_t {
		node_reset(env, f_idx, eval_list(env, args));
		return NIL_NODE;
	});
	thread_pool->add_task(task);
	return f_idx;
}

// (auto-future-call f)
// like future-call, but deref is automatic
static node_idx_t native_auto_future_call(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 1) {
		warnf("(auto-future-call) requires at least 1 argument\n");
		return NIL_NODE;
	}
	node_idx_t f_idx = new_node(NODE_FUTURE, NODE_FLAG_AUTO_DEREF);
	node_reset(env, f_idx, INV_NODE);
	jo_task_ptr_t task = new jo_task_t([env,args,f_idx]() -> node_idx_t {
		node_reset(env, f_idx, eval_list(env, args));
		return NIL_NODE;
	});
	thread_pool->add_task(task);
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
	return node_try_deref(env, args->first_value()) >= 0 ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_is_future(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_FUTURE ? TRUE_NODE : FALSE_NODE; }

static node_idx_t native_thread_sleep(env_ptr_t env, list_ptr_t args) {
	double ms = get_node_float(args->first_value());
	jo_sleep(ms / 1000.0);
	return NIL_NODE;
}

// (io! & body)
// If an io! block occurs in a transaction, throws an
// IllegalStateException, else runs body in an implicit do. If the
// first expression in body is a literal string, will use that as the
// exception message.
static node_idx_t native_io(env_ptr_t env, list_ptr_t args) {
	if(env->tx) {
		warnf("(io!) cannot be used in a transaction\n");
		return NIL_NODE;
	}
	return native_do(env, args);
}

// memoize
// Returns a memoized version of a referentially transparent function. The
// memoized version of the function keeps a cache of the mapping from arguments
// to results and, when calls with the same arguments are repeated often, has
// higher performance at the expense of higher memory use.
static node_idx_t native_memoize(env_ptr_t env, list_ptr_t args) {
	node_idx_t f = args->first_value();
	hash_map_ptr_t cache_map = new_hash_map();
	node_idx_t cache_map_idx = new_node_hash_map(cache_map);
	node_idx_t cache_idx = new_node_atom(cache_map_idx);
	node_idx_t func_idx = new_node(NODE_NATIVE_FUNC, 0);
	node_t *func = get_node(func_idx);
	func->t_native_function = new native_func_t([f,cache_idx](env_ptr_t env, list_ptr_t args) -> node_idx_t {
		node_idx_t args_idx = new_node_list(args);
		node_idx_t C_idx = native_deref(env, list_va(cache_idx));
		node_t *C = get_node(C_idx);
		hash_map_ptr_t mem = C->as_hash_map();
		if(mem->contains(args_idx, node_eq)) {
			return mem->get(args_idx, node_eq);
		}
		node_idx_t ret = eval_list(env, args->push_front(f));
		native_swap_e(env, list_va(cache_idx, env->get("assoc"), args_idx, ret));
		return ret;
	});
	return func_idx;
}

// (pmap f coll)(pmap f coll & colls)
// Like map, except f is applied in parallel. Semi-lazy in that the
// parallel computation stays ahead of the consumption, but doesn't
// realize the entire result unless required. Only useful for
// computationally intensive functions where the time of f dominates
// the coordination overhead.
static node_idx_t native_pmap(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f = *it++;
	list_ptr_t ret = new_list();
	ret->push_back_inplace(env->get("pmap-next"));
	ret->push_back_inplace(f);
	while(it) {
		ret->push_back_inplace(eval_node(env, *it++));
	}
	return new_node_lazy_list(env, new_node_list(ret));
}

static node_idx_t native_pmap_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f = *it++;
	// pull off the first element of each list and call f with it
	list_ptr_t next_list = new_list();
	list_ptr_t arg_list = new_list();
	arg_list->push_back_inplace(f);
	next_list->push_back_inplace(env->get("pmap-next"));
	next_list->push_back_inplace(f);
	for(; it; it++) {
		node_idx_t arg_idx = *it;
		node_t *arg = get_node(arg_idx);
		auto fr = arg->seq_first_rest();
		if(!fr.third) return NIL_NODE;
		arg_list->push_back_inplace(fr.first);
		next_list->push_back_inplace(fr.second);
	}
	// call f with the args
	node_idx_t ret = native_auto_future(env, list_va(new_node_list(arg_list)));
	next_list->cons_inplace(ret);
	return new_node_list(next_list);
}

// (pcalls & fns)
// Executes the no-arg fns in parallel, returning a lazy sequence of
// their values
static node_idx_t native_pcalls(env_ptr_t env, list_ptr_t args) {
	return new_node_lazy_list(env, new_node_list(args->push_front(env->get("pcalls-next"))));
}

static node_idx_t native_pcalls_next(env_ptr_t env, list_ptr_t args) {
	if(!args->size()) {
		return NIL_NODE;
	}
	node_idx_t ret = native_auto_future_call(env, list_va(args->first_value()));
	return new_node_list(args->rest()->push_front2(ret, env->get("pcalls-next")));
}

// (pvalues & fns)
// Executes the no-arg fns in parallel, returning a lazy sequence of
// their values
static node_idx_t native_pvalues(env_ptr_t env, list_ptr_t args) {
	return new_node_lazy_list(env, new_node_list(args->push_front(env->get("pvalues-next"))));
}

static node_idx_t native_pvalues_next(env_ptr_t env, list_ptr_t args) {
	if(!args->size()) {
		return NIL_NODE;
	}
	node_idx_t ret = native_auto_future(env, list_va(args->first_value()));
	return new_node_list(args->rest()->push_front2(ret, env->get("pvalues-next")));
}

// TODO: Implement promise with atoms so its STM compliant!

// (promise)
// Returns a promise object that can be read with deref/@, and set,
// once only, with deliver. Calls to deref/@ prior to delivery will
// block, unless the variant of deref with timeout is used. All
// subsequent derefs will return the same delivered value without
// blocking. See also - realized?.
static node_idx_t native_promise(env_ptr_t env, list_ptr_t args) {
	node_idx_t ret_idx = new_node(NODE_PROMISE, 0);
	node_reset(env, ret_idx, INV_NODE);
	return ret_idx;
}

// (deliver promise val)
// Delivers the supplied value to the promise, releasing any pending
// derefs. A subsequent call to deliver on a promise will have no effect.
static node_idx_t native_deliver(env_ptr_t env, list_ptr_t args) {
	node_compare_and_set(env, args->first_value(), INV_NODE, args->second_value());
	return NIL_NODE;
}

// (locking x & body)
// Executes exprs in an implicit do, while holding the monitor of x.
// Will release the monitor of x in all circumstances.
static node_idx_t native_locking(env_ptr_t env, list_ptr_t args) {
	node_idx_t x_idx = args->first_value();
	node_t *atom = get_node(x_idx);

	node_idx_t old_val;

	size_t current_thread_id = thread_id;

	// mutexes in transactions (does this even make sense?)
	if(env->tx.ptr) {
		// TODO
		warnf("locking in transactions! Danger Will Robinson! TODO abort transaction if fail");
	}

	// lock the atom
	int count = 0;
	do {
		old_val = atom->t_atom.load();
		while(old_val <= TX_HOLD_NODE) {
			// re-entrant support
			if(atom->t_thread_id == current_thread_id) {
				break;
			}
			jo_yield_backoff(&count);
			old_val = atom->t_atom.load();
		}
		if(atom->t_atom.compare_exchange_weak(old_val, TX_HOLD_NODE)) {
			break;
		}
		jo_yield_backoff(&count);
		atom_retries++;
	} while(true);

	// re-entrant support
	atom->t_thread_id = current_thread_id;

	// do whatever it is that its locked for...
	node_idx_t ret = eval_node_list(env, args->rest());

	// unlock the atom
	if(old_val > TX_HOLD_NODE) {
		atom->t_atom.store(old_val);
	}

	return ret;
}

static node_idx_t native_atom_retries(env_ptr_t env, list_ptr_t args) {
	return new_node_int(atom_retries);
}

static node_idx_t native_atom_retries_reset(env_ptr_t env, list_ptr_t args) {
	atom_retries = 0;
	return NIL_NODE;
}

static node_idx_t native_stm_retries(env_ptr_t env, list_ptr_t args) {
	return new_node_int(stm_retries);
}

static node_idx_t native_stm_retries_reset(env_ptr_t env, list_ptr_t args) {
	stm_retries = 0;
	return NIL_NODE;
}

// Returns true if a value has been produced for a promise, delay, future or lazy sequence.
static node_idx_t native_realized(env_ptr_t env, list_ptr_t args) {
	return node_try_deref(env, args->first_value()) >= 0 ? TRUE_NODE : NIL_NODE;
}

static node_idx_t native_tx_start_time(env_ptr_t env, list_ptr_t args) {
	return env->tx.ptr ? new_node_float(env->tx.ptr->start_time) : ZERO_NODE;
}

static node_idx_t native_tx_retries(env_ptr_t env, list_ptr_t args) {
	return env->tx.ptr ? new_node_int(env->tx.ptr->num_retries) : ZERO_NODE;
}

static node_idx_t native_add_tap(env_ptr_t env, list_ptr_t args) {
	node_idx_t fn = args->first_value();
	node_swap(env, TAP_LIST_NODE, env->get("assoc"), list_va(fn));
	return NIL_NODE;
}

static node_idx_t native_remove_tap(env_ptr_t env, list_ptr_t args) {
	node_idx_t fn = args->first_value();
	node_swap(env, TAP_LIST_NODE, env->get("dissoc"), list_va(fn));
	return NIL_NODE;
}

static node_idx_t native_tap(env_ptr_t env, list_ptr_t args) {
	node_idx_t arg = args->first_value();
	node_idx_t tap_list_idx = node_try_deref(env, TAP_LIST_NODE);
	seq_iterate(tap_list_idx, [&](node_idx_t idx) {
		eval_va(env, idx, arg);
		return true;
	});
	return NIL_NODE;
}

// (agent state & options)
// Creates and returns an agent with an initial value of state and
// zero or more options (in any order):
// :meta metadata-map
// :validator validate-fn
// :error-handler handler-fn
// :error-mode mode-keyword
// If metadata-map is supplied, it will become the metadata on the
// agent. validate-fn must be nil or a side-effect-free fn of one
// argument, which will be passed the intended new state on any state
// change. If the new state is unacceptable, the validate-fn should
// return false or throw an exception.  handler-fn is called if an
// action throws an exception or if validate-fn rejects a new state --
// see set-error-handler! for details.  The mode-keyword may be either
// :continue (the default if an error-handler is given) or :fail (the
// default if no error-handler is given) -- see set-error-mode! for
// details.
static node_idx_t native_agent(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t state = *it++;
	jo_clojure_agent_ptr_t a = new_agent();
	a->pending = new_node_atom(EMPTY_SET_NODE);
	node_idx_t agent_idx = new_node_agent(a);
	node_t *agent = get_node(agent_idx);
	for(;it; ++it) {
		switch(*it) {
			case K_META_NODE: agent->t_meta = *++it; break;
			case K_VALIDATOR_NODE: a->validate = *++it; break;
			case K_ERROR_HANDLER_NODE: a->handler = *++it; break;
			case K_ERROR_MODE_NODE: a->mode = *++it; break;
		}
	}
	agent->t_atom.store(state);
	return agent_idx;
}

static jo_task_ptr_t native_send_internal(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t agent_idx = *it++;
	node_idx_t action_fn = *it++;
	list_ptr_t rest = args->rest(it);
	node_idx_t f_idx = new_node(NODE_FUTURE, 0);
	node_reset(env, f_idx, INV_NODE);
	jo_task_ptr_t task = new jo_task_t([env,f_idx,agent_idx,action_fn,rest]() -> node_idx_t {
		jo_clojure_agent_ptr_t agent = get_node(agent_idx)->t_object.cast<jo_clojure_agent_t>();
		node_idx_t nv = node_swap(env, agent_idx, action_fn, rest, agent->validate);
		if(get_node_type(nv) == NODE_EXCEPTION) {
			if(agent->handler != NIL_NODE) {
				eval_va(env, agent->handler, agent_idx, nv);
				// continue by default
			} else {
				// fail by default
			}
		}
		node_reset(env, f_idx, nv);
		node_swap(env, agent->pending, env->get("dissoc"), list_va(f_idx));
		return NIL_NODE;
	});
	jo_clojure_agent_ptr_t agent = get_node(agent_idx)->t_object.cast<jo_clojure_agent_t>();
	node_swap(env, agent->pending, env->get("assoc"), list_va(f_idx));
	return task;
}

static node_idx_t native_send(env_ptr_t env, list_ptr_t args) {
	node_t *agent = get_node(args->first_value());
	if(agent->type == NODE_AGENT) {
		thread_pool->add_task(native_send_internal(env, args));
	}
	return args->first_value();
}

static node_idx_t native_send_off(env_ptr_t env, list_ptr_t args) {
	node_t *agent = get_node(args->first_value());
	if(agent->type == NODE_AGENT) {
		thread_pool2->add_task(native_send_internal(env, args));
	}
	return args->first_value();
}

// (await & agents)
// Blocks the current thread (indefinitely!) until all actions
// dispatched thus far, from this thread or agent, to the agent(s) have
// occurred.  Will block on failed agents.  Will never return if
// a failed agent is restarted with :clear-actions true or shutdown-agents was called.
static node_idx_t native_await(env_ptr_t env, list_ptr_t args) {
	for(list_t::iterator it(args); it; ++it) {
		node_idx_t agent_idx = *it;
		node_t *agent_node = get_node(agent_idx);
		if(agent_node->type != NODE_AGENT) continue;
		jo_clojure_agent_ptr_t agent = get_node(agent_idx)->t_object.cast<jo_clojure_agent_t>();
		node_idx_t set = node_deref(env, agent->pending);
		seq_iterate(set, [&](node_idx_t idx) {
			node_deref(env, idx);
			return true;
		});
	}
	return NIL_NODE;
}


void jo_clojure_async_init(env_ptr_t env) {
	// atoms
    env->set("atom", new_node_native_function("atom", &native_atom, false, NODE_FLAG_PRERESOLVE));
    env->set("deref", new_node_native_function("deref", &native_deref, true, NODE_FLAG_PRERESOLVE));
    env->set("swap!", new_node_native_function("swap!", &native_swap_e, false, NODE_FLAG_PRERESOLVE));
    env->set("reset!", new_node_native_function("reset!", &native_reset_e, false, NODE_FLAG_PRERESOLVE));
    env->set("compare-and-set!", new_node_native_function("compare-and-set!", &native_compare_and_set_e, false, NODE_FLAG_PRERESOLVE));
    env->set("swap-vals!", new_node_native_function("swap-vals!", &native_swap_vals_e, false, NODE_FLAG_PRERESOLVE));
    env->set("reset-vals!", new_node_native_function("reset-vals!", &native_reset_vals_e, false, NODE_FLAG_PRERESOLVE));
	   
	// mutexes
    env->set("locking", new_node_native_function("locking", &native_locking, true, NODE_FLAG_PRERESOLVE));

    // STM like stuff
    env->set("multi-swap!", new_node_native_function("multi-swap!", &native_multi_swap_e, false, NODE_FLAG_PRERESOLVE));
    env->set("multi-reset!", new_node_native_function("multi-reset!", &native_multi_reset_e, false, NODE_FLAG_PRERESOLVE));
    env->set("multi-swap-vals!", new_node_native_function("multi-swap-vals!", &native_multi_swap_vals_e, false, NODE_FLAG_PRERESOLVE));
    env->set("multi-reset-vals!", new_node_native_function("multi-reset-vals!", &native_multi_reset_vals_e, false, NODE_FLAG_PRERESOLVE));
	env->set("dosync", new_node_native_function("dosync", &native_dosync, true, NODE_FLAG_PRERESOLVE));
	env->set("sync", new_node_native_function("sync", &native_dosync, true, NODE_FLAG_PRERESOLVE));
	env->set("io!", new_node_native_function("io!", &native_io, true, NODE_FLAG_PRERESOLVE));

	// threads
	env->set("Thread/sleep", new_node_native_function("Thread/sleep", &native_thread_sleep, false, NODE_FLAG_PRERESOLVE));
	env->set("Thread/workers", new_node_native_function("Thread/workers", &native_thread_workers, false, NODE_FLAG_PRERESOLVE));

	// futures
	env->set("future", new_node_native_function("future", &native_future, true, NODE_FLAG_PRERESOLVE));
	env->set("future-call", new_node_native_function("future-call", &native_future_call, true, NODE_FLAG_PRERESOLVE));
	env->set("future-cancel", new_node_native_function("future-cancel", &native_future_cancel, false, NODE_FLAG_PRERESOLVE));
	env->set("future-cancelled?", new_node_native_function("future-cancelled?", &native_future_cancelled, false, NODE_FLAG_PRERESOLVE));
	env->set("future-done?", new_node_native_function("future-done?", &native_future_done, false, NODE_FLAG_PRERESOLVE));
	env->set("future?", new_node_native_function("future?", &native_is_future, false, NODE_FLAG_PRERESOLVE));
	env->set("auto-future", new_node_native_function("auto-future", &native_auto_future, true, NODE_FLAG_PRERESOLVE));
	env->set("auto-future-call", new_node_native_function("auto-future-call", &native_auto_future_call, true, NODE_FLAG_PRERESOLVE));
	env->set("promise", new_node_native_function("promise", &native_promise, false, NODE_FLAG_PRERESOLVE));
	env->set("deliver", new_node_native_function("deliver", &native_deliver, false, NODE_FLAG_PRERESOLVE));

	// stuff built with auto-futures
	env->set("pmap", new_node_native_function("pmap", &native_pmap, false, NODE_FLAG_PRERESOLVE));
	env->set("pmap-next", new_node_native_function("pmap-next", &native_pmap_next, true, NODE_FLAG_PRERESOLVE));
	env->set("pcalls", new_node_native_function("pcalls", &native_pcalls, false, NODE_FLAG_PRERESOLVE));
	env->set("pcalls-next", new_node_native_function("pcalls-next", &native_pcalls_next, true, NODE_FLAG_PRERESOLVE));
	env->set("pvalues", new_node_native_function("pvalues", &native_pvalues, true, NODE_FLAG_PRERESOLVE));
	env->set("pvalues-next", new_node_native_function("pvalues-next", &native_pvalues_next, true, NODE_FLAG_PRERESOLVE));
	
	// misc
	env->set("*hardware-concurrency*", new_node_int(processor_count, NODE_FLAG_PRERESOLVE));
	env->set("memoize", new_node_native_function("memoize", &native_memoize, false, NODE_FLAG_PRERESOLVE));
	env->set("realized?", new_node_native_function("realized?", &native_realized, false, NODE_FLAG_PRERESOLVE));
	env->set("Thread/atom-retries", new_node_native_function("Thread/atom-retries", &native_atom_retries, false, NODE_FLAG_PRERESOLVE));
	env->set("Thread/atom-retries-reset", new_node_native_function("Thread/atom-retries-reset", &native_atom_retries_reset, false, NODE_FLAG_PRERESOLVE));
	env->set("Thread/stm-retries", new_node_native_function("Thread/stm-retries", &native_stm_retries, false, NODE_FLAG_PRERESOLVE));
	env->set("Thread/stm-retries-reset", new_node_native_function("Thread/stm-retries-reset", &native_stm_retries_reset, false, NODE_FLAG_PRERESOLVE));
	env->set("Thread/tx-start-time", new_node_native_function("Thread/tx-start-time", &native_tx_start_time, false, NODE_FLAG_PRERESOLVE));
	env->set("Thread/tx-retries", new_node_native_function("Thread/tx-retries", &native_tx_retries, false, NODE_FLAG_PRERESOLVE));

	// tap
	env->set("tap>", new_node_native_function("tap>", &native_tap, false, NODE_FLAG_PRERESOLVE));
	env->set("add-tap", new_node_native_function("add-tap", &native_add_tap, false, NODE_FLAG_PRERESOLVE));
	env->set("remove-tap", new_node_native_function("remove-tap", &native_remove_tap, false, NODE_FLAG_PRERESOLVE));

	// agents
	env->set("agent", new_node_native_function("agent", &native_agent, false, NODE_FLAG_PRERESOLVE));
	env->set("send", new_node_native_function("send", &native_send, false, NODE_FLAG_PRERESOLVE));
	env->set("send-off", new_node_native_function("send-off", &native_send_off, false, NODE_FLAG_PRERESOLVE));
	env->set("await", new_node_native_function("await", &native_await, false, NODE_FLAG_PRERESOLVE));
}
