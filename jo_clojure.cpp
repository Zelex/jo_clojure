// TODO: arbitrary precision numbers... really? do I need to support this?
#define _SCL_SECURE 0
#define _HAS_ITERATOR_DEBUGGING 0

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <future>
#include <thread>
#include <shared_mutex>

//#define WITH_TELEMETRY
#ifdef WITH_TELEMETRY
#pragma comment(lib,"rad_tm_win64.lib")
#include "tm/rad_tm.h"
#else
#define tmProfileThread sizeof
#endif

// get current thread id
static std::atomic<size_t> thread_uid(0);
static thread_local size_t thread_id = thread_uid.fetch_add(1);

#include "debugbreak.h"
#include "jo_stdcpp.h"
#include "jo_clojure_persistent.h"

//#define debugf printf
#ifndef debugf
#define debugf sizeof
#endif

#define warnf printf
#ifndef warnf
#define warnf sizeof
#endif

// should be first static in entire program to get actual program start.
static double time_program_start = jo_time();

static std::atomic<size_t> atom_retries(0);
static std::atomic<size_t> stm_retries(0);

static const auto processor_count = std::thread::hardware_concurrency();


enum {

	// hard coded nodes
	TX_HOLD_NODE = -4, // Set to this value if in the middle of a transaction commit. Don't touch while in this state!
	EOL_NODE = -3,
	TMP_NODE = -2,
	INV_NODE = -1,
	NIL_NODE = 0,
	ZERO_NODE,
	ONE_NODE,
	INT_0_NODE = ZERO_NODE,
	INT_256_NODE = INT_0_NODE + 256,
	FALSE_NODE,
	TRUE_NODE,
	QUOTE_NODE,
	UNQUOTE_NODE,
	UNQUOTE_SPLICE_NODE,
	QUASIQUOTE_NODE,
	EMPTY_LIST_NODE,
	EMPTY_VECTOR_NODE,
	EMPTY_MAP_NODE,
	EMPTY_SET_NODE,
	PCT_NODE,
	PCT1_NODE,
	PCT2_NODE,
	PCT3_NODE,
	PCT4_NODE,
	PCT5_NODE,
	PCT6_NODE,
	PCT7_NODE,
	PCT8_NODE,
	K_ELSE_NODE,
	K_WHEN_NODE,
	K_WHILE_NODE,
	K_LET_NODE,
	K_PC_NODE,
	K_ALL_NODE,
	K_BY_NODE,
	K_AUTO_DEREF_NODE,
	START_USER_NODES,

	// node types
	NODE_NIL = 0,
	NODE_BOOL,
	NODE_INT,
	NODE_FLOAT,
	NODE_STRING,
	NODE_SYMBOL,
	NODE_KEYWORD,
	//NODE_REGEX,
	NODE_LIST,
	NODE_LAZY_LIST,
	NODE_VECTOR,
	NODE_VECTOR2D,
	NODE_HASH_SET,
	NODE_MAP,
	NODE_NATIVE_FUNC,
	NODE_FUNC,
	NODE_VAR,
	NODE_DELAY,
	NODE_FILE,
	NODE_DIR,
	NODE_ATOM,
	NODE_AGENT,
	NODE_FUTURE,
	NODE_PROMISE,
	NODE_RECUR,
	NODE_REDUCED,
	NODE_GIF, // jo_gif library

	// node flags
	NODE_FLAG_MACRO        = 1<<0,
	NODE_FLAG_STRING       = 1<<1, // string or symbol or keyword
	NODE_FLAG_LAZY         = 1<<2, // unused
	NODE_FLAG_LITERAL      = 1<<3,
	NODE_FLAG_LITERAL_ARGS = 1<<4,
	NODE_FLAG_PRERESOLVE   = 1<<5,
	NODE_FLAG_AUTO_DEREF   = 1<<6,
	NODE_FLAG_FOREVER	   = 1<<7, // never release this node
	NODE_FLAG_GARBAGE	   = 1<<8, // this node is garbage
	NODE_FLAG_CHAR		   = 1<<9, // char	
};

struct transaction_t;
struct env_t;
struct node_t;
struct lazy_list_iterator_t;


//typedef jo_persistent_vector_bidirectional<node_idx_t> list_t;

typedef jo_shared_ptr_t<jo_object> object_ptr_t;

typedef jo_alloc_t<transaction_t> transaction_alloc_t;
transaction_alloc_t transaction_alloc;
typedef jo_shared_ptr_t<transaction_t> transaction_ptr_t;
static transaction_ptr_t new_transaction() { return transaction_ptr_t(transaction_alloc.emplace()); }

typedef jo_alloc_t<env_t> env_alloc_t;
env_alloc_t env_alloc;
typedef jo_shared_ptr_t<env_t> env_ptr_t;
static env_ptr_t new_env(env_ptr_t parent) { return env_ptr_t(env_alloc.emplace(parent)); }

typedef std::function<node_idx_t(env_ptr_t, list_ptr_t)> native_func_t;
typedef jo_shared_ptr<native_func_t> native_func_ptr_t;

typedef node_idx_t (*native_function_t)(env_ptr_t env, list_ptr_t args);

static inline node_t *get_node(node_idx_t idx);
static inline int get_node_type(node_idx_t idx);
static inline int get_node_type(const node_t *n);
static inline int get_node_flags(node_idx_t idx);
static inline jo_string get_node_string(node_idx_t idx);
static inline node_idx_t get_node_var(node_idx_t idx);
static inline bool get_node_bool(node_idx_t idx);
static inline list_ptr_t get_node_list(node_idx_t idx);
static inline vector_ptr_t get_node_vector(node_idx_t idx);
static inline hash_map_ptr_t get_node_map(node_idx_t idx);
static inline hash_set_ptr_t get_node_hash_set(node_idx_t idx);
static inline long long get_node_int(node_idx_t idx);
static inline double get_node_float(node_idx_t idx);
static inline vector_ptr_t get_node_func_args(node_idx_t idx);
static inline list_ptr_t get_node_func_body(node_idx_t idx);
static inline node_idx_t get_node_lazy_fn(node_idx_t idx);
static inline node_idx_t get_node_lazy_fn(const node_t *n);
static inline FILE *get_node_file(node_idx_t idx);
static inline void *get_node_dir(node_idx_t idx);
static inline atomic_node_idx_t &get_node_atom(node_idx_t idx);
static inline env_ptr_t get_node_env(node_idx_t idx);
static inline env_ptr_t get_node_env(const node_t *n);

static node_idx_t new_node_int(long long i, int flags = 0);
static node_idx_t new_node_list(list_ptr_t nodes, int flags = 0);
static node_idx_t new_node_string(const jo_string &s, int flags = 0);
static node_idx_t new_node_map(hash_map_ptr_t nodes, int flags = 0);
static node_idx_t new_node_hash_set(hash_set_ptr_t nodes, int flags = 0);
static node_idx_t new_node_vector(vector_ptr_t nodes, int flags = 0);
static node_idx_t new_node_vector2d(vector2d_ptr_t nodes, int flags = 0);
static node_idx_t new_node_lazy_list(env_ptr_t env, node_idx_t lazy_fn, int flags = 0);
static node_idx_t new_node_var(const jo_string &name, node_idx_t value, int flags = 0);

static bool node_eq(node_idx_t n1i, node_idx_t n2i);
static bool node_lt(node_idx_t n1i, node_idx_t n2i);
static bool node_lte(node_idx_t n1i, node_idx_t n2i);
static node_idx_t node_add(node_idx_t n1i, node_idx_t n2i);
static node_idx_t node_mul(node_idx_t n1i, node_idx_t n2i);
static void node_let(env_ptr_t env, node_idx_t n1i, node_idx_t n2i);

static node_idx_t eval_node(env_ptr_t env, node_idx_t root);
static node_idx_t eval_node_list(env_ptr_t env, list_ptr_t list);
static node_idx_t eval_list(env_ptr_t env, list_ptr_t list, int list_flags=0);

static node_idx_t eval_va(env_ptr_t env, node_idx_t a);
static node_idx_t eval_va(env_ptr_t env, node_idx_t a, node_idx_t b);
static node_idx_t eval_va(env_ptr_t env, node_idx_t a, node_idx_t b, node_idx_t c);
static node_idx_t eval_va(env_ptr_t env, node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d);
static node_idx_t eval_va(env_ptr_t env, node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e);
static node_idx_t eval_va(env_ptr_t env, node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e, node_idx_t f);

static inline list_ptr_t list_va(node_idx_t a);
static inline list_ptr_t list_va(node_idx_t a, node_idx_t b);
static inline list_ptr_t list_va(node_idx_t a, node_idx_t b, node_idx_t c);
static inline list_ptr_t list_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d);
static inline list_ptr_t list_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e);
static inline list_ptr_t list_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e, node_idx_t f);

static vector_ptr_t vector_va(node_idx_t a);
static vector_ptr_t vector_va(node_idx_t a, node_idx_t b);
static vector_ptr_t vector_va(node_idx_t a, node_idx_t b, node_idx_t c);
static vector_ptr_t vector_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d);
static vector_ptr_t vector_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e);
static vector_ptr_t vector_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e, node_idx_t f);

static void print_node(node_idx_t node, int depth = 0, bool same_line=false);
static void print_node_type(node_idx_t node);
static void print_node_list(list_ptr_t nodes, int depth = 0);
static void print_node_vector(vector_ptr_t nodes, int depth = 0);
static void print_node_map(hash_map_ptr_t nodes, int depth = 0);
static void print_node_set(hash_map_ptr_t nodes, int depth = 0);

struct transaction_t {
	struct tx_t {
		node_idx_t old_val;
		node_idx_t new_val;
		tx_t() : old_val(INV_NODE), new_val(INV_NODE) {}
	};
	//typedef node_idx_t atom_idx_t;
	typedef node_idx_unsafe_t atom_idx_t;
	std::map<atom_idx_t, tx_t> tx_map;
	double start_time;
	int num_retries;

	transaction_t() : tx_map(), start_time(jo_time() - time_program_start), num_retries() {}

	node_idx_t read(atom_idx_t atom_idx) {
		if(tx_map.find(atom_idx) != tx_map.end()) {
			tx_t &tx = tx_map[atom_idx];
			return tx.new_val != INV_NODE ? tx.new_val : tx.old_val;
		}
		debugf("stm read %lld\n", atom_idx);
		auto &atom = get_node_atom(atom_idx);
		node_idx_t old_val = atom.load();
		int count = 0;
		while(old_val == TX_HOLD_NODE) {
			jo_yield_backoff(&count);
			old_val = atom.load();
		}
		tx_t &tx = tx_map[atom_idx];
		tx.old_val = old_val;
		return old_val;
	}

	void write(atom_idx_t atom_idx, node_idx_t new_val) {
		debugf("stm write %lld %lld\n", atom_idx, new_val);
		tx_t &tx = tx_map[atom_idx];
		tx.new_val = new_val;
	}

	// return false if failed and tx must be retried 
	bool commit() {
		if(!tx_map.size()) {
			return true;
		}

		// Transition all values to hold
		for(auto tx = tx_map.begin(); tx != tx_map.end(); tx++) {
			// Write stomp?
			if(tx->second.old_val == INV_NODE) {
				continue;
			}
			auto &atom = get_node_atom(tx->first);
			if(!atom.compare_exchange_strong(tx->second.old_val, TX_HOLD_NODE)) {
				// restore old values... we failed
				for(auto tx2 = tx_map.begin(); tx2 != tx; tx2++) {
					auto &atom2 = get_node_atom(tx2->first);
					atom2.store(tx2->second.old_val);
				}
				tx_map.clear();
				++num_retries;
				return false;
			}
		}

		// Set new values / restore reads from hold status
		for(auto tx = tx_map.begin(); tx != tx_map.end(); tx++) {
			node_idx_t store_val = tx->second.new_val != INV_NODE ? tx->second.new_val : tx->second.old_val;

			auto &atom = get_node_atom(tx->first);
			// If we don't have an old value, cause we only stored, grab one real quick so we can lock it.
			if(tx->second.old_val == INV_NODE) {
				node_idx_t old_val;
				do {
					old_val = atom.load();
					int count = 0;
					while(old_val == TX_HOLD_NODE) {
						jo_yield_backoff(&count);
						old_val = atom.load();
					}
					if(atom.compare_exchange_strong(old_val, store_val)) {
						break;
					}
					jo_yield_backoff(&count);
				} while(true);
			} else {
				atom.store(store_val);
			}
		}

		// TX success!
		tx_map.clear();
		return true;
	}
};


struct env_t {
	// for iterating them all, otherwise unused.
	list_ptr_t vars;
	// TODO: need persistent version of vars_map
	struct fast_val_t {
		node_idx_t var;
		node_idx_t value;
		bool valid;
		fast_val_t() : var(NIL_NODE), value(NIL_NODE), valid() {}
		fast_val_t(node_idx_t _var, node_idx_t _value) : var(_var), value(_value), valid(true) {}
	};
	std::unordered_map<std::string, fast_val_t> vars_map;
	env_ptr_t parent;

	transaction_ptr_t tx;

	env_t() : vars(new_list()), vars_map(), parent(), tx() {}
	env_t(env_ptr_t p) : vars(new_list()), vars_map(), parent(p) {
		if(p) {
			tx = p->tx;
		}
	}

	void begin_transaction() {
		if(!tx) {
			tx = new_transaction();
		}
	}

	bool end_transaction() {
		bool ret = tx->commit();
		if(ret) {
			tx = parent ? parent->tx : nullptr;
		} else {
			stm_retries++;
		}
		return ret;
	}

	fast_val_t find(const jo_string &name) const {
		auto it = vars_map.find(name.c_str());
		if(it != vars_map.end()) {
			return it->second;
		}
		if(parent.ptr) {
			return parent->find(name);
		}
		return fast_val_t();
	}

	inline fast_val_t get(const jo_string &name) const {
		return find(name);
	}

	inline bool has(const jo_string &name) const {
		return find(name).valid;
	}

	void remove(const jo_string &name) {
		auto it = vars_map.find(name.c_str());
		if(it != vars_map.end()) {
			vars = vars->erase(it->second.var);
			vars_map.erase(it);
			return;
		}
		if(parent.ptr) {
			parent->remove(name);
		}
	}

	void set(jo_string name, node_idx_t value) {
		if(has(name)) {
			remove(name);
		}
		node_idx_t idx = new_node_var(name, value, NODE_FLAG_FOREVER);
		vars = vars->push_front(idx);
		vars_map[name.c_str()] = fast_val_t(idx, value);
	}

	// sets the map only, but cannot iterate it. for dotimes and stuffs.
	void set_temp(const jo_string &name, node_idx_t value) {
		vars_map[name.c_str()] = fast_val_t(NIL_NODE, value);
	}

	void print_map(int depth = 0) {
		printf("%*s{", depth, "");
		for(auto it = vars_map.begin(); it != vars_map.end(); it++) {
			print_node(new_node_string(it->first.c_str()), depth);
			printf(" = ");
			print_node(it->second.value, depth);
			printf(",\n");
		}
		printf("%*s}\n", depth, "");
	}
};

struct lazy_list_iterator_t {
	env_ptr_t env;
	node_idx_t cur;
	node_idx_t val;
	jo_vector<node_idx_t> next_list;
	node_idx_unsafe_t next_idx;

	lazy_list_iterator_t(const node_t *node) : env(), cur(), val(NIL_NODE), next_list(), next_idx() {
		if(get_node_type(node) == NODE_LAZY_LIST) {
			env = get_node_env(node);
			cur = eval_node(env, get_node_lazy_fn(node));
			if(!done()) {
				val = get_node_list(cur)->first_value();
			}
		}
	}

	lazy_list_iterator_t(node_idx_t node_idx) : env(), cur(node_idx), val(NIL_NODE), next_list(), next_idx() {
		if(get_node_type(cur) == NODE_LAZY_LIST) {
			env = get_node_env(cur);
			cur = eval_node(env, get_node_lazy_fn(cur));
			if(!done()) {
				val = get_node_list(cur)->first_value();
			}
		}
	}

	bool done() const {
		return next_idx >= (node_idx_unsafe_t)next_list.size() && !get_node_list(cur);
	}

	node_idx_t next() {
		if(next_idx < next_list.size()) {
			val = next_list[next_idx++];
			return val;
		}
		if(done()) {
			return INV_NODE;
		}
		cur = eval_list(env, get_node_list(cur)->rest());
		if(!done()) {
			val = get_node_list(cur)->first_value();
		} else {
			val = INV_NODE;
		}
		return val;
	}

	node_idx_t next_fn() {
		if(done()) {
			return NIL_NODE;
		}
		return new_node_list(get_node_list(cur)->rest());
	}

	node_idx_t next_fn(long long n) {
		for(long long i = 0; i < n; i++) {
			next();
		}
		if(done()) {
			return NIL_NODE;
		}
		return new_node_list(get_node_list(cur)->rest());
	}

	node_idx_t nth(long long n) {
		node_idx_t res = val;
		while(n-- > 0 && !done()) {
			next();
		}
		return val;
	}

	operator bool() const {
		return !done();
	}

	list_ptr_t all(long long n = INT_MAX) {
		list_ptr_t res = new_list();
		while(!done() && n-- > 0) {
			res->push_back_inplace(val);
			next();
		}
		return res;
	}

	// fetch the next N values, and put them into next_list
	void prefetch(long long n) {
		// already prefetched at least n values?
		if(next_list.size() > n || done()) {
			return;
		}
		jo_vector<node_idx_t> res;
		while(n-- > 0 && !done()) {
			res.push_back(val);
			next();
		}
		next_list = std::move(res);
		next_idx = 0;
	}
};

struct node_t {
	std::atomic<int> ref_count;
	int type;
	int flags;
	jo_string t_string;
	list_ptr_t t_list;
	object_ptr_t t_object;
	native_func_ptr_t t_native_function;
	atomic_node_idx_t t_atom;
	env_ptr_t t_env;
	struct {
		vector_ptr_t args;
		list_ptr_t body;
	} t_func;
	// var, delay, lazy_fn, reduced
	node_idx_t t_extra;
	union {
		bool t_bool;
		// most implementations combine these as "number", but at the moment that sounds silly
		long long t_int;
		double t_float;
		FILE *t_file;
		void *t_dir;
		native_function_t t_nfunc_raw;
		volatile unsigned long long t_thread_id;
	};

	inline list_ptr_t &as_list() { return t_list; }
	inline vector_ptr_t &as_vector() { return t_object.cast<vector_t>(); }
	inline vector2d_ptr_t &as_vector2d() { return t_object.cast<vector2d_t>(); }
	inline hash_map_ptr_t &as_hash_map() { return t_object.cast<hash_map_t>(); }
	inline hash_set_ptr_t &as_hash_set() { return t_object.cast<hash_set_t>(); }

	inline const vector_ptr_t &as_vector() const { return t_object.cast<vector_t>(); }
	inline const vector2d_ptr_t &as_vector2d() const { return t_object.cast<vector2d_t>(); }
	inline const hash_map_ptr_t &as_hash_map() const { return t_object.cast<hash_map_t>(); }
	inline const hash_set_ptr_t &as_hash_set() const { return t_object.cast<hash_set_t>(); }

	node_t() 
		: ref_count()
		, type(NODE_NIL)
		, flags(0)
		, t_string()
		, t_list()
		, t_object()
		, t_native_function()
		, t_atom()
		, t_env()
		, t_func()
		, t_extra()
		, t_int(0)
	{
	} 

	// copy constructor
	node_t(const node_t &other) 
	: ref_count()
	, type(other.type)
	, flags(other.flags)
	, t_string(other.t_string)
	, t_list(other.t_list)
	, t_object(other.t_object)
	, t_native_function(other.t_native_function)
	, t_env(other.t_env)
	, t_func(other.t_func)
	, t_extra(other.t_extra)
	, t_int(other.t_int) 
	, t_atom(other.t_atom)
	{
	}

	// move constructor
	node_t(node_t &&other)
	: type(other.type)
	, flags(other.flags)
	, t_string(std::move(other.t_string))
	, t_list(std::move(other.t_list))
	, t_object(std::move(other.t_object))
	, t_native_function(other.t_native_function)
	, t_env(std::move(other.t_env))
	, t_func(std::move(other.t_func))
	, t_extra(other.t_extra)
	, t_int(other.t_int)
	, t_atom(other.t_atom)
	{
	}

	// move assignment operator
	node_t &operator=(node_t &&other) {
		type = other.type;
		flags = other.flags;
		t_string = std::move(other.t_string);
		t_list = std::move(other.t_list);
		t_object = std::move(other.t_object);
		t_native_function = other.t_native_function;
		t_atom.store(other.t_atom.load());
		t_env = std::move(other.t_env);
		t_func = std::move(other.t_func);
		t_extra = other.t_extra;
		t_int = other.t_int;
		return *this;
	}

	void release() {
		ref_count = 0;
		type = NODE_NIL;
		flags = NODE_FLAG_GARBAGE;
		t_list = nullptr;
		t_object = nullptr;
		t_native_function = nullptr;
		t_atom.store(0);
		t_env = nullptr;
		t_func.args = nullptr;
		t_func.body = nullptr;
		t_extra = 0;
		t_int = 0;
	}

	bool is_symbol() const { return type == NODE_SYMBOL; }
	bool is_keyword() const { return type == NODE_KEYWORD; }
	bool is_list() const { return type == NODE_LIST; }
	bool is_vector() const { return type == NODE_VECTOR; }
	bool is_vector2d() const { return type == NODE_VECTOR2D; }
	bool is_hash_map() const { return type == NODE_MAP; }
	bool is_hash_set() const { return type == NODE_HASH_SET; }
	bool is_lazy_list() const { return type == NODE_LAZY_LIST; }
	bool is_string() const { return type == NODE_STRING; }
	bool is_func() const { return type == NODE_FUNC; }
	bool is_native_func() const { return type == NODE_NATIVE_FUNC; }
	bool is_macro() const { return flags & NODE_FLAG_MACRO;}
	bool is_float() const { return type == NODE_FLOAT; }
	bool is_int() const { return type == NODE_INT; }
	bool is_file() const { return type == NODE_FILE; }
	bool is_dir() const { return type == NODE_DIR; }
	bool is_atom() const { return type == NODE_ATOM; }
	bool is_future() const { return type == NODE_FUTURE; }
	bool is_promise() const { return type == NODE_PROMISE; }

	bool is_seq() const { return is_list() || is_lazy_list() || is_hash_map() || is_hash_set() || is_vector(); }
	bool can_eval() const { return is_symbol() || is_keyword() || is_list() || is_func() || is_native_func(); }

	// first, more?
	typedef jo_pair<node_idx_t, bool> seq_first_t;
	seq_first_t seq_first() const {
		if(is_list()) return seq_first_t(t_list->first_value(), !t_list->empty());
		if(is_vector()) return seq_first_t(as_vector()->first_value(), !as_vector()->empty());
		if(is_hash_map()) {
			if(as_hash_map()->empty()) return seq_first_t(NIL_NODE, false);
			auto e = as_hash_map()->first();
			return seq_first_t(new_node_vector(vector_va(e.first, e.second)), true);
		}
		if(is_hash_set()) {
			if(as_hash_set()->empty()) return seq_first_t(NIL_NODE, false);
			return seq_first_t(as_hash_set()->first_value(), true);
		}
		if(is_lazy_list()) {
			lazy_list_iterator_t lit(this);
			if(lit.done()) return seq_first_t(NIL_NODE, false);
			return seq_first_t(lit.val, true);
		}
		if(is_string()) {
			if(!t_string.size()) return seq_first_t(NIL_NODE, false);
			return seq_first_t(new_node_int(t_string.c_str()[0], NODE_FLAG_CHAR), true);
		}
		return seq_first_t(NIL_NODE, false);
	}

	// second
	typedef jo_pair<node_idx_t, bool> seq_second_t;
	seq_second_t seq_second() const {
		if(is_list()) return seq_second_t(t_list->second_value(), t_list->size() >= 2);
		if(is_vector()) return seq_second_t(as_vector()->nth(1), as_vector()->size() >= 2);
		if(is_hash_map()) {
			if(as_hash_map()->size() < 2) return seq_second_t(NIL_NODE, false);
			auto e = as_hash_map()->second();
			return seq_second_t(new_node_vector(vector_va(e.first, e.second)), true);
		}
		if(is_hash_set()) {
			if(as_hash_set()->size() < 2) return seq_second_t(NIL_NODE, false);
			return seq_second_t(as_hash_set()->second_value(), true);
		}
		if(is_lazy_list()) {
			lazy_list_iterator_t lit(this);
			if(lit.done()) return seq_second_t(NIL_NODE, false);
			lit.next();
			if(lit.done()) return seq_second_t(NIL_NODE, false);
			return seq_second_t(lit.val, true);
		}
		if(is_string()) {
			if(t_string.size() < 2) return seq_second_t(NIL_NODE, false);
			return seq_second_t(new_node_int(t_string.c_str()[1], NODE_FLAG_CHAR), true);
		}
		return seq_second_t(NIL_NODE, false);
	}

	typedef jo_pair<node_idx_t, bool> seq_rest_t;
	seq_rest_t seq_rest() const {
		if(is_list()) {
			if(t_list->empty()) return seq_rest_t(NIL_NODE, false);
			return seq_rest_t(new_node_list(t_list->rest()), true);
		}
		if(is_vector()) {
			if(as_vector()->empty()) return seq_rest_t(NIL_NODE, false);
			return seq_rest_t(new_node_vector(as_vector()->rest()), true);
		}
		if(is_hash_map()) {
			if(as_hash_map()->empty()) return seq_rest_t(NIL_NODE, false);
			return seq_rest_t(new_node_map(as_hash_map()->rest()), true);
		}
		if(is_hash_set()) {
			if(as_hash_set()->empty()) return seq_rest_t(NIL_NODE, false);
			return seq_rest_t(new_node_hash_set(as_hash_set()->rest()), true);
		}
		if(is_lazy_list()) {
			lazy_list_iterator_t lit(this);
			if(lit.done()) return seq_rest_t(NIL_NODE, false);
			return seq_rest_t(new_node_lazy_list(t_env, lit.next_fn()), true);
		}
		if(is_string()) {
			if(!t_string.size()) return seq_rest_t(NIL_NODE, false);
			return seq_rest_t(new_node_string(t_string.substr(1)), true);
		}
		return seq_rest_t(NIL_NODE, false);
	}

	// first, rest, more?
	typedef jo_triple<node_idx_t, node_idx_t, bool> seq_first_rest_t;
	seq_first_rest_t seq_first_rest() const {
		if(is_list()) {
			if(t_list->empty()) return seq_first_rest_t(NIL_NODE, NIL_NODE, false);
			return seq_first_rest_t(t_list->first_value(), new_node_list(t_list->rest()), true);
		}
		if(is_vector()) {
			if(as_vector()->empty()) return seq_first_rest_t(NIL_NODE, NIL_NODE, false);
			return seq_first_rest_t(as_vector()->first_value(), new_node_vector(as_vector()->rest()), true);
		} 
		if(is_hash_map()) {
			if(as_hash_map()->empty()) return seq_first_rest_t(NIL_NODE, NIL_NODE, false);
			auto e = as_hash_map()->first();
			return seq_first_rest_t(new_node_vector(vector_va(e.first, e.second)), new_node_map(as_hash_map()->rest()), true);
		}
		if(is_hash_set()) {
			if(as_hash_set()->empty()) return seq_first_rest_t(NIL_NODE, NIL_NODE, false);
			return seq_first_rest_t(as_hash_set()->first_value(), new_node_hash_set(as_hash_set()->rest()), true);
		}
		if(is_lazy_list()) {
			lazy_list_iterator_t lit(this);
			if(lit.done()) return seq_first_rest_t(NIL_NODE, NIL_NODE, false);
			return seq_first_rest_t(lit.val, new_node_lazy_list(t_env, lit.next_fn()), true);
		}
		if(is_string() && t_string.size()) {
			 if(!t_string.size()) return seq_first_rest_t(NIL_NODE, NIL_NODE, false);
			 return seq_first_rest_t(INT_0_NODE + t_string.c_str()[0], new_node_string(t_string.substr(1)), true);
		}
		return seq_first_rest_t(NIL_NODE, NIL_NODE, false);
	}

	bool seq_empty() const {
		if(is_list()) return t_list->empty();
		if(is_vector()) return as_vector()->empty();
		if(is_hash_map()) return as_hash_map()->size() == 0;
		if(is_hash_set()) return as_hash_set()->size() == 0;
		if(is_lazy_list()) {
			lazy_list_iterator_t lit(this);
			return lit.done();
		}
		if(is_string() && t_string.size()) return false;
		return true;
	}

	typedef jo_pair<node_idx_t, bool> seq_take_t;
	seq_take_t seq_take(size_t n) const {
		if(is_list()) {
			if(t_list->empty()) return seq_take_t(NIL_NODE, false);
			return seq_take_t(new_node_list(t_list->take(n)), true);
		} 
		if(is_vector()) {
			if(as_vector()->empty()) return seq_take_t(NIL_NODE, false);
			return seq_take_t(new_node_vector(as_vector()->take(n)), true);
		}
		if(is_hash_map()) {
			if(as_hash_map()->empty()) return seq_take_t(NIL_NODE, false);
			return seq_take_t(new_node_map(as_hash_map()->take(n)), true);
		}
		if(is_hash_set()) {
			if(as_hash_set()->empty()) return seq_take_t(NIL_NODE, false);
			return seq_take_t(new_node_hash_set(as_hash_set()->take(n)), true);
		}
		if(is_lazy_list()) {
			lazy_list_iterator_t lit(this);
			if(lit.done()) return seq_take_t(NIL_NODE, false);
			return seq_take_t(new_node_list(lit.all(n)), true);
		}
		if(is_string()) {
			if(!t_string.size()) return seq_take_t(NIL_NODE, false);
			return seq_take_t(new_node_string(t_string.substr(0, n)), true);
		}
		return seq_take_t(NIL_NODE, false);
	}

	node_idx_t seq_drop(size_t n) const {
		if(is_list()) return new_node_list(t_list->drop(n));
		if(is_vector()) return new_node_vector(as_vector()->drop(n));
		if(is_hash_map()) return new_node_map(as_hash_map()->drop(n));
		if(is_hash_set()) return new_node_hash_set(as_hash_set()->drop(n));
		if(is_lazy_list()) {
			lazy_list_iterator_t lit(this);
			return new_node_list(lit.all(n));
		}
		if(is_string() && t_string.size()) return new_node_string(t_string.substr(n));
		return NIL_NODE;
	}

	size_t seq_size() const {
		if(is_list()) return t_list->size();
		if(is_vector()) return as_vector()->size();
		if(is_hash_map()) return as_hash_map()->size();
		if(is_hash_set()) return as_hash_set()->size();
		if(is_lazy_list()) {
			lazy_list_iterator_t lit(this);
			return lit.all()->size();
		}
		if(is_string() && t_string.size()) return t_string.size();
		return NIL_NODE;
	}

	void seq_push_back(node_idx_t x) {
		if(is_list()) {
			t_list = t_list->push_back(x);
		} else if(is_vector()) {
			t_object = as_vector()->push_back(x).cast<jo_object>();
		} else if(is_hash_map()) {
			warnf("map.push_back: not implemented");
		} else if(is_hash_set()) {
			warnf("hash_set.push_back: not implemented");
		} else if(is_lazy_list()) {
			warnf("seq_push_back: not implemented for lazy lists");
		} else if(is_string()) {
			t_string += get_node_string(x);
		}
	}

	bool as_bool() const {
		switch(type) {
			case NODE_BOOL:   return t_bool;
			case NODE_INT:    return t_int != 0;
			case NODE_FLOAT:  return t_float != 0.0;
			case NODE_SYMBOL:
			case NODE_KEYWORD:
			case NODE_STRING: return t_string.length() != 0;
			case NODE_LIST:
			case NODE_LAZY_LIST:
			case NODE_VECTOR:
			case NODE_VECTOR2D:
			case NODE_HASH_SET:
			case NODE_MAP:    return true; // TODO
			default:          return false;
		}
	}

	long long as_int() const {
		switch(type) {
		case NODE_BOOL:   return t_bool;
		case NODE_INT:    return t_int;
		case NODE_FLOAT:  return (long long)t_float;
		case NODE_FUNC:
		case NODE_VAR:
		case NODE_SYMBOL:
		case NODE_KEYWORD:
		case NODE_STRING: return atoi(t_string.c_str());
		}
		return 0;
	}

	double as_float() const {
		switch(type) {
		case NODE_BOOL:   return t_bool;
		case NODE_INT:    return t_int;
		case NODE_FLOAT:  return t_float;
		case NODE_FUNC:
		case NODE_VAR:
		case NODE_SYMBOL:
		case NODE_KEYWORD:
		case NODE_STRING: return atof(t_string.c_str());
		}
		return 0;
	}

	jo_string as_string(int pretty = 0) const {
		switch(type) {
		case NODE_NIL:    return "nil";
		case NODE_BOOL:   return t_bool ? "true" : "false";
		case NODE_INT:    
			// only letter? TODO
		 	if((flags & NODE_FLAG_CHAR) && jo_isletter(t_int)) {
				if(pretty >= 2) {
					if(t_int == 32) return "\\space";
					if(t_int == 9) return "\\tab";
					if(t_int == 10) return "\\newline";
					if(t_int == 13) return "\\return";
					return "\\" + jo_string(t_int);
				}
				return jo_string(t_int);
			}
			return va("%lld", t_int);
		case NODE_FLOAT:  return va("%f", t_float);
		case NODE_LIST: 
			{
				jo_string s;
				s = '(';
				if(t_list.ptr) for(list_t::iterator it(t_list); it;) {
					s += get_node(*it)->as_string(3);
					++it;
					if(it) {
						s += " ";
					}
				}
				s += ')';
				return s;
			}
		case NODE_VECTOR:
			{
				jo_string s;
				s = '[';
				if(as_vector().ptr) for(auto it = as_vector()->begin(); it;) {
					s += get_node(*it)->as_string(3);
					++it;
					if(it) {
						s += " ";
					}
				}
				s += ']';
				return s;
			}
		case NODE_MAP:
			{
				jo_string s;
				s = '{';
				if(as_hash_map().ptr) for(auto it = as_hash_map()->begin(); it;) {
					s += get_node(it->first)->as_string(3);
					s += " ";
					s += get_node(it->second)->as_string(3);
					++it;
					if(it) {
						s += ", ";
					}
				}
				s += '}';
				return s;
			}
		case NODE_HASH_SET:
			{
				jo_string s;
				s = "#{";
				if(as_hash_set().ptr) for(auto it = as_hash_set()->begin(); it;) {
					s += get_node(it->first)->as_string(3);
					++it;
					if(it) {
						s += " ";
					}
				}
				s += '}';
				return s;
			}
		case NODE_LAZY_LIST:
			{
				int left = 1024;
				jo_string s;
				s = '(';
				for(lazy_list_iterator_t lit(this); !lit.done() && left; --left) {
					s += get_node(eval_node(lit.env, lit.val))->as_string(3);
					lit.next();
					if(!lit.done()) {
						s += " ";
					}
				}
				s += ')';
				return s;
			}
		case NODE_FUTURE:
			return get_node(deref())->as_string(pretty);
		}
		if(pretty >= 1 && type == NODE_KEYWORD) return ":" + t_string;
		if(pretty >= 3 && type == NODE_STRING) return "\"" + t_string + "\"";
		return t_string;
	}

	node_idx_t deref() const {
		if(type == NODE_FUTURE) {
			node_idx_t ret = t_atom.load();
			int count = 0;
			while(ret == TX_HOLD_NODE || ret == INV_NODE) {
				jo_yield_backoff(&count);
				ret = t_atom.load();
			}
			return ret;
		}
		return NIL_NODE;
	}

	const char *type_name() const {
		switch(type) {
		case NODE_NIL:	   return "nil";
		case NODE_BOOL:    return "bool";
		case NODE_INT:     return "int";
		case NODE_FLOAT:   return "float";
		case NODE_STRING:  return "string";
		case NODE_LIST:    return "list";
		case NODE_LAZY_LIST: return "lazy-list";
		case NODE_VECTOR:  return "vector";
		case NODE_VECTOR2D:  return "vector2d";
		case NODE_HASH_SET: return "set";
		case NODE_MAP:     return "map";
		case NODE_FUNC:	   return "function";
		case NODE_NATIVE_FUNC: return "native_function";
		case NODE_VAR:	   return "var";
		case NODE_SYMBOL:  return "symbol";
		case NODE_KEYWORD: return "keyword";
		case NODE_ATOM:	   return "atom";
		case NODE_DELAY:   return "delay";
		case NODE_FILE:	   return "file";
		case NODE_DIR:     return "dir";	
		case NODE_FUTURE:  return "future";
		case NODE_PROMISE: return "promise";
		}
		return "unknown";		
	}

	void print() const {
		printf("%s\n", as_string().c_str());
	}
};

static jo_pinned_vector<node_t> nodes;
static const int num_free_sectors = 8;
static jo_mpmcq<node_idx_unsafe_t, NIL_NODE, (1<<20)> free_nodes[num_free_sectors]; // available for allocation...
static const int num_garbage_sectors = 8;
static jo_mpmcq<node_idx_unsafe_t, NIL_NODE, (1<<20)> garbage_nodes[num_garbage_sectors]; // need resource release 

static inline void node_add_ref(node_idx_unsafe_t idx) { 
	if(idx >= START_USER_NODES) {
		node_t *n = &nodes[idx];
		int flags = n->flags;
		if((flags & (NODE_FLAG_PRERESOLVE|NODE_FLAG_FOREVER|NODE_FLAG_GARBAGE)) == 0) {
			int rc = n->ref_count.fetch_add(1);
			debugf("node_add_ref(%lld,%i): %s of type %s\n", idx, rc+1, n->as_string().c_str(), n->type_name());
		}
	}
}

static inline void node_release(node_idx_unsafe_t idx) { 
	if(idx >= START_USER_NODES) {
		node_t *n = &nodes[idx];
		int flags = n->flags;
		if((flags & (NODE_FLAG_PRERESOLVE|NODE_FLAG_FOREVER|NODE_FLAG_GARBAGE)) == 0) {
			int rc = n->ref_count.fetch_sub(1);
			debugf("node_release(%lld,%i): %s\n", idx, rc-1, n->as_string().c_str());
			if(rc <= 0) {
				printf("Error in ref count: node_release(%lld,%i): %s\n", idx, rc-1, n->as_string().c_str());
			}
			if(rc <= 1) {
				n->flags |= NODE_FLAG_GARBAGE;
				int sector = idx & (num_garbage_sectors-1);
				if(garbage_nodes[sector].full()) {
					n->release();
					free_nodes[sector].push(idx);
				} else {
					garbage_nodes[sector].push(idx);
				}
			}
		}
	}
}

static void collect_garbage(int sector) {
	tmProfileThread(0,0,0);
	node_idx_unsafe_t idx = INV_NODE;
	while(true) {
		std::this_thread::yield();
		if(garbage_nodes[sector].closing) {
			idx = NIL_NODE;
			break;
		} 
		while(garbage_nodes[sector].size() > 1) {
			idx = garbage_nodes[sector].pop();
			if(idx == NIL_NODE) break;
			node_t *n = &nodes[idx];
			n->release();
			free_nodes[sector].push(idx);
		}
		if(idx == NIL_NODE) break;
	}
}

static inline node_t *get_node(node_idx_t idx) {
	 //assert(!(nodes[idx].flags & NODE_FLAG_GARBAGE));
	 if(nodes[idx].flags & NODE_FLAG_GARBAGE) {
		idx = NIL_NODE;
	 }
	 return &nodes[idx]; 
}
static inline int get_node_type(node_idx_t idx) { return get_node(idx)->type; }
static inline int get_node_type(const node_t *n) { return n->type; }
static inline int get_node_flags(node_idx_t idx) { return get_node(idx)->flags; }
static inline jo_string get_node_string(node_idx_t idx) { return get_node(idx)->as_string(); }
static inline node_idx_t get_node_var(node_idx_t idx) { return get_node(idx)->t_extra; }
static inline bool get_node_bool(node_idx_t idx) { return get_node(idx)->as_bool(); }
static inline list_ptr_t get_node_list(node_idx_t idx) { return get_node(idx)->as_list(); }
static inline vector_ptr_t get_node_vector(node_idx_t idx) { return get_node(idx)->as_vector(); }
static inline hash_map_ptr_t get_node_map(node_idx_t idx) { return get_node(idx)->as_hash_map(); }
static inline hash_set_ptr_t get_node_set(node_idx_t idx) { return get_node(idx)->as_hash_set(); }
static inline long long get_node_int(node_idx_t idx) { 
	if(idx >= INT_0_NODE && idx <= INT_256_NODE) return idx - INT_0_NODE;
	return get_node(idx)->as_int(); 
}
static inline double get_node_float(node_idx_t idx) { return get_node(idx)->as_float(); }
static inline const char *get_node_type_string(node_idx_t idx) { return get_node(idx)->type_name(); }
static inline vector_ptr_t get_node_func_args(node_idx_t idx) { return get_node(idx)->t_func.args; }
static inline list_ptr_t get_node_func_body(node_idx_t idx) { return get_node(idx)->t_func.body; }
static inline node_idx_t get_node_lazy_fn(node_idx_t idx) { return get_node(idx)->t_extra; }
static inline node_idx_t get_node_lazy_fn(const node_t *n) { return n->t_extra; }
static inline FILE *get_node_file(node_idx_t idx) { return get_node(idx)->t_file; }
static inline void *get_node_dir(node_idx_t idx) { return get_node(idx)->t_dir; }
static inline atomic_node_idx_t &get_node_atom(node_idx_t idx) { return get_node(idx)->t_atom; }
static inline env_ptr_t get_node_env(node_idx_t idx) { return get_node(idx)->t_env; }
static inline env_ptr_t get_node_env(const node_t *n) { return n->t_env; }

// TODO: Should prefer to allocate nodes next to existing nodes which will be linked (for cache coherence)
static inline node_idx_t new_node(node_t &&n) {
	// TODO: need try-pop really...
	int sector = jo_pcg32(&jo_rnd_state) % num_free_sectors;
	if(free_nodes[sector].size() > processor_count * 2) {
		node_idx_unsafe_t ni = free_nodes[sector].pop();
		if(ni >= START_USER_NODES) {
			node_t *nn = &nodes[ni];
			*nn = std::move(n);
			return ni;
		}
	}
	return nodes.push_back(std::move(n));
}

static node_idx_t new_node(int type, int flags) {
	node_t n;
	n.type = type;
	n.flags = flags;
	return new_node(std::move(n));
}

static node_idx_t new_node_list(list_ptr_t nodes, int flags) {
	node_idx_t idx = new_node(NODE_LIST, flags);
	get_node(idx)->t_list = nodes;
	return idx;
}

static node_idx_t new_node_object(int type, object_ptr_t object, int flags) {
	node_idx_t idx = new_node(type, flags);
	get_node(idx)->t_object = object;
	return idx;
}

static node_idx_t new_node_map(hash_map_ptr_t nodes, int flags) { return new_node_object(NODE_MAP, nodes.cast<jo_object>(), flags); }
static node_idx_t new_node_hash_set(hash_set_ptr_t nodes, int flags) { return new_node_object(NODE_HASH_SET, nodes.cast<jo_object>(), flags); }
static node_idx_t new_node_vector(vector_ptr_t nodes, int flags) { return new_node_object(NODE_VECTOR, nodes.cast<jo_object>(), flags); }
static node_idx_t new_node_vector2d(vector2d_ptr_t nodes, int flags) { return new_node_object(NODE_VECTOR2D, nodes.cast<jo_object>(), flags); }

static node_idx_t new_node_lazy_list(env_ptr_t env, node_idx_t lazy_fn, int flags) {
	node_idx_t idx = new_node(NODE_LAZY_LIST, NODE_FLAG_LAZY | flags);
	get_node(idx)->t_extra = lazy_fn;
	get_node(idx)->t_env = env;
	return idx;
}

static node_idx_t new_node_native_function(native_function_t f, bool is_macro, int flags=0) {
	node_t n;
	n.type = NODE_NATIVE_FUNC;
	n.t_nfunc_raw = f;
	n.flags = flags;
	n.flags |= is_macro ? NODE_FLAG_MACRO : 0;
	n.flags |= NODE_FLAG_LITERAL;
	return new_node(std::move(n));
}

static node_idx_t new_node_native_function(std::function<node_idx_t(env_ptr_t,list_ptr_t)> f, bool is_macro, int flags=0) {
	node_t n;
	n.type = NODE_NATIVE_FUNC;
	n.t_native_function = new native_func_t(f);
	n.flags = flags;
	n.flags |= is_macro ? NODE_FLAG_MACRO : 0;
	n.flags |= NODE_FLAG_LITERAL;
	return new_node(std::move(n));
}

static node_idx_t new_node_native_function(const char *name, native_function_t f, bool is_macro, int flags=0) {
	node_t n;
	n.type = NODE_NATIVE_FUNC;
	n.t_nfunc_raw = f;
	n.t_string = name;
	n.flags = flags;
	n.flags |= is_macro ? NODE_FLAG_MACRO : 0;
	n.flags |= NODE_FLAG_LITERAL;
	return new_node(std::move(n));
}

static node_idx_t new_node_native_function(const char *name, std::function<node_idx_t(env_ptr_t,list_ptr_t)> f, bool is_macro, int flags=0) {
	node_t n;
	n.type = NODE_NATIVE_FUNC;
	n.t_native_function = new native_func_t(f);
	n.t_string = name;
	n.flags = flags;
	n.flags |= is_macro ? NODE_FLAG_MACRO : 0;
	n.flags |= NODE_FLAG_LITERAL;
	return new_node(std::move(n));
}

static node_idx_t new_node_bool(bool b) {
	return b ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t new_node_int(long long i, int flags) {
	if(i >= 0 && i <= 256 && flags == 0) {
		return INT_0_NODE + i;
	}
	node_t n;
	n.type = NODE_INT;
	n.t_int = i;
	n.flags = NODE_FLAG_LITERAL | flags;
	return new_node(std::move(n));
}

static node_idx_t new_node_float(double f, int flags = 0) {
	node_t n;
	n.type = NODE_FLOAT;
	n.t_float = f;
	n.flags = NODE_FLAG_LITERAL | flags;
	return new_node(std::move(n));
}

static node_idx_t new_node_string(const jo_string &s, int flags) {
	node_t n;
	n.type = NODE_STRING;
	n.t_string = s;
	n.flags = NODE_FLAG_LITERAL | NODE_FLAG_STRING | flags;
	return new_node(std::move(n));
}

static node_idx_t new_node_symbol(const jo_string &s, int flags=0) {
	node_t n;
	n.type = NODE_SYMBOL;
	n.t_string = s;
	n.flags = NODE_FLAG_STRING | flags;
	return new_node(std::move(n));
}

static node_idx_t new_node_keyword(const jo_string &s, int flags=0) {
	node_t n;
	n.type = NODE_KEYWORD;
	n.t_string = s;
	n.flags = NODE_FLAG_LITERAL | NODE_FLAG_STRING | flags;
	return new_node(std::move(n));
}

static node_idx_t new_node_var(const jo_string &name, node_idx_t value, int flags) {
	node_t n;
	n.type = NODE_VAR;
	n.t_string = name;
	n.t_extra = value;
	n.flags = flags;
	return new_node(std::move(n));
}

static node_idx_t new_node_file(FILE *fp, int flags = 0) {
	node_t n;
	n.type = NODE_FILE;
	n.t_file = fp;
	n.flags = flags;
	return new_node(std::move(n));
}

static node_idx_t new_node_dir(void *dp, int flags = 0) {
	node_t n;
	n.type = NODE_DIR;
	n.t_dir = dp;
	n.flags = flags;
	return new_node(std::move(n));
}

static node_idx_t new_node_atom(node_idx_t atom, int flags = 0) {
	node_t n;
	n.type = NODE_ATOM;
	n.t_atom = atom;
	n.flags = flags;
	return new_node(std::move(n));
}

static int is_whitespace(int c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
static int is_num(int c) { return (c >= '0' && c <= '9'); }
static int is_alnum(int c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static int is_separator(int c) { return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == ','; }

enum token_type_t {
	TOK_EOF=0,
	TOK_STRING,
	TOK_SYMBOL,
	TOK_KEYWORD,
	TOK_SEPARATOR,
};

struct token_t {
	token_type_t type;
	jo_string str;
	int line;
};

struct parse_state_t {
	FILE *fp = 0;
	const char *buf = 0;
	const char *buf_end = 0;
	char unget_buf[32];
	int unget_cnt = 0;
	int line_num = 1;
	int getc() {
		int c;
		if(unget_cnt) c = unget_buf[--unget_cnt];
		else if(fp) c = fgetc(fp);
		else if(buf && buf < buf_end) c = *buf++;
		else c = EOF;
		if(c == '\n') {
			line_num++;
		}
		return c;
	}
	void ungetc(int c) {
		if(c == '\n') {
			line_num--;
		};
		assert(unget_cnt < sizeof(unget_buf));
		unget_buf[unget_cnt++] = (char)c;
	}
};

static token_t get_token(parse_state_t *state) {
	// skip leading whitepsace and comma
	do {
		int c = state->getc();
		if(!is_whitespace(c) && c != ',') {
			state->ungetc(c);
			break;
		}
	} while(true);

	token_t tok;
	tok.line = state->line_num;

	int c = state->getc();
	
	// handle special case of end of file reached
	if(c == EOF) {
		tok.type = TOK_EOF;
		debugf("token: EOF\n");
		return tok;
	}
 
	/*
	if(c == '\\') {
		tok.type = TOK_SYMBOL;
		int val = state->getc();
		tok.str = va("%i", val);
		return tok;
	}
	*/
	if(c == '"') {
		tok.type = TOK_STRING;
		// string literal
		do {
			int C = state->getc();
			if (C == EOF) {
				fprintf(stderr, "unterminated string on line %i\n", state->line_num);
				exit(__LINE__);
			}
			// escape next character
			if(C == '\\') {
				C = state->getc();
				if(C == EOF) {
					fprintf(stderr, "unterminated string on line %i\n", state->line_num);
					exit(__LINE__);
				}
				switch(C) {
				case 'n': C = '\n'; break;
				case 't': C = '\t'; break;
				case 'r': C = '\r'; break;
				case '\\': C = '\\'; break;
				case '"': C = '"'; break;
				default:
					fprintf(stderr, "unknown escape sequence \\%c on line %i\n", C, state->line_num);
					exit(__LINE__);
				}

			} else if(C == '"') {
				break;
			}
			tok.str += C;
		} while (true);
		debugf("token: %s\n", tok.str.c_str());
		return tok;
	}
	if(c == '#') {
		int C = state->getc();
		if(C == '(') {
			// shorthand for inline function
			tok.type = TOK_SEPARATOR;
			tok.str = "__fn";
			debugf("token: %s\n", tok.str.c_str());
			return tok;
		} else {
			state->ungetc(C);
		}
	}
	if(c == '\'') {
		int C = state->getc();
		if(C == '(') {
			tok.type = TOK_SEPARATOR;
			tok.str = "quote";
			debugf("token: %s\n", tok.str.c_str());
			return tok;
		}
		tok.type = TOK_STRING;
		// string literal of a symbol
		do {
			if(is_whitespace(C) || is_separator(C) || C == EOF) {
				state->ungetc(C);
				break;
			}
			tok.str += C;
			C = state->getc();
		} while (true);
		debugf("token: %s\n", tok.str.c_str());
		return tok;
	}
	if(c == '#') {
		int C = state->getc();
		if(C == '{') {
			tok.type = TOK_SEPARATOR;
			tok.str = "hash-set";
			debugf("token: %s\n", tok.str.c_str());
			return tok;
		}
	}
	if(c == '`') {
		tok.type = TOK_SEPARATOR;
		tok.str = "quasiquote";
		debugf("token: %s\n", tok.str.c_str());
		return tok;
	}
	if(c == '~') {
		tok.type = TOK_SEPARATOR;
		int C = state->getc();
		if(C == '@') {
			// shorthand for inline function
			tok.str = "unquote-splice";
			debugf("token: %s\n", tok.str.c_str());
			return tok;
		} else {
			state->ungetc(C);
		}
		tok.str = "unquote";
		debugf("token: %s\n", tok.str.c_str());
		return tok;
	}
	if(c == '@') {
		tok.type = TOK_SEPARATOR;
		tok.str = "deref";
		debugf("token: %s\n", tok.str.c_str());
		return tok;
	}
	if(c == ':') {
		tok.type = TOK_KEYWORD;
		// string literal of a keyword
		int C = state->getc();
		do {
			if(is_whitespace(C) || is_separator(C) || C == EOF) {
				state->ungetc(C);
				break;
			}
			tok.str += C;
			C = state->getc();
		} while (true);
		debugf("token: %s\n", tok.str.c_str());
		return tok;
	}
	if(c == ';') {
		// comment (skip)
		do {
			int C = state->getc();
			if (C == EOF) {
				fprintf(stderr, "unterminated comment\n");
				exit(__LINE__);
			}
			if(C == '\n') break;
		} while(true);
		return get_token(state); // recurse
	}
	if(is_separator(c)) {
		tok.type = TOK_SEPARATOR;
		// vector, list, map, set
		tok.str += c;
		debugf("token: %s\n", tok.str.c_str());
		return tok;
	}

	// while not whitespace, or separator, get characters
	tok.type = TOK_SYMBOL;
	do {
		if(is_whitespace(c) || is_separator(c) || c == EOF) {
			state->ungetc(c);
			break;
		}
		tok.str += (char)c;
		c = state->getc();
	} while(true);
	
	if(tok.str.c_str()[0] == '\\') {
		// escape sequence
		if(tok.str == "\\space") tok.str = "32";
		else tok.str = va("%i", tok.str.c_str()[1]);
		debugf("token: %s\n", tok.str.c_str());
		return tok;
	}
	
	debugf("token: %s\n", tok.str.c_str());

	return tok;
}

static list_ptr_t get_symbols_list_r(list_ptr_t list);
static list_ptr_t get_symbols_vector_r(vector_ptr_t list);

static list_ptr_t get_symbols_vector_r(vector_ptr_t list) {
	list_ptr_t symbol_list = new_list();
	for(vector_t::iterator it = list->begin(); it; it++) {
		int type = get_node_type(*it);
		if(type == NODE_SYMBOL) {
			symbol_list->push_back_inplace(*it);			
		} else if(type == NODE_MAP) {
			printf("TODO: map @ %i\n", __LINE__);
		} else if(type == NODE_VECTOR) {
			if(get_node(*it)->as_vector().ptr) {
				list_ptr_t sub_list = get_symbols_vector_r(get_node(*it)->as_vector());
				if(sub_list->length) {
					symbol_list->conj_inplace(*sub_list);
				}
			}
		} else {
			if(get_node(*it)->t_list.ptr) {
				list_ptr_t sub_list = get_symbols_list_r(get_node(*it)->t_list);
				if(sub_list->length) {
					symbol_list->conj_inplace(*sub_list);
				}
			}
		}
	}
	return symbol_list;
}

static list_ptr_t get_symbols_list_r(list_ptr_t list) {
	list_ptr_t symbol_list = new_list();
	for(list_t::iterator it(list); it; it++) {
		int type = get_node_type(*it);
		if(type == NODE_SYMBOL) {
			symbol_list->push_back_inplace(*it);			
		} else if(type == NODE_MAP) {
			printf("TODO: map @ %i\n", __LINE__);
		} else if(type == NODE_VECTOR) {
			if(get_node(*it)->as_vector().ptr) {
				list_ptr_t sub_list = get_symbols_vector_r(get_node(*it)->as_vector());
				if(sub_list->length) {
					symbol_list->conj_inplace(*sub_list);
				}
			}
		} else {
			if(get_node(*it)->t_list.ptr) {
				list_ptr_t sub_list = get_symbols_list_r(get_node(*it)->t_list);
				if(sub_list->length) {
					symbol_list->conj_inplace(*sub_list);
				}
			}
		}
	}
	return symbol_list;
}

static node_idx_t parse_next(env_ptr_t env, parse_state_t *state, int stop_on_sep) {
	token_t tok = get_token(state);
	debugf("parse_next \"%s\", with '%c'\n", tok.str.c_str(), stop_on_sep);

	if(tok.type == TOK_EOF) {
		// end of list
		return INV_NODE;
	}

	const char *tok_ptr = tok.str.c_str();
	const char *tok_ptr_end = tok_ptr + tok.str.size();
	int c = tok_ptr[0];
	int c2 = tok_ptr[1];

	if(c == stop_on_sep) {
		// end of list
		return INV_NODE;
	}

	if(tok.type == TOK_SEPARATOR && c != stop_on_sep && (c == ')' || c == ']' || c == '}')) {
		fprintf(stderr, "unexpected separator '%c', was expecting '%c' on line %i\n", c, stop_on_sep, tok.line);
		return INV_NODE;
	}

	// 
	// literals
	//

	// container types
	//if(c == '#' && c2 == '"') return parse_regex(state);
	// parse number...
	if(tok.type == TOK_STRING) {
		debugf("string: %s\n", tok.str.c_str());
		return new_node_string(tok.str.c_str(), NODE_FLAG_FOREVER);
	} 
	if(is_num(c) || (c == '-' && is_num(c2))) {
		// floating point
		if(tok.str.find('.') != jo_npos) {
			double float_val = atof(tok_ptr);
			debugf("float: %f\n", float_val);
			return new_node_float(float_val, NODE_FLAG_FOREVER);
		}

		int int_val = 0;
		// 0x hexadecimal
		if(c == '0' && (c2 == 'x' || c2 == 'X')) {
			tok_ptr += 2;
			// parse hex from tok_ptr
			while(is_alnum(*tok_ptr)) {
				int_val <<= 4;
				int_val += *tok_ptr - '0';
				if(*tok_ptr >= 'a' && *tok_ptr <= 'f') int_val -= 'a' - '0' - 10; 
				if(*tok_ptr >= 'A' && *tok_ptr <= 'F') int_val -= 'A' - '0' - 10;
				tok_ptr++;
			}
		}
		// 0b binary
		else if(c == '0' && (c2 == 'b' || c2 == 'B')) {
			tok_ptr += 2;
			// parse binary from tok_ptr
			while(is_alnum(*tok_ptr)) {
				int_val <<= 1;
				int_val += *tok_ptr - '0';
				tok_ptr++;
			}
		}
		// 0 octal
		else if(c == '0' && tok.str.size() > 1) {
			tok_ptr += 1;
			// parse octal from tok_ptr
			while(is_alnum(*tok_ptr)) {
				int_val <<= 3;
				int_val += *tok_ptr - '0';
				tok_ptr++;
			}
		}
		else {
			int_val = atoi(tok_ptr);
		}
		// Create a new number node
		debugf("int: %d\n", int_val);
		return new_node_int(int_val, NODE_FLAG_FOREVER);
	} 
	if(tok.type == TOK_KEYWORD) {
		debugf("keyword: %s\n", tok.str.c_str());
		if(tok.str == "else") return K_ELSE_NODE;
		if(tok.str == "when") return K_WHEN_NODE;
		if(tok.str == "while") return K_WHILE_NODE;
		if(tok.str == "let") return K_LET_NODE;
		if(tok.str == "__PC__") return K_PC_NODE;
		if(tok.str == "__ALL__") return K_ALL_NODE;
		if(tok.str == "__BY__") return K_BY_NODE;
		if(tok.str == "__AUTO_DEREF__") return K_AUTO_DEREF_NODE;
		return new_node_keyword(tok.str.c_str(), NODE_FLAG_FOREVER);
	}
	if(tok.type == TOK_SYMBOL) {
		debugf("symbol: %s\n", tok.str.c_str());
		if(env->has(tok.str.c_str())) {
			node_idx_t node = env->get(tok.str.c_str()).value;
			if(get_node_flags(node) & NODE_FLAG_PRERESOLVE) {
				debugf("pre-resolve symbol: %s\n", tok.str.c_str());
				return node;
			}
		}
		// fixed symbols
		if(tok.str == "%") return PCT_NODE;
		if(tok.str == "%1") return PCT1_NODE;
		if(tok.str == "%2") return PCT2_NODE;
		if(tok.str == "%3") return PCT3_NODE;
		if(tok.str == "%4") return PCT4_NODE;
		if(tok.str == "%5") return PCT5_NODE;
		if(tok.str == "%6") return PCT6_NODE;
		if(tok.str == "%7") return PCT7_NODE;
		if(tok.str == "%8") return PCT8_NODE;
		return new_node_symbol(tok.str.c_str(), NODE_FLAG_FOREVER);
	} 

	// parse quote shorthand
	if(tok.type == TOK_SEPARATOR && tok.str == "quote") {
		debugf("list begin\n");
		node_idx_t next = parse_next(env, state, ')');
		if(next == INV_NODE) {
			return EMPTY_LIST_NODE;
		}
		node_t n;
		n.type = NODE_LIST;
		n.flags = NODE_FLAG_FOREVER;
		n.t_list = new_list();
		n.t_list->push_back_inplace(env->get("quote").value);
		while(next != INV_NODE) {
			n.t_list->push_back_inplace(next);
			next = parse_next(env, state, ')');
		}
		debugf("list end\n");
		return new_node(std::move(n));
	}

	// parse hash-set shorthand
	if(tok.type == TOK_SEPARATOR && tok.str == "hash-set") {
		debugf("hash-set begin\n");
		node_idx_t next = parse_next(env, state, '}');
		if(next == INV_NODE) {
			return EMPTY_SET_NODE;
		}
		node_t n;
		n.type = NODE_HASH_SET;
		n.flags = NODE_FLAG_FOREVER;
		n.as_hash_set() = new_hash_set();
		int common_flags = ~0;
		while(next != INV_NODE) {
			common_flags &= get_node_flags(next);
			n.as_hash_set() = n.as_hash_set()->assoc(next, node_eq);
			next = parse_next(env, state, '}');
		}
		if(common_flags & NODE_FLAG_LITERAL) {
			n.flags |= NODE_FLAG_LITERAL;
		}
		debugf("hash-set end\n");
		return new_node(std::move(n));
	}

	// parse unquote shorthand
	if(tok.type == TOK_SEPARATOR && tok.str == "unquote") {
		node_idx_t inner = parse_next(env, state, stop_on_sep);
		if(inner == INV_NODE) {
			return INV_NODE;
		}
		node_t n;
		n.type = NODE_LIST;
		n.flags = NODE_FLAG_FOREVER;
		n.t_list = new_list();
		n.t_list->push_back_inplace(UNQUOTE_NODE);
		n.t_list->push_back_inplace(inner);
		return new_node(std::move(n));
	}

	// parse unquote shorthand
	if(tok.type == TOK_SEPARATOR && tok.str == "unquote-splice") {
		node_idx_t inner = parse_next(env, state, stop_on_sep);
		if(inner == INV_NODE) {
			return INV_NODE;
		}
		node_t n;
		n.type = NODE_LIST;
		n.flags = NODE_FLAG_FOREVER;
		n.t_list = new_list();
		n.t_list->push_back_inplace(UNQUOTE_SPLICE_NODE);
		n.t_list->push_back_inplace(inner);
		return new_node(std::move(n));
	}

	if(tok.type == TOK_SEPARATOR && tok.str == "quasiquote") {
		node_idx_t inner = parse_next(env, state, stop_on_sep);
		if(inner == INV_NODE) {
			return INV_NODE;
		}
		node_t n;
		n.type = NODE_LIST;
		n.flags = NODE_FLAG_FOREVER;
		n.t_list = new_list();
		n.t_list->push_back_inplace(env->get("quasiquote").value);
		n.t_list->push_back_inplace(inner);
		return new_node(std::move(n));
	}

	if(tok.type == TOK_SEPARATOR && tok.str == "deref") {
		node_idx_t inner = parse_next(env, state, stop_on_sep);
		if(inner == INV_NODE) {
			return INV_NODE;
		}
		node_t n;
		n.type = NODE_LIST;
		n.flags = NODE_FLAG_FOREVER;
		n.t_list = new_list();
		n.t_list->push_back_inplace(env->get("deref").value);
		n.t_list->push_back_inplace(inner);
		return new_node(std::move(n));
	}

	// anonymous function shorthand. 
	// Note: Analyze the function tree at parse time? I think this is correct?
	if(tok.type == TOK_SEPARATOR && tok.str == "__fn") {
		debugf("list begin\n");
		node_idx_t next = parse_next(env, state, ')');
		if(next == INV_NODE) {
			return EMPTY_LIST_NODE;
		}
		node_t n;
		n.type = NODE_LIST;
		n.flags = NODE_FLAG_FOREVER;
		n.t_list = new_list();
		n.t_list->push_back_inplace(env->get("fn").value);
		list_ptr_t body = new_list();
		while(next != INV_NODE) {
			body->push_back_inplace(next);
			next = parse_next(env, state, ')');
		}
		list_ptr_t symbol_list = get_symbols_list_r(body);
		vector_ptr_t arg_list = new_vector();
		int num_args_used = 0;
		for(list_t::iterator it(symbol_list); it; ++it) {
			jo_string sym = get_node_string(*it);
			if(sym == "%") {
				num_args_used = jo_max(num_args_used, 1);
			} else if(sym.substr(0, 1) == "%") {
				num_args_used = jo_max(num_args_used, atoi(sym.substr(1).c_str()));
			}
		}
		if(num_args_used == 1) {
			arg_list->push_back_inplace(new_node_symbol("%"));
		} else {
			for(int i = 1; i <= num_args_used; ++i) {
				jo_stringstream ss;
				ss << "%" << i;
				arg_list->push_back_inplace(new_node_symbol(ss.str().c_str()));
			}
		}
		n.t_list->push_back_inplace(new_node_vector(arg_list));
		n.t_list->push_back_inplace(new_node_list(body));
		debugf("list end\n");
		//print_node(new_node(&n));
		return new_node(std::move(n));
	}

	// parse list
	if(c == '(') {
		debugf("list begin\n");
		node_idx_t next = parse_next(env, state, ')');
		if(next == INV_NODE) {
			return EMPTY_LIST_NODE;
		}
		node_t n;
		n.type = NODE_LIST;
		n.flags = NODE_FLAG_FOREVER;
		n.t_list = new_list();
		int common_flags = ~0;
		bool is_native_fn = get_node_type(next) == NODE_NATIVE_FUNC;
		while(next != INV_NODE) {
			common_flags &= get_node_flags(next);
			n.t_list->push_back_inplace(next);
			next = parse_next(env, state, ')');
		}
		if(common_flags & NODE_FLAG_LITERAL) {
			if(is_native_fn) {
				n.flags |= NODE_FLAG_LITERAL_ARGS;
			} else {
				n.flags |= NODE_FLAG_LITERAL;
			}
		}
		debugf("list end\n");
		return new_node(std::move(n));
	}

	// parse vector
	if(c == '[') {
		debugf("vector begin\n");
		node_idx_t next = parse_next(env, state, ']');
		if(next == INV_NODE) {
			debugf("vector end\n");
			return EMPTY_VECTOR_NODE;
		}
		node_t n;
		n.type = NODE_VECTOR;
		n.flags = NODE_FLAG_FOREVER;
		vector_ptr_t v = new_vector();
		int common_flags = ~0;
		while(next != INV_NODE) {
			common_flags &= get_node_flags(next);
			v->push_back_inplace(next);
			next = parse_next(env, state, ']');
		}
		if(common_flags & NODE_FLAG_LITERAL) {
			n.flags |= NODE_FLAG_LITERAL;
		}
		n.t_object = v.cast<jo_object>();
		// @ If its all the same type, convert to simple array of values of type
		debugf("vector end\n");
		return new_node(std::move(n));
	}

	// parse map (hashmap)
	// like a list, but with keys and values alternating
	if(c == '{') {
		debugf("map begin\n");
		node_t n;
		n.type = NODE_MAP;
		n.flags = NODE_FLAG_FOREVER;
		hash_map_ptr_t m = new_hash_map();
		node_idx_t next = parse_next(env, state, '}');
		node_idx_t next2 = next != INV_NODE ? parse_next(env, state, '}') : INV_NODE;
		if(next == INV_NODE || next2 == INV_NODE) {
			debugf("map end\n");
			return EMPTY_MAP_NODE;
		}
		while(next != INV_NODE && next2 != INV_NODE) {
			m = m->assoc(next, next2, node_eq);
			next = parse_next(env, state, '}');
			next2 = next != INV_NODE ? parse_next(env, state, '}') : INV_NODE;
		}
		n.t_object = m.cast<jo_object>();
		debugf("map end\n");
		return new_node(std::move(n));
	}

	return INV_NODE;
}


// eval a list of nodes
static node_idx_t eval_list(env_ptr_t env, list_ptr_t list, int list_flags) {
	list_t::iterator it(list);
	if(!it) {
		return EMPTY_LIST_NODE;
	}
	node_idx_t n1i = *it++;
	int n1_type = get_node_type(n1i);
	int n1_flags = get_node_flags(n1i);
	if(n1_type == NODE_LIST 
	|| n1_type == NODE_SYMBOL 
	|| n1_type == NODE_KEYWORD 
	|| n1_type == NODE_STRING 
	|| n1_type == NODE_NATIVE_FUNC
	|| n1_type == NODE_FUNC
	|| n1_type == NODE_MAP
	|| n1_type == NODE_HASH_SET
	|| n1_type == NODE_VECTOR
	) {
		node_idx_t sym_idx = n1i;
		int sym_type = n1_type;
		int sym_flags = n1_flags;
		if(n1_type == NODE_LIST) {
			sym_idx = eval_list(env, get_node(n1i)->t_list);
			sym_type = get_node_type(sym_idx);
		} else if(n1_type == NODE_SYMBOL) {
			sym_idx = env->get(get_node_string(n1i)).value;
			sym_type = get_node_type(sym_idx);
		}
		sym_flags = get_node(sym_idx)->flags;
		node_t *sym_node = get_node(sym_idx);

		// get the symbol's value
		if(sym_type == NODE_NATIVE_FUNC) {
			debugf("nativefn: %s\n", get_node_string(sym_idx).c_str());
			if((sym_flags|list_flags) & (NODE_FLAG_MACRO|NODE_FLAG_LITERAL_ARGS)) {
				if(sym_node->t_nfunc_raw) {
					return sym_node->t_nfunc_raw(env, list->rest());
				}
				return (*sym_node->t_native_function.ptr)(env, list->rest());
			}

			list_ptr_t args = new_list();
			for(; it; it++) {
				args->push_back_inplace(eval_node(env, *it));
			}
			// call the function
			if(sym_node->t_nfunc_raw) {
				return sym_node->t_nfunc_raw(env, args);
			} else {
				return (*sym_node->t_native_function.ptr)(env, args);
			}
		} else if(sym_type == NODE_FUNC || sym_type == NODE_DELAY) {
			if(sym_type == NODE_DELAY && sym_node->t_extra != INV_NODE) {
				return sym_node->t_extra;
			}

			vector_ptr_t proto_args = sym_node->t_func.args;
			list_ptr_t proto_body = sym_node->t_func.body;
			env_ptr_t proto_env = sym_node->t_env;
			list_ptr_t args1(list->rest());
			env_ptr_t fn_env = new_env(proto_env);

			fn_env->tx = env->tx;

			if(proto_args.ptr) {
				int is_macro = (sym_flags|list_flags) & NODE_FLAG_MACRO;
				vector_t::iterator i = proto_args->begin();
				list_t::iterator i2(args1);
				for (; i && i2; i++, i2++) {
					node_let(fn_env, *i, is_macro ? *i2 : eval_node(env, *i2));
				}
			}

			// Evaluate all statements in the body list
			node_idx_t last = NIL_NODE;
			for(list_t::iterator i(proto_body); i; i++) {
				last = eval_node(fn_env, *i);
			}

			node_t *last_node = get_node(last);
			while(last_node->type == NODE_RECUR) {
				auto proto_it = proto_args->begin();
				list_t::iterator recur_it(last_node->t_list);
				for(; proto_it && recur_it; ) {
					node_let(fn_env, *proto_it++, *recur_it++);
				}
				for(list_t::iterator i(proto_body); i; i++) {
					last = eval_node(fn_env, *i);
				}
				last_node = get_node(last);
			}

			if(sym_type == NODE_DELAY) {
				sym_node->t_extra = last;
			}
			return last;
		} else if(sym_type == NODE_MAP) {
			if(it) {
				// lookup the key in the map
				node_idx_t n2i = eval_node(env, *it++);
				node_idx_t n3i = it ? eval_node(env, *it++) : NIL_NODE;
				auto it2 = sym_node->as_hash_map()->find(n2i, node_eq);
				if(it2.third) {
					return it2.second;
				}
				return n3i;
			}
		} else if(sym_type == NODE_HASH_SET) {
			if(it) {
				// lookup the key in the map
				node_idx_t n2i = eval_node(env, *it++);
				node_idx_t n3i = it ? eval_node(env, *it++) : NIL_NODE;
				auto it2 = sym_node->as_hash_set()->find(n2i, node_eq);
				if(it2.second) {
					return it2.first;
				}
				return n3i;
			}
		} else if(sym_type == NODE_KEYWORD) {
			if(it) {
				// lookup the key in the map
				node_idx_t n2i = eval_node(env, *it++);
				node_idx_t n3i = it ? eval_node(env, *it++) : NIL_NODE;
				if(get_node_type(n2i) == NODE_MAP) {
					auto it2 = get_node(n2i)->as_hash_map()->find(sym_idx, node_eq);
					if(it2.third) {
						return it2.second;
					}
				}
				return n3i;
			}
		} else if(sym_type == NODE_VECTOR) {
			if(it) return sym_node->as_vector()->nth(get_node_int(eval_node(env, *it++)));
		}
	}

	// eval the list
	/*
	{
		list_ptr_t ret = new_list();
		for(list_t::iterator it(list); it; it++) {
			ret->push_back_inplace(eval_node(env, *it));
		}
		return new_node_list(ret);
	}
	*/

	return new_node_list(list);
}

static node_idx_t eval_node(env_ptr_t env, node_idx_t root) {
	node_t *node = get_node(root);
	int flags = node->flags;
	if(flags & NODE_FLAG_LITERAL) { return root; }

	int type = node->type;
	if(type == NODE_LIST) {
		return eval_list(env, get_node(root)->t_list, flags);
	} else if(type == NODE_SYMBOL) {
		auto sym = env->get(get_node_string(root));
		if(!sym.valid) {
			return root;
		}
		return sym.value;//eval_node(env, sym_idx);
	} else if(type == NODE_VECTOR) {
		if(flags & NODE_FLAG_LITERAL) { return root; }
		// TODO: some way to quick resolve the vector? IE, know exactly which ones are things that need to be evaluated
		// resolve all symbols in the vector (if any)
		vector_ptr_t vec = node->as_vector();
		size_t vec_size = vec->size();
		for(size_t i = 0; i < vec_size; i++) {
			node_idx_t n = (*vec)[i];
			node_idx_t new_n = eval_node(env, n);
			if(n != new_n) {
				vec = vec->assoc(i, new_n);
			}
		}
		return new_node_vector(vec, NODE_FLAG_LITERAL);
	} else if(type == NODE_MAP) {
		if(flags & NODE_FLAG_LITERAL) { return root; }
		// resolve all symbols in the map
		hash_map_ptr_t map = node->as_hash_map();
		for(auto it = map->begin(); it; it++) {
			node_idx_t k = it->first;
			node_idx_t v = it->second;
			node_idx_t new_k = eval_node(env, k);
			node_idx_t new_v = eval_node(env, v);
			if(k != new_k || v != new_v) {
				if(k != new_k) {
					map = map->dissoc(k, node_eq);
				}
				map = map->assoc(new_k, new_v, node_eq);
			}
		}
		return new_node_map(map, NODE_FLAG_LITERAL);
	} else if(type == NODE_HASH_SET) {
		if(flags & NODE_FLAG_LITERAL) { return root; }
		// resolve all symbols in the hash set
		hash_set_ptr_t set = node->as_hash_set();
		for(auto it = set->begin(); it; it++) {
			node_idx_t k = it->first;
			node_idx_t new_k = eval_node(env, k);
			if(k != new_k) {
				set = set->dissoc(k, node_eq);
				set = set->assoc(new_k, node_eq);
			}
		}
		return new_node_hash_set(set, NODE_FLAG_LITERAL);
	} else if(type == NODE_FUTURE && (flags & NODE_FLAG_AUTO_DEREF)) {
		return eval_node(env, get_node(root)->deref());
	}
	return root;
}

static node_idx_t eval_node_list(env_ptr_t env, list_ptr_t list) {
	node_idx_t res = NIL_NODE;
	for(list_t::iterator it(list); it; it++) {
		res = eval_node(env, *it);
	}
	return res;
}

static list_ptr_t list_va(node_idx_t a) { list_ptr_t L = new_list(); L->push_front_inplace(a); return L; }
static list_ptr_t list_va(node_idx_t a, node_idx_t b) { list_ptr_t L = new_list(); L->push_front2_inplace(a, b); return L; }
static list_ptr_t list_va(node_idx_t a, node_idx_t b, node_idx_t c) { list_ptr_t L = new_list(); L->push_front3_inplace(a, b, c); return L; }
static list_ptr_t list_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d) { list_ptr_t L = new_list(); L->push_front4_inplace(a, b, c, d); return L; }
static list_ptr_t list_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e) { list_ptr_t L = new_list(); L->push_front5_inplace(a, b, c, d, e); return L; }
static list_ptr_t list_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e, node_idx_t f) { list_ptr_t L = new_list(); L->push_front6_inplace(a, b, c, d, e, f); return L; }

static node_idx_t eval_va(env_ptr_t env, node_idx_t a) { return eval_list(env, list_va(a)); }
static node_idx_t eval_va(env_ptr_t env, node_idx_t a, node_idx_t b) { return eval_list(env, list_va(a, b)); }
static node_idx_t eval_va(env_ptr_t env, node_idx_t a, node_idx_t b, node_idx_t c) { return eval_list(env, list_va(a, b, c)); }
static node_idx_t eval_va(env_ptr_t env, node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d) { return eval_list(env, list_va(a, b, c, d)); }
static node_idx_t eval_va(env_ptr_t env, node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e) { return eval_list(env, list_va(a, b, c, d, e)); }
static node_idx_t eval_va(env_ptr_t env, node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e, node_idx_t f) { return eval_list(env, list_va(a, b, c, d, e, f)); }

static vector_ptr_t vector_va(node_idx_t a) {
	vector_ptr_t vec = new_vector();
	vec->push_back_inplace(a);
	return vec;
}

static vector_ptr_t vector_va(node_idx_t a, node_idx_t b) {
	vector_ptr_t vec = new_vector();
	vec->push_back_inplace(a);
	vec->push_back_inplace(b);
	return vec;
}

static vector_ptr_t vector_va(node_idx_t a, node_idx_t b, node_idx_t c) {
	vector_ptr_t vec = new_vector();
	vec->push_back_inplace(a);
	vec->push_back_inplace(b);
	vec->push_back_inplace(c);
	return vec;
}

static vector_ptr_t vector_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d) {
	vector_ptr_t vec = new_vector();
	vec->push_back_inplace(a);
	vec->push_back_inplace(b);
	vec->push_back_inplace(c);
	vec->push_back_inplace(d);
	return vec;
}

static vector_ptr_t vector_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e) {
	vector_ptr_t vec = new_vector();
	vec->push_back_inplace(a);
	vec->push_back_inplace(b);
	vec->push_back_inplace(c);
	vec->push_back_inplace(d);
	vec->push_back_inplace(e);
	return vec;
}

static vector_ptr_t vector_va(node_idx_t a, node_idx_t b, node_idx_t c, node_idx_t d, node_idx_t e, node_idx_t f) {
	vector_ptr_t vec = new_vector();
	vec->push_back_inplace(a);
	vec->push_back_inplace(b);
	vec->push_back_inplace(c);
	vec->push_back_inplace(d);
	vec->push_back_inplace(e);
	vec->push_back_inplace(f);
	return vec;
}

// Print the node heirarchy
static void print_node(node_idx_t node, int depth, bool same_line) {
#if 1
	int type = get_node_type(node);
	int flags = get_node_flags(node);
	if(type == NODE_LIST) {
		list_ptr_t list = get_node(node)->t_list;
		printf("(");
		for(list_t::iterator it(list); it; it++) {
			print_node(*it, depth+1, it);
			printf(" ");
		}
		printf(")");
	} else if(type == NODE_LAZY_LIST) {
		printf("%*s(<lazy-list> ", depth, "");
		print_node(get_node(node)->t_extra, depth + 1);
		printf("%*s)", depth, "");
	} else if(type == NODE_VECTOR) {
		vector_ptr_t vector = get_node(node)->as_vector();
		printf("[");
		for(auto it = vector->begin(); it; it++) {
			print_node(*it, depth+1, it);
			printf(" ");
		}
		printf("]");
	} else if(type == NODE_MAP) {
		hash_map_ptr_t map = get_node(node)->as_hash_map();
		if(map->size() == 0) {
			printf("{}");
			return;
		}
		printf("{");
		for(auto it = map->begin(); it; it++) {
			print_node(it->first, depth+1, it != map->end());
			printf(" ");
			print_node(it->second, depth+1, it != map->end());
			printf(" ");
		}
		printf("}");
	} else if(type == NODE_HASH_SET) {
		hash_set_ptr_t set = get_node(node)->as_hash_set();
		if(set->size() == 0) {
			printf("{}");
			return;
		}
		printf("{");
		for(auto it = set->begin(); it; it++) {
			print_node(it->first, depth+1, it != set->end());
			printf(" ");
		}
		printf("}");
	} else if(type == NODE_SYMBOL) {
		printf("%s", get_node(node)->t_string.c_str());
	} else if(type == NODE_KEYWORD) {
		printf(":%s", get_node(node)->t_string.c_str());
	} else if(type == NODE_STRING) {
		printf("\"%s\"", get_node(node)->t_string.c_str());
	} else if(type == NODE_NATIVE_FUNC) {
		printf("<%s>", get_node(node)->t_string.c_str());
	} else if(type == NODE_FUNC) {
		print_node_vector(get_node_func_args(node), depth+1);
		print_node_list(get_node_func_body(node), depth+1);
	} else if(type == NODE_DELAY) {
		printf("<delay>");
	} else if(type == NODE_FLOAT) {
		printf("%f", get_node_float(node));
	} else if(type == NODE_INT) {
		printf("%lld", get_node_int(node));
	} else if(type == NODE_BOOL) {
		printf("%s", get_node_bool(node) ? "true" : "false");
	} else if(type == NODE_NIL) {
		printf("nil");
	} else {
		printf("<<%s>>", get_node_type_string(node));
	}
#else
	node_t *n = get_node(node);
	if(n->type == NODE_LIST) {
		printf("%*s(\n", depth, "");
		print_node_list(n->t_list, depth + 1);
		printf("%*s)\n", depth, "");
	} else if(n->type == NODE_LAZY_LIST) {
		printf("%*s(lazy-list\n", depth, "");
		print_node(n->t_extra, depth + 1);
		printf("%*s)\n", depth, "");
	} else if(n->type == NODE_STRING) {
		printf("%*s\"%s\"\n", depth, "", get_node(node).t_string.c_str());
	} else if(n->type == NODE_SYMBOL) {
		printf("%*s%s\n", depth, "", get_node(node).t_string.c_str());
	} else if(n->type == NODE_KEYWORD) {
		printf("%*s:%s\n", depth, "", get_node(node).t_string.c_str());
	} else if(n->type == NODE_INT) {
		printf("%*s%d\n", depth, "", n->t_int);
	} else if(n->type == NODE_FLOAT) {
		printf("%*s%f\n", depth, "", n->t_float);
	} else if(n->type == NODE_BOOL) {
		printf("%*s%s\n", depth, "", n->t_bool ? "true" : "false");
	} else if(n->type == NODE_NATIVE_FUNC) {
		printf("%*s<%s>\n", depth, "", n->t_string.c_str());
	} else if(n->type == NODE_FUNC) {
		printf("%*s(fn \n", depth, "");
		print_node_list(n->t_func.args, depth + 1);
		print_node_list(n->t_func.body, depth + 1);
		printf("%*s)\n", depth, "");
	} else if(n->type == NODE_DELAY) {
		printf("%*s(def  (delay\n", depth, "");
		print_node_list(n->t_func.body, depth + 1);
		printf("%*s)\n", depth, "");
	} else if(n->type == NODE_VAR) {
		printf("%*s%s = ", depth, "", get_node(node).t_string.c_str());
		print_node(n->t_extra, depth + 1);
	} else if(n->type == NODE_MAP) {
		printf("%*s<map>\n", depth, "");
	} else if(n->type == NODE_HASH_SET) {
		printf("%*s<set>\n", depth, "");
	} else if(n->type == NODE_NIL) {
		printf("%*snil\n", depth, "");
	} else {
		printf("%*s<unknown>\n", depth, "");
	}
#endif
}

static void print_node_list(list_ptr_t nodes, int depth) {
	printf("(");
	for(list_t::iterator i(nodes); i; i++) {
		print_node(*i, depth);
		printf(" ");
	}
	printf(")");
}

static void print_node_vector(vector_ptr_t nodes, int depth) {
	printf("[");
	for(vector_t::iterator i = nodes->begin(); i; i++) {
		print_node(*i, depth);
		printf(" ");
	}
	printf("]");
}

static void print_node_type(node_idx_t i) {
	printf("%s", get_node(i)->type_name());
}


// Generic iterator... 
struct seq_iterator_t {
	int type;
	node_idx_t val;
	bool is_done;
	list_t::iterator it;
	vector_t::iterator vit;
	hash_map_t::iterator mit;
	hash_set_t::iterator hsit;
	lazy_list_iterator_t lit;

	seq_iterator_t(node_idx_t node_idx) : type(), val(NIL_NODE), is_done(), it(), vit(), mit(), lit(node_idx) {
		type = get_node_type(node_idx);
		val = INV_NODE;
		if(type == NODE_LIST) {
			it = list_t::iterator(get_node(node_idx)->t_list);
			if(!done()) {
				val = *it;
			}
		} else if(type == NODE_VECTOR) {
			vit = get_node(node_idx)->as_vector()->begin();
			if(!done()) {
				val = *vit;
			}
		} else if(type == NODE_MAP) {
			mit = get_node(node_idx)->as_hash_map()->begin();
			if(!done()) {
				val = mit->second;
			}
		} else if(type == NODE_HASH_SET) {
			hsit = get_node(node_idx)->as_hash_set()->begin();
			if(!done()) {
				val = hsit->first;
			}
		} else if(type == NODE_LAZY_LIST) {
			val = lit.val;
		} else {
			assert(false);
		}
	}

	// copy
	seq_iterator_t(const seq_iterator_t &other) : type(other.type), val(other.val), is_done(other.is_done), it(other.it), vit(other.vit), mit(other.mit), lit(other.lit) {
	}

	bool done() const {
		if(type == NODE_LIST) {
			return !it;
		} else if(type == NODE_VECTOR) {
			return !vit;
		} else if(type == NODE_MAP) {
			return !mit;
		} else if(type == NODE_HASH_SET) {
			return !hsit;
		} else if(type == NODE_LAZY_LIST) {
			return lit.done();
		}
		return true;
	}

	node_idx_t next() {
		if(done()) {
			return INV_NODE;
		}
		if(type == NODE_LIST) {
			*it++;
			val = it ? *it : INV_NODE;
		} else if(type == NODE_VECTOR) {
			*vit++;
			val = vit ? *vit : INV_NODE;
		} else if(type == NODE_MAP) {
			mit++;
			val = mit ? mit->second : INV_NODE;
		} else if(type == NODE_HASH_SET) {
			hsit++;
			val = hsit ? hsit->first : INV_NODE;
		} else if(type == NODE_LAZY_LIST) {
			lit.next();
			val = lit ? lit.val : INV_NODE;
		}
		return val;
	}

	node_idx_t nth(long long n) {
		if(type == NODE_LAZY_LIST) {
			lit.nth(n);
			return lit.val;
		}
		while(n-- > 0 && !done()) {
			next();
		}
		return val;
	}

	operator bool() const {
		return !done();
	}

	list_ptr_t all() {
		list_ptr_t res = new_list();
		for(;!done();next()) {
			res->push_back_inplace(val);
		}
		if(val != INV_NODE) {
			res->push_back_inplace(val);
		}
		return res;
	}

	void prefetch(long long n) {
		if(type == NODE_LAZY_LIST) {
			lit.prefetch(n);
		}
	}
};

template<typename F>
static inline bool seq_iterate(node_idx_t seq, F f) {
	node_t *n = get_node(seq);
	if(n->type == NODE_LIST) {
		for(list_t::iterator i(n->t_list); i; i++) {
			if(!f(*i)) break;
		}
		return true;
	} else if(n->type == NODE_VECTOR) {
		for(vector_t::iterator i = n->as_vector()->begin(); i; i++) {
			if(!f(*i)) break;
		}
		return true;
	} else if(n->type == NODE_MAP) {
		for(hash_map_t::iterator i = n->as_hash_map()->begin(); i; i++) {
			if(!f(new_node_vector(vector_va(i->first, i->second)))) break;
		}
		return true;
	} else if(n->type == NODE_HASH_SET) {
		for(hash_set_t::iterator i = n->as_hash_set()->begin(); i; i++) {
			if(!f(i->first)) break;
		}
		return true;
	} else if(n->type == NODE_LAZY_LIST) {
		lazy_list_iterator_t lit(seq);
		for(;!lit.done();lit.next()) {
			if(!f(lit.val)) break;
		}
		return true;
	} else if(n->type == NODE_STRING) {
		auto it = n->t_string.c_str();
		for(;*it;it++) {
			if(!f(new_node_int(*it, NODE_FLAG_CHAR))) break;
		}
		return true;
	}
	return false;
}

static bool node_eq(node_idx_t n1i, node_idx_t n2i) {
	if(n1i == INV_NODE || n2i == INV_NODE) return false;
	if(n1i == n2i) return true;

	node_t *n1 = get_node(n1i);
	node_t *n2 = get_node(n2i);

	if(n1->type == NODE_FUTURE && (n1->flags & NODE_FLAG_AUTO_DEREF)) {
		n1i = n1->deref();
		n1 = get_node(n1i);
	}

	if(n2->type == NODE_FUTURE && (n2->flags & NODE_FLAG_AUTO_DEREF)) {
		n2i = n2->deref();
		n2 = get_node(n2i);
	}

	if(n1->type == NODE_NIL || n2->type == NODE_NIL) {
		return n1->type == NODE_NIL && n2->type == NODE_NIL;
	} else if(n1->type == NODE_BOOL && n2->type == NODE_BOOL) {
		return n1->t_bool == n2->t_bool;
	} else if(n1->type == NODE_INT && n2->type == NODE_INT) {
		return n1->t_int == n2->t_int;
	} else if(n1->type == NODE_FLOAT || n2->type == NODE_FLOAT) {
		return n1->as_float() == n2->as_float();
	} else if(n1->type == NODE_ATOM || n2->type == NODE_ATOM) {
		return n1->t_atom == n2->t_atom;
	} else if(n1->flags & n2->flags & NODE_FLAG_STRING) {
		return n1->t_string == n2->t_string;
	} else if(n1->is_func() && n2->is_func()) {
		#if 1 // ?? is this correct?
			return n1i == n2i;
		#else // actual function comparison.... not spec compliant it seems
		{ 
			vector_t::iterator i1 = n1->t_func.args->begin();
			vector_t::iterator i2 = n2->t_func.args->begin();
			for(; i1 && i2; ++i1, ++i2) {
				if(!node_eq(*i1, *i2)) {
					return false;
				}
			}
			if(i1 || i2) {
				return false;
			}
		}
		{
			list_t::iterator i1 = n1->t_func.body->begin();
			list_t::iterator i2 = n2->t_func.body->begin();
			for(; i1 && i2; ++i1, ++i2) {
				if(!node_eq(*i1, *i2)) {
					return false;
				}
			}
			if(i1 || i2) {
				return false;
			}
		}
		return true;
		#endif
	} else if(n1->type == NODE_HASH_SET && n2->type == NODE_HASH_SET) {
		for(auto i1 = n1->as_hash_set()->begin(); i1; i1++) {
			if(!n2->as_hash_set()->contains(i1->first, node_eq)) {
				return false;
			}
		}
		return true;
	} else if(n1->type == NODE_MAP && n2->type == NODE_MAP) {
		for(auto i1 = n1->as_hash_map()->begin(); i1; i1++) {
			auto elem = n2->as_hash_map()->find(i1->first, node_eq);
			if(!elem.third) return false;
			if(!node_eq(i1->second, elem.second)) return false;
		}
		return true;
	} else if(n1->is_seq() && n2->is_seq()) {
		// in this case we want to iterate over the sequences and compare
		// each element
		seq_iterator_t i1(n1i), i2(n2i);
		while(i1.val != INV_NODE && i2.val != INV_NODE) {
			if(!node_eq(i1.val, i2.val)) {
				return false;
			}
			i1.next();
			i2.next();
		}
		if(i1.val != INV_NODE || i2.val != INV_NODE) {
			return false;
		}
		return true;
	}
	return false;
}

static bool node_lt(node_idx_t n1i, node_idx_t n2i) {
	if(n1i == INV_NODE || n2i == INV_NODE) {
		return false;
	}

	node_t *n1 = get_node(n1i);
	node_t *n2 = get_node(n2i);

	if(n1->type == NODE_FUTURE && (n1->flags & NODE_FLAG_AUTO_DEREF)) {
		n1i = n1->deref();
		n1 = get_node(n1i);
	}

	if(n2->type == NODE_FUTURE && (n2->flags & NODE_FLAG_AUTO_DEREF)) {
		n2i = n2->deref();
		n2 = get_node(n2i);
	}

	if(n1->type == NODE_NIL || n2->type == NODE_NIL) {
		return false;
	} else if(n1->is_seq() && n2->is_seq()) {
		// in this case we want to iterate over the sequences and compare
		// each element
		seq_iterator_t i1(n1i), i2(n2i);
		for(; i1 && i2; i1.next(), i2.next()) {
			if(!node_lt(i1.val, i2.val)) {
				return false;
			}
		}
		if(i1.val != INV_NODE || i2.val != INV_NODE) {
			if(!node_lt(i1.val, i2.val)) {
				return false;
			}
		}
		if(i1 || i2) {
			return false;
		}
		return true;
	} else if(n1->type == NODE_BOOL && n2->type == NODE_BOOL) {
		return n1->t_bool < n2->t_bool;
	} else if(n1->type == NODE_STRING && n2->type == NODE_STRING) {
		return n1->t_string < n2->t_string;
	} else if(n1->type == NODE_INT && n2->type == NODE_INT) {
		return n1->t_int < n2->t_int;
	} else if(n1->type == NODE_FLOAT || n2->type == NODE_FLOAT) {
		return n1->as_float() < n2->as_float();
	}
	return false;
}

static bool node_lte(node_idx_t n1i, node_idx_t n2i) {
	if(n1i == INV_NODE || n2i == INV_NODE) {
		return false;
	}

	node_t *n1 = get_node(n1i);
	node_t *n2 = get_node(n2i);

	if(n1->type == NODE_FUTURE && (n1->flags & NODE_FLAG_AUTO_DEREF)) {
		n1i = n1->deref();
		n1 = get_node(n1i);
	}

	if(n2->type == NODE_FUTURE && (n2->flags & NODE_FLAG_AUTO_DEREF)) {
		n2i = n2->deref();
		n2 = get_node(n2i);
	}

	if(n1->type == NODE_NIL || n2->type == NODE_NIL) {
		return false;
	} else if(n1->is_seq() && n2->is_seq()) {
		// in this case we want to iterate over the sequences and compare
		// each element
		seq_iterator_t i1(n1i), i2(n2i);
		for(; i1 && i2; i1.next(), i2.next()) {
			if(!node_lte(i1.val, i2.val)) {
				return false;
			}
		}
		if(i1.val != INV_NODE || i2.val != INV_NODE) {
			if(!node_lte(i1.val, i2.val)) {
				return false;
			}
		}
		if(i1 || i2) {
			return false;
		}
		return true;
	} else if(n1->type == NODE_BOOL && n2->type == NODE_BOOL) {
		return n1->t_bool <= n2->t_bool;
	} else if(n1->type == NODE_STRING && n2->type == NODE_STRING) {
		return n1->t_string <= n2->t_string;
	} else if(n1->type == NODE_INT && n2->type == NODE_INT) {
		return n1->t_int <= n2->t_int;
	} else if(n1->type == NODE_FLOAT || n2->type == NODE_FLOAT) {
		return n1->as_float() <= n2->as_float();
	}
	return false;
}

static void node_let(env_ptr_t env, node_idx_t n1i, node_idx_t n2i) {
	node_t *n1 = get_node(n1i);
	node_t *n2 = get_node(n2i);
	if(n1->type == NODE_SYMBOL) {
		env->set_temp(n1->t_string, n2i);
	/*
	} else if(n1->is_vector() && n2->is_hash_map()) {
		hash_map_ptr_t m = n2->as_hash_map();
		node_idx_t k = n1->as_vector()->nth(0);
		node_idx_t v = n1->as_vector()->nth(1);
		if(get_node_type(k) == NODE_SYMBOL) {
			if(m->size() >= 1) {
				auto it = m->begin();
				env->set_temp(get_node(k)->t_string, it->first);
				node_let(env, v, it->second);
			}
		} else if(get_node_type(k) == NODE_KEYWORD) {
			node_let(env, v, m->get(k, node_eq));
		}
	*/
	} else if(n1->is_seq() && n2->is_seq()) {
		seq_iterator_t i1(n1i), i2(n2i);
		for(; i1 && i2; i1.next(), i2.next()) {
			node_let(env, i1.val, i2.val);
		}
		if(i1.val != INV_NODE && i2.val != INV_NODE) {
			node_let(env, i1.val, i2.val);
		}
	}
}

// jo_hash_value of node_idx_t
size_t jo_hash_value(node_idx_t n) {
	node_t *n1 = get_node(n);
	if(n1->type == NODE_NIL) {
		return 0;
	} else if(n1->is_seq()) {
		size_t hash0 = 2166136261, hash1 = 2166136261, hash2 = 2166136261, hash3 = 2166136261;
		size_t hash4 = 2166136261, hash5 = 2166136261, hash6 = 2166136261, hash7 = 2166136261;
		seq_iterate(n, [&](node_idx_t i) {
			size_t h = jo_hash_value(i);
			hash0 = (16777619 * hash0) ^ ((h>>0) & 255);
			hash1 = (16777619 * hash1) ^ ((h>>8) & 255);
			hash2 = (16777619 * hash2) ^ ((h>>16) & 255);
			hash3 = (16777619 * hash3) ^ ((h>>24) & 255);
			hash4 = (16777619 * hash4) ^ ((h>>32) & 255);
			hash5 = (16777619 * hash5) ^ ((h>>40) & 255);
			hash6 = (16777619 * hash6) ^ ((h>>48) & 255);
			hash7 = (16777619 * hash7) ^ ((h>>56) & 255);
			return true;
		});
		size_t hash = ((hash0 ^ hash1) ^ (hash2 ^ hash3)) ^ ((hash4 ^ hash5) ^ (hash6 ^ hash7));
		return hash & 0x7FFFFFFFFFFFFFFFull;
	} else if(n1->type == NODE_BOOL) {
		return n1->t_bool ? 1 : 0;
	} else if(n1->flags & NODE_FLAG_STRING) {
		return jo_hash_value(n1->t_string.c_str()) & 0x7FFFFFFFFFFFFFFFull;
	} else if(n1->type == NODE_INT) {
		return n1->t_int & 0x7FFFFFFFFFFFFFFFull;
	} else if(n1->type == NODE_FLOAT) {
		return jo_hash_value(n1->as_float()) & 0x7FFFFFFFFFFFFFFFull;
	}
	return 0;
}

// add with destructuring 
static node_idx_t node_add(node_idx_t n1i, node_idx_t n2i) {
	if(n1i == INV_NODE || n2i == INV_NODE) {
		return ZERO_NODE;
	}

	node_t *n1 = get_node(n1i), *n2 = get_node(n2i);
	if(n1->type == NODE_FUTURE && (n1->flags & NODE_FLAG_AUTO_DEREF)) {
		n1i = n1->deref();
		n1 = get_node(n1i);
	}
	if(n2->type == NODE_FUTURE && (n2->flags & NODE_FLAG_AUTO_DEREF)) {
		n2i = n2->deref();
		n2 = get_node(n2i);
	}

	if(n1->is_list() && n2->is_list()) {
		list_ptr_t r = new_list();
		list_t::iterator it1(n1->t_list), it2(n2->t_list);
		for(; it1 && it2; it1++, it2++) {
			r->push_back_inplace(node_add(n1->as_vector()->nth(*it1), n2->as_vector()->nth(*it2)));
		}
		for(; it1; it1++) r->push_back_inplace(n1->as_vector()->nth(*it1));
		for(; it2; it2++) r->push_back_inplace(n2->as_vector()->nth(*it2));
		return new_node_list(r);
	}
	if(n1->is_vector() && n2->is_vector()) {
		vector_ptr_t r = new_vector();
		vector_ptr_t v1 = n1->as_vector(), v2 = n2->as_vector();
		size_t s1 = v1->size(), s2 = v2->size();
		size_t i = 0;
		for(; i < s1 && i < s2; i++) {
			r->push_back_inplace(node_add(v1->nth(i), v2->nth(i)));
		}
		for(; i < s1; i++) r->push_back_inplace(v1->nth(i));
		for(; i < s2; i++) r->push_back_inplace(v2->nth(i));
		return new_node_vector(r);
	}
	if(n1->is_hash_map() && n2->is_hash_map()) {
		hash_map_ptr_t r = new_hash_map();
		hash_map_ptr_t m1 = n1->as_hash_map(), m2 = n2->as_hash_map();
		for(hash_map_t::iterator it = m1->begin(); it; it++) {
			if(!m2->contains(it->first, node_eq)) {
				r->assoc_inplace(it->first, it->second, node_eq);
				continue;
			}
			r->assoc_inplace(it->first, node_add(it->second, m2->get(it->first, node_eq)), node_eq);
		}
		for(hash_map_t::iterator it = m2->begin(); it; it++) {
			if(r->contains(it->first, node_eq)) continue;
			r->assoc_inplace(it->first, it->second, node_eq);
		}
		return new_node_map(r);
	}
	return new_node_float(n1->as_float() + n2->as_float());
}

// mul with destructuring 
static node_idx_t node_mul(node_idx_t n1i, node_idx_t n2i) {
	if(n1i == INV_NODE || n2i == INV_NODE) {
		return ZERO_NODE;
	}

	node_t *n1 = get_node(n1i), *n2 = get_node(n2i);
	if(n1->type == NODE_FUTURE && (n1->flags & NODE_FLAG_AUTO_DEREF)) {
		n1i = n1->deref();
		n1 = get_node(n1i);
	}
	if(n2->type == NODE_FUTURE && (n2->flags & NODE_FLAG_AUTO_DEREF)) {
		n2i = n2->deref();
		n2 = get_node(n2i);
	}

	if(n1->is_list() && n2->is_list()) {
		list_ptr_t r = new_list();
		list_t::iterator it1(n1->t_list), it2(n2->t_list);
		for(; it1 && it2; it1++, it2++) {
			r->push_back_inplace(node_mul(n1->as_vector()->nth(*it1), n2->as_vector()->nth(*it2)));
		}
		for(; it1; it1++) r->push_back_inplace(n1->as_vector()->nth(*it1));
		for(; it2; it2++) r->push_back_inplace(n2->as_vector()->nth(*it2));
		return new_node_list(r);
	}
	if(n1->is_vector() && n2->is_vector()) {
		vector_ptr_t r = new_vector();
		vector_ptr_t v1 = n1->as_vector(), v2 = n2->as_vector();
		size_t s1 = v1->size(), s2 = v2->size();
		size_t i = 0;
		for(; i < s1 && i < s2; i++) {
			r->push_back_inplace(node_mul(v1->nth(i), v2->nth(i)));
		}
		for(; i < s1; i++) r->push_back_inplace(v1->nth(i));
		for(; i < s2; i++) r->push_back_inplace(v2->nth(i));
		return new_node_vector(r);
	}
	if(n1->is_hash_map() && n2->is_hash_map()) {
		hash_map_ptr_t r = new_hash_map();
		hash_map_ptr_t m1 = n1->as_hash_map(), m2 = n2->as_hash_map();
		for(hash_map_t::iterator it = m1->begin(); it; it++) {
			if(!m2->contains(it->first, node_eq)) {
				r->assoc_inplace(it->first, it->second, node_eq);
				continue;
			}
			r->assoc_inplace(it->first, node_mul(it->second, m2->get(it->first, node_eq)), node_eq);
		}
		for(hash_map_t::iterator it = m2->begin(); it; it++) {
			if(r->contains(it->first, node_eq)) continue;
			r->assoc_inplace(it->first, it->second, node_eq);
		}
		return new_node_map(r);
	}
	return new_node_float(n1->as_float() * n2->as_float());
}

// Tests the equality between two or more objects
static node_idx_t native_eq(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return TRUE_NODE;
	}
	list_t::iterator i(args);
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = node_eq(n1, n2);
	for(; i && ret; i++) {
		ret &= node_eq(n1, *i);
	}
	return ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_neq(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return NIL_NODE;
	}
	list_t::iterator i(args);
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = node_eq(n1, n2);
	for(; i && ret; i++) {
		ret = ret && node_eq(n1, *i);
	}
	return !ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_lt(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return TRUE_NODE;
	}
	list_t::iterator i(args);
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = node_lt(n1, n2);
	for(; i && ret; i++) {
		ret = ret && node_lt(n2, *i);
		n2 = *i;
	}
	return ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_lte(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return TRUE_NODE;
	}
	list_t::iterator i(args);
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = node_lte(n1, n2);
	for(; i && ret; i++) {
		ret = ret && node_lte(n2, *i);
		n2 = *i;
	}
	return ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_gt(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return TRUE_NODE;
	}
	list_t::iterator i(args);
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = !node_lte(n1, n2);
	for(; i && ret; i++) {
		ret = ret && !node_lte(n2, *i);
		n2 = *i;
	}
	return ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_gte(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return TRUE_NODE;
	}
	list_t::iterator i(args);
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = !node_lt(n1, n2);
	for(; i && ret; i++) {
		ret = ret && !node_lt(n2, *i);
		n2 = *i;
	}
	return ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_if(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return NIL_NODE;
	}
	list_t::iterator i(args);
	node_idx_t cond = eval_node(env, *i++);
	node_idx_t when_true = *i++;
	node_idx_t when_false = i ? *i++ : NIL_NODE;
	return eval_node(env, get_node(cond)->as_bool() ? when_true : when_false);
}

static node_idx_t native_if_not(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return NIL_NODE;
	}
	list_t::iterator i(args);
	node_idx_t cond = eval_node(env, *i++);
	node_idx_t when_true = *i++;
	node_idx_t when_false = i ? *i++ : NIL_NODE;
	return eval_node(env, !get_node(cond)->as_bool() ? when_true : when_false);
}

// (if-let bindings then)(if-let bindings then else & oldform)
// bindings => binding-form test
// If test is true, evaluates then with binding-form bound to the value of 
// test, if not, yields else
static node_idx_t native_if_let(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return NIL_NODE;
	}
	list_t::iterator i(args);
	node_idx_t bindings = *i++;
	node_t *bindings_node = get_node(bindings);
	if(bindings_node->type != NODE_VECTOR || bindings_node->as_vector()->size() != 2) {
		return NIL_NODE;
	}
	list_ptr_t bindings_list = bindings_node->as_list();
	env_ptr_t env2 = new_env(env);
	node_idx_t key_idx = bindings_list->first_value(); // TODO: should this be eval'd?
	node_idx_t value_idx = eval_node(env2, bindings_list->last_value());
	node_t *key = get_node(key_idx);
	env2->set_temp(key->as_string(), value_idx);
	node_idx_t when_true = *i++;
	node_idx_t when_false = i ? *i++ : NIL_NODE;
	return eval_node(env2, !get_node(value_idx)->as_bool() ? when_true : when_false);
}

static node_idx_t native_if_some(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return NIL_NODE;
	}
	list_t::iterator i(args);
	node_idx_t bindings = *i++;
	node_t *bindings_node = get_node(bindings);
	if(bindings_node->type != NODE_VECTOR || bindings_node->as_vector()->size() != 2) {
		return NIL_NODE;
	}
	list_ptr_t bindings_list = bindings_node->as_list();
	env_ptr_t env2 = new_env(env);
	node_idx_t key_idx = bindings_list->first_value(); // TODO: should this be eval'd?
	node_idx_t value_idx = eval_node(env2, bindings_list->last_value());
	node_t *key = get_node(key_idx);
	env2->set_temp(key->as_string(), value_idx);
	node_idx_t when_true = *i++;
	node_idx_t when_false = i ? *i++ : NIL_NODE;
	return eval_node(env2, value_idx != NIL_NODE ? when_true : when_false);
}


static node_idx_t native_print_str(env_ptr_t env, list_ptr_t args) {
	jo_string s;
	for(list_t::iterator i(args); i;) {
		node_idx_t n = *i++;
		s += get_node(n)->as_string(2);
		if(i) {
			s += " ";
		}
	}
	return new_node_string(s);
}

static node_idx_t native_println_str(env_ptr_t env, list_ptr_t args) {
	jo_string s;
	for(list_t::iterator i(args); i;) {
		node_idx_t n = *i++;
		s += get_node(n)->as_string(2);
		if(i) {
			s += " ";
		}
	}
	s += "\n";
	return new_node_string(s);
}

static node_idx_t native_print(env_ptr_t env, list_ptr_t args) {
	node_idx_t s_idx = native_print_str(env, args);
	printf("%s", get_node(s_idx)->as_string(2).c_str());
	return NIL_NODE;
}

static node_idx_t native_println(env_ptr_t env, list_ptr_t args) {
	node_idx_t s_idx = native_print_str(env, args);
	printf("%s\n", get_node(s_idx)->as_string(2).c_str());
	return NIL_NODE;	
}

static node_idx_t native_do(env_ptr_t env, list_ptr_t args) {
	return eval_node_list(env, args);
}

// (doall coll)
// (doall n coll)
// When lazy sequences are produced via functions that have side
// effects, any effects other than those needed to produce the first
// element in the seq do not occur until the seq is consumed. doall can
// be used to force any effects. Walks through the successive nexts of
// the seq, retains the head and returns it, thus causing the entire
// seq to reside in memory at one time.
static node_idx_t native_doall(env_ptr_t env, list_ptr_t args) {
	list_t::iterator i(args);

	if(args->size() == 1) {
		node_idx_t coll = eval_node(env, args->first_value());
		node_t *n = get_node(coll);
		if(!n->is_seq()) {
			return NIL_NODE;
		}
		seq_iterator_t it(coll);
		return new_node_list(it.all());
	}

	if(args->size() == 2) {
		long long n = get_node(eval_node(env, *i++))->as_int();
		node_idx_t coll = eval_node(env, *i++);
		node_t *n4 = get_node(coll);
		if(!n4->is_seq()) {
			return NIL_NODE;
		}
		list_ptr_t ret = new_list();
		seq_iterator_t it(coll);
		for(--n; it && n; it.next(), n--) {
			ret->push_back_inplace(it.val);
		}
		if(it.val != NIL_NODE && n) {
			ret->push_back_inplace(it.val);
		}
		return new_node_list(ret);
	}

	return NIL_NODE;
}

// (doall-vec coll)
// (doall-vec n coll)
// When lazy sequences are produced via functions that have side
// effects, any effects other than those needed to produce the first
// element in the seq do not occur until the seq is consumed. doall can
// be used to force any effects. Walks through the successive nexts of
// the seq, retains the head and returns it, thus causing the entire
// seq to reside in memory at one time.
static node_idx_t native_doall_vec(env_ptr_t env, list_ptr_t args) {
	list_t::iterator i(args);

	if(args->size() == 1) {
		node_idx_t coll = eval_node(env, args->first_value());
		node_t *n = get_node(coll);
		if(!n->is_seq()) {
			return NIL_NODE;
		}
		vector_ptr_t ret = new_vector();
		seq_iterate(coll, [&ret](node_idx_t ni) {
			ret->push_back_inplace(ni);
			return true;
		});
		return new_node_vector(ret);
	}

	if(args->size() == 2) {
		long long n = get_node(eval_node(env, *i++))->as_int();
		node_idx_t coll = eval_node(env, *i++);
		node_t *n4 = get_node(coll);
		if(!n4->is_seq()) {
			return NIL_NODE;
		}
		vector_ptr_t ret = new_vector();
		seq_iterator_t it(coll);
		for(--n; it && n; it.next(), n--) {
			ret->push_back_inplace(it.val);
		}
		if(it.val != NIL_NODE && n) {
			ret->push_back_inplace(it.val);
		}
		return new_node_vector(ret);
	}

	return NIL_NODE;
}

// (dorun coll)(dorun n coll)
// When lazy sequences are produced via functions that have side
// effects, any effects other than those needed to produce the first
// element in the seq do not occur until the seq is consumed. dorun can
// be used to force any effects. Walks through the successive nexts of
// the seq, does not retain the head and returns nil.
static node_idx_t native_dorun(env_ptr_t env, list_ptr_t args) {
	list_t::iterator i(args);

	if(args->size() == 1) {
		node_idx_t coll = eval_node(env, args->first_value());
		node_t *n = get_node(coll);
		if(!n->is_seq()) {
			return NIL_NODE;
		}
		node_idx_t ret = NIL_NODE;
		seq_iterator_t it(coll);
		for(; it; it.next()) {}
		return NIL_NODE;
	}

	if(args->size() == 2) {
		long long n = get_node(eval_node(env, *i++))->as_int();
		node_idx_t coll = eval_node(env, *i++);
		node_t *n4 = get_node(coll);
		if(!n4->is_seq()) {
			return NIL_NODE;
		}
		seq_iterator_t it(coll);
		for(--n; it && n; it.next(), n--) {}
		return NIL_NODE;
	}

	return NIL_NODE;
}

static node_idx_t native_while(env_ptr_t env, list_ptr_t args) {
	list_t::iterator i(args);
	node_idx_t cond_idx = *i++;
	node_idx_t ret = NIL_NODE;
	while(get_node_bool(eval_node(env, cond_idx))) {
		for(list_t::iterator j = i; j; j++) {
			ret = eval_node(env, *j);
		}
	}
	return ret;
}

static node_idx_t native_while_not(env_ptr_t env, list_ptr_t args) {
	list_t::iterator i(args);
	node_idx_t cond_idx = *i++;
	node_idx_t ret = NIL_NODE;
	while(!get_node_bool(eval_node(env, cond_idx))) {
		for(list_t::iterator j = i; j; j++) {
			ret = eval_node(env, *j);
		}
	}
	return ret;
}

static node_idx_t native_quote(env_ptr_t env, list_ptr_t args) { return new_node_list(args); }
static node_idx_t native_list(env_ptr_t env, list_ptr_t args) { return new_node_list(args); }
static node_idx_t native_unquote(env_ptr_t env, list_ptr_t args) { return args->first_value(); }

static node_idx_t native_quasiquote_1(env_ptr_t env, node_idx_t arg) {
	node_t *n = get_node(arg);
	if(n->type == NODE_VECTOR) {
		vector_ptr_t ret = new_vector();
		for(auto i = n->as_vector()->begin(); i; i++) {
			node_t *n2 = get_node(*i);
			if(n2->type == NODE_LIST && n2->t_list->first_value() == UNQUOTE_SPLICE_NODE) {
				node_idx_t n3_idx = n2->t_list->second_value();
				node_t *n3 = get_node(n3_idx);
				if(n3->is_seq()) {
					seq_iterator_t it(n3_idx);
					for(; it; it.next()) {
						ret->push_back_inplace(it.val);
					}
				} else {
					ret->push_back_inplace(n3_idx);
				}
			} else {
				ret->push_back_inplace(native_quasiquote_1(env, *i));
			}
		}
		return new_node_vector(ret);
	}
	if(n->type == NODE_LIST) {
		list_ptr_t args = n->as_list();
		node_idx_t first_arg = args->first_value();
		if(first_arg == UNQUOTE_NODE || first_arg == UNQUOTE_SPLICE_NODE) {
			return eval_node(env, args->second_value());
		}
		list_ptr_t ret = new_list();
		for(list_t::iterator i(args); i; i++) {
			node_t *n2 = get_node(*i);
			if(n2->type == NODE_LIST && n2->t_list->first_value() == UNQUOTE_SPLICE_NODE) {
				//node_idx_t n3_idx = n2->t_list->second_value();
				node_idx_t n3_idx = eval_node(env, n2->t_list->second_value());
				node_t *n3 = get_node(n3_idx);
				if(n3->is_seq()) {
					seq_iterator_t it(n3_idx);
					for(; it; it.next()) {
						ret->push_back_inplace(it.val);
					}
				} else {
					ret->push_back_inplace(n3_idx);
				}
			} else {
				ret->push_back_inplace(native_quasiquote_1(env, *i));
			}
		}
		return new_node_list(ret);
	}
	if(n->type == NODE_MAP) {
		hash_map_ptr_t ret = new_hash_map();
		for(auto i = n->as_hash_map()->begin(); i; i++) {
			ret = ret->assoc(i->first, native_quasiquote_1(env, i->second), node_eq);
		}
		return new_node_map(ret);
	}
	if(n->type == NODE_HASH_SET) {
		hash_set_ptr_t ret = new_hash_set();
		for(auto i = n->as_hash_set()->begin(); i; i++) {
			ret = ret->assoc(native_quasiquote_1(env, i->first), node_eq);
		}
		return new_node_hash_set(ret);
	}
	return arg;
}

static node_idx_t native_quasiquote(env_ptr_t env, list_ptr_t args) {
	return native_quasiquote_1(env, args->first_value());
}

/*
Usage: (var symbol)
The symbol must resolve to a var, and the Var object
itself (not its value) is returned. The reader macro #'x
expands to (var x).
*/
static node_idx_t native_var(env_ptr_t env, list_ptr_t args) {
	list_t::iterator i(args);
	node_t *n = get_node(*i++);
	if(env->has(n->as_string())) {
		return env->find(n->as_string()).var;
	}
	return NIL_NODE;
}

/*
Usage: (def symbol doc-string? init?)

Creates and interns a global var with the name
of symbol in the current namespace (*ns*) or locates such a var if
it already exists.  If init is supplied, it is evaluated, and the
root binding of the var is set to the resulting value.  If init is
not supplied, the root binding of the var is unaffected.
*/

static node_idx_t native_def(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 1 || args->size() > 3) {
		return NIL_NODE;
	}
	list_t::iterator i(args);
	node_idx_t sym_node_idx = *i++;
	jo_string sym_node = get_node(sym_node_idx)->as_string();
	node_idx_t init = NIL_NODE;
	if(i) {
		node_idx_t doc_string = *i++; // ignored for eval purposes if present
		if(args->size() < 3 || get_node_type(doc_string) != NODE_STRING) {
			init = doc_string;
		} else if(i) {
			init = *i++;
		}
	}

	if(get_node_type(sym_node_idx) != NODE_SYMBOL) {
		fprintf(stderr, "def: expected symbol\n");
		print_node(sym_node_idx);
		return NIL_NODE;
	}

	env->set(sym_node, eval_node(env, init));
	return NIL_NODE;
}

// (defonce name expr)
// defs name to have the root value of the expr if the named var has no root value,
// else expr is unevaluated
static node_idx_t native_defonce(env_ptr_t env, list_ptr_t args) {
	if(env->has(get_node_string(args->first_value()))) {
		return NIL_NODE;
	}
	return native_def(env, args);
}

// (declare & names)
// defs the supplied var names with no bindings, useful for making forward declarations.
static node_idx_t native_declare(env_ptr_t env, list_ptr_t args) {
	for(list_t::iterator i(args); i; i++) {
		node_t *n = get_node(*i);
		if(n->is_symbol()) {
			env->set(n->as_string(), NIL_NODE);
		}
	}
	return NIL_NODE;
}

static node_idx_t native_fn_internal(env_ptr_t env, list_ptr_t args, const jo_string &private_fn_name, int flags) {
	list_t::iterator i(args);
	if(get_node_type(*i) == NODE_VECTOR) {
		node_idx_t reti = new_node(NODE_FUNC, 0);
		node_t *ret = get_node(reti);
		ret->flags |= flags;
		ret->t_func.args = get_node(*i)->as_vector();
		ret->t_func.body = args->rest();
		if(private_fn_name.empty()) {
			ret->t_string = "<anonymous>";
			ret->t_env = env;
		} else {
			ret->t_string = private_fn_name;
			env_ptr_t private_env = new_env(env);
			private_env->set(private_fn_name, reti);
			ret->t_env = private_env;
		}
		return reti;
	}
	return NIL_NODE;
}

static node_idx_t native_fn_macro(env_ptr_t env, list_ptr_t args, bool macro) {
	list_t::iterator i(args);
	int flags = macro ? NODE_FLAG_MACRO : 0;

	jo_string private_fn_name;
	if(get_node_type(*i) == NODE_SYMBOL) {
		private_fn_name = get_node_string(*i++);
		if(get_node_type(*i) == NODE_VECTOR) {
			return native_fn_internal(env, args->rest(), private_fn_name, flags);
		}
	} else if(get_node_type(*i) == NODE_VECTOR) {
		return native_fn_internal(env, args, private_fn_name, flags);
	}

	if(get_node_type(*i) == NODE_LIST) {
		list_ptr_t fn_list = new_list();
		for(; i; i++) {
			node_idx_t arg = *i;
			if(get_node_type(arg) == NODE_LIST) {
				fn_list = fn_list->push_front(native_fn_internal(env, get_node(arg)->t_list, private_fn_name, flags));
			}
		}
		return new_node_native_function("fn_lambda", [=](env_ptr_t env, list_ptr_t args) -> node_idx_t {
			long long num_args = args->size();
			for(list_t::iterator i(fn_list); i; i++) {
				node_idx_t fn_idx = *i;
				node_t *fn = get_node(fn_idx);
				if(fn->type != NODE_FUNC) {
					continue;
				}
				if(fn->t_func.args->size() == num_args) {
					return eval_list(env, args->push_front(*i));
				}
			}
			return NIL_NODE;
		}, macro);
	}
	return NIL_NODE;
}

static node_idx_t native_fn(env_ptr_t env, list_ptr_t args) { return native_fn_macro(env, args, false); }
static node_idx_t native_macro(env_ptr_t env, list_ptr_t args) { return native_fn_macro(env, args, true); }

// (defn name doc-string? attr-map? [params*] prepost-map? body)
// (defn name doc-string? attr-map? ([params*] prepost-map? body) + attr-map?)
// Same as (def name (fn [params* ] exprs*)) or (def
// name (fn ([params* ] exprs*)+)) with any doc-string or attrs added
// to the var metadata. prepost-map defines a map with optional keys
// :pre and :post that contain collections of pre or post conditions.
static node_idx_t native_defn(env_ptr_t env, list_ptr_t args) {
	list_t::iterator i(args);
	node_idx_t sym_node_idx = *i++;
	jo_string sym_node = get_node(sym_node_idx)->as_string();
	node_idx_t doc_string = NIL_NODE;
	if(get_node_type(*i) == NODE_STRING) {
		doc_string = *i++;
	}
	if(get_node_type(*i) == NODE_MAP) { // attribute map
		i++; // skip for now, TODO
	}

	if(get_node_type(sym_node_idx) != NODE_SYMBOL) {
		fprintf(stderr, "defn: expected symbol");
		return NIL_NODE;
	}

	env->set(sym_node, native_fn(env, args->rest(i)));
	return NIL_NODE;
}

// (defmacro name doc-string? attr-map? [params*] body)
// (defmacro name doc-string? attr-map? ([params*] body) + attr-map?)
// Like defn, but the resulting function name is declared as a
// macro and will be used as a macro by the compiler when it is
// called.
static node_idx_t native_defmacro(env_ptr_t env, list_ptr_t args) {
	list_t::iterator i(args);
	node_idx_t sym_node_idx = *i++;
	jo_string sym_node = get_node(sym_node_idx)->as_string();
	node_idx_t doc_string = NIL_NODE;
	if(get_node_type(*i) == NODE_STRING) {
		doc_string = *i++;
	}
	if(get_node_type(*i) == NODE_MAP) { // attribute map
		i++; // skip for now, TODO
	}

	if(get_node_type(sym_node_idx) != NODE_SYMBOL) {
		fprintf(stderr, "defmacro: expected symbol");
		return NIL_NODE;
	}

	node_idx_t m = native_macro(env, args->rest(i));
	env->set(sym_node, new_node_native_function("defmacro__inner", [m](env_ptr_t env2, list_ptr_t args2) -> node_idx_t {
		return eval_node(env2, eval_list(env2, args2->push_front(m)));
	}, true));
	return NIL_NODE;
}

static node_idx_t native_is_nil(env_ptr_t env, list_ptr_t args) { return args->first_value() == NIL_NODE ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_not_nil(env_ptr_t env, list_ptr_t args) { return args->first_value() != NIL_NODE ? TRUE_NODE : FALSE_NODE; }

static node_idx_t native_is_zero(env_ptr_t env, list_ptr_t args) {
	node_idx_t n_idx = args->first_value();
	node_t *n = get_node(n_idx);
	int n_type = get_node_type(n_idx);
	if(n_type == NODE_INT) {
		return n->t_int == 0 ? TRUE_NODE : FALSE_NODE;
	} else if(n_type == NODE_FLOAT) {
		return n->t_float == 0.0 ? TRUE_NODE : FALSE_NODE;
	}
	return FALSE_NODE;
}

// This statement takes a set of test/expression pairs. 
// It evaluates each test one at a time. 
// If a test returns logical true, ‘cond’ evaluates and returns the value of the corresponding expression 
// and doesn't evaluate any of the other tests or expressions. ‘cond’ returns nil.
static node_idx_t native_cond(env_ptr_t env, list_ptr_t args) {
	for(list_t::iterator it(args); it; ) {
		node_idx_t test = eval_node(env, *it++), expr = *it++;
		if (get_node(test)->as_bool() || test == K_ELSE_NODE) {
			return eval_node(env, expr);
		}
	}
	return NIL_NODE;
}

/*
The expression to be evaluated is placed in the ‘case’ statement. 
This generally will evaluate to a value, which is used in the subsequent statements.
Each value is evaluated against that which is passed by the ‘case’ expression. 
Depending on which value holds true, subsequent statement will be executed.
There is also a default statement which gets executed if none of the prior values evaluate to be true.
Example:
(case x 5 (println "x is 5")
      10 (println "x is 10")
      (println "x is neither 5 nor 10"))
*/
static node_idx_t native_case(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t cond_idx = eval_node(env, *it++);
	node_idx_t next = *it++;
	while(it) {
		node_idx_t body_idx = *it++;
		node_idx_t value_idx = eval_node(env, next);
		node_t *value = get_node(value_idx);
		if(node_eq(value_idx, cond_idx)) {
			return eval_node(env, body_idx);
		}
		next = *it++;
	}
	// default: 
	return eval_node(env, next);
}

static node_idx_t native_dotimes(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t binding_idx = *it++;
	if(!get_node(binding_idx)->is_vector()) {
		return NIL_NODE;
	}
	node_t *binding = get_node(binding_idx);
	vector_ptr_t binding_list = binding->as_vector();
	if (binding_list->size() != 2) {
		return NIL_NODE;
	}
	node_idx_t name_idx = binding_list->first_value();
	node_idx_t value_idx = eval_node(env, binding_list->nth(1));
	long long times = get_node(value_idx)->as_int();
	jo_string name = get_node(name_idx)->as_string();
	env_ptr_t env2 = new_env(env);
	node_idx_t ret = NIL_NODE;
	for(long long i = 0; i < times; ++i) {
		env2->set_temp(name, new_node_int(i));
		for(list_t::iterator it2 = it; it2; it2++) { 
			ret = eval_node(env2, *it2);
		}
	}
	return ret;
}

/*
(defn Example []
   (doseq [n (0 1 2)]
   (println n)))
(Example)

The ‘doseq’ statement is similar to the ‘for each’ statement which is found in many other programming languages. 
The doseq statement is basically used to iterate over a sequence.
*/
static node_idx_t native_doseq(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t binding_idx = *it++;
	if(!get_node(binding_idx)->is_vector()) {
		return NIL_NODE;
	}
	node_t *binding = get_node(binding_idx);
	vector_ptr_t binding_list = binding->as_vector();
	if (binding_list->size() != 2) {
		return NIL_NODE;
	}
	node_idx_t name_idx = binding_list->first_value();
	node_idx_t value_idx = eval_node(env, binding_list->nth(1));
	jo_string name = get_node(name_idx)->as_string();
	node_t *value = get_node(value_idx);
	node_idx_t ret = NIL_NODE;
	env_ptr_t env2 = new_env(env);
	for(seq_iterator_t it2(value_idx); it2; it2.next()) {
		env2->set_temp(name, it2.val);
		for(auto it3 = it; it3; it3++) { 
			ret = eval_node(env2, *it3);
		}
	}
	return ret;
}

static node_idx_t native_delay(env_ptr_t env, list_ptr_t args) {
	node_idx_t reti = new_node(NODE_DELAY, 0);
	node_t *ret = get_node(reti);
	//ret->t_func.args  // no args for delays...
	ret->t_func.body = args;
	ret->t_env = env;
	ret->t_extra = INV_NODE;
	return reti;
}

static node_idx_t native_when(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t ret = NIL_NODE;
	if(get_node_bool(eval_node(env, *it++))) {
		for(; it; it++) {
			ret = eval_node(env, *it);
		}
	}
	return ret;
}

// (when-let bindings & body)
// bindings => binding-form test
// When test is true, evaluates body with binding-form bound to the value of test
static node_idx_t native_when_let(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t binding_idx = *it++;
	if(!get_node(binding_idx)->is_vector()) {
		return NIL_NODE;
	}
	node_t *binding = get_node(binding_idx);
	vector_ptr_t binding_list = binding->as_vector();
	if (binding_list->size() & 1) {
		return NIL_NODE;
	}
	env_ptr_t env2 = new_env(env);
	for (vector_t::iterator i = binding_list->begin(); i;) {
		node_idx_t key_idx = *i++; // TODO: should this be eval'd?
		node_idx_t value_idx = eval_node(env2, *i++);
		if (!get_node(value_idx)->as_bool()) {
			return NIL_NODE;
		}
		node_let(env2, key_idx, value_idx);
	}
	node_idx_t ret = NIL_NODE;
	for(; it; it++) {
		ret = eval_node(env2, *it);
	}
	return ret;
}

// (conj coll x)(conj coll x & xs)
// conj[oin]. Returns a new collection with the xs
// 'added'. (conj nil item) returns (item).  The 'addition' may
// happen at different 'places' depending on the concrete type.
static node_idx_t native_conj(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t first_idx = eval_node(env, *it++);
	node_t *first = get_node(first_idx);
	int first_type = first->type;
	list_ptr_t list;
	if(first_type == NODE_NIL) {
		list = new_list();
	} else if(first_type == NODE_LIST) {
		list = first->t_list;
	} else if(first_type == NODE_VECTOR) {
		vector_ptr_t vec = first->as_vector();
		for(; it; it++) {
			vec = vec->push_back(eval_node(env, *it));
		}
		return new_node_vector(vec, NODE_FLAG_LITERAL);
	} else if(first_type == NODE_HASH_SET) {
		hash_set_ptr_t set = first->as_hash_set();
		for(; it; it++) {
			set = set->assoc(eval_node(env, *it), node_eq);
		}
		return new_node_hash_set(set, NODE_FLAG_LITERAL);
	} else {
		list = new_list();
		list->push_front_inplace(first_idx);
	}
	if(list.ptr) {
		for(; it; it++) {
			list = list->push_front(eval_node(env, *it));
		}
		return new_node_list(list, NODE_FLAG_LITERAL);
	}
	return NIL_NODE;
}

static node_idx_t native_pop(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	if(list->is_list()) {
		list_ptr_t list_list = list->as_list();
		if(list_list->size() == 0) return NIL_NODE;
		return new_node_list(list_list->pop());
	}
	if(list->is_vector()) {
		vector_ptr_t list_vec = list->as_vector();
		if(list_vec->size() == 0) return NIL_NODE;
		return new_node_vector(list_vec->pop());
	}
	if(list->is_string()) {
		jo_string list_str = list->as_string();
		if(list_str.size() == 0) return NIL_NODE;
		return new_node_string(list_str.substr(1));
	}
	if(list->is_lazy_list()) {
		auto r = list->seq_rest();
		if(!r.second) return NIL_NODE;
		return r.first;
	}
	return NIL_NODE;
}

static node_idx_t native_peek(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	if(list->is_list()) {
		list_ptr_t list_list = list->t_list;
		if(list_list->size() == 0) return NIL_NODE;
		return list_list->first_value();
	}
	if(list->is_vector()) {
		vector_ptr_t list_vec = list->as_vector();
		if(list_vec->size() == 0) return NIL_NODE;
		return list_vec->first_value();
	}
	if(list->is_string()) {
		jo_string s = list->as_string();
		if(s.size() == 0) return NIL_NODE;
		return new_node_int(s.c_str()[0]);
	}
	if(list->is_lazy_list()) {
		auto f = list->seq_first();
		if(!f.second) return NIL_NODE;
		return f.first;
	}
	return NIL_NODE;
}

// (constantly x)
// Returns a function that takes any number of arguments and returns x.
static node_idx_t native_constantly(env_ptr_t env, list_ptr_t args) {
	node_idx_t x_idx = args->first_value();
	return new_node_native_function("native_constantly_fn", [=](env_ptr_t env, list_ptr_t args) { return x_idx; }, false);
}

static node_idx_t native_count(env_ptr_t env, list_ptr_t args) {
	return new_node_int(get_node(args->first_value())->seq_size());
}

static node_idx_t native_is_delay(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_DELAY ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_empty(env_ptr_t env, list_ptr_t args) { return get_node(args->first_value())->seq_empty() ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_not_empty(env_ptr_t env, list_ptr_t args) { return get_node(args->first_value())->seq_empty() ? FALSE_NODE : TRUE_NODE; }
static node_idx_t native_is_false(env_ptr_t env, list_ptr_t args) { return get_node_bool(args->first_value()) ? FALSE_NODE : TRUE_NODE; }
static node_idx_t native_is_true(env_ptr_t env, list_ptr_t args) { return get_node_bool(args->first_value()) ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_some(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) != NODE_NIL ? TRUE_NODE : FALSE_NODE; }

// (first coll)
// Returns the first item in the collection. Calls seq on its argument. If coll is nil, returns nil.
static node_idx_t native_first(env_ptr_t env, list_ptr_t args) {
	node_t *node = get_node(args->first_value());
	auto f = node->seq_first();
	if(!f.second) return NIL_NODE;
	return f.first;
}

static node_idx_t native_second(env_ptr_t env, list_ptr_t args) {
	node_t *node = get_node(args->first_value());
	auto r = node->seq_rest();
	if(!r.second) return NIL_NODE;
	node = get_node(r.first);
	auto f = node->seq_first();
	if(!f.second) return NIL_NODE;
	return f.first;
}

static node_idx_t native_last(env_ptr_t env, list_ptr_t args) {
	node_idx_t node_idx = args->first_value();
	node_t *node = get_node(node_idx);
	if(node->is_string()) {
		jo_string &str = node->t_string;
		if(str.size() < 1) return NIL_NODE;
		return new_node_int(node->t_string.c_str()[str.size() - 1]);
	}
	if(node->is_list()) return node->t_list->last_value();
	if(node->is_vector()) return node->as_vector()->last_value();
	if(node->is_hash_map()) {
		hash_map_ptr_t map = node->as_hash_map();
		if(map->size() == 0) return NIL_NODE;
		return map->last_value();
	}
	if(node->is_hash_set()) {
		hash_set_ptr_t set = node->as_hash_set();
		if(set->size() == 0) return NIL_NODE;
		return set->last_value();
	}
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(node_idx);
		for(; !lit.done(); lit.next()) {}
		return lit.val;
	}
	return NIL_NODE;
}

static node_idx_t native_nth(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	node_idx_t n_idx = *it++;
	long long n = get_node(n_idx)->as_int();
	if(list->is_string()) {
		jo_string &str = list->t_string;
		if(n < 0) n = str.size() + n;
		if(n < 0 || n >= str.size()) return NIL_NODE;
		return new_node_int(str.c_str()[n]);
	}
	if(list->is_list()) {
		if(n < 0 || n >= list->as_list()->size()) return NIL_NODE;
		return list->as_list()->nth(n);
	}
	if(list->is_vector()) {
		if(n < 0 || n >= list->as_vector()->size()) return NIL_NODE;
		return list->as_vector()->nth(n);
	}
	if(list->is_lazy_list()) return lazy_list_iterator_t(list_idx).nth(n);
	return NIL_NODE;
}

// (rand-nth coll)
// Return a random element of the (sequential) collection. Will have
// the same performance characteristics as nth for the given
// collection.
static node_idx_t native_rand_nth(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	if(list->is_string()) {
		jo_string &str = list->t_string;
		if(str.size() == 0) return NIL_NODE;
		return new_node_int(str.c_str()[jo_pcg32(&jo_rnd_state) % str.size()]);
	}
	if(list->is_list()) {
		size_t size = list->as_list()->size();
		if(size > 0) return list->as_list()->nth(jo_pcg32(&jo_rnd_state) % size);
	}
	if(list->is_vector()) {
		size_t size = list->as_vector()->size();
		if(size > 0) return list->as_vector()->nth(jo_pcg32(&jo_rnd_state) % size);
	}
	return NIL_NODE;
}

// equivalent to (first (first x))
static node_idx_t native_ffirst(env_ptr_t env, list_ptr_t args) { return native_first(env, list_va(native_first(env, args))); }
static node_idx_t native_is_fn(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_FUNC ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_indexed(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_VECTOR ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_letter(env_ptr_t env, list_ptr_t args) { return jo_isletter(get_node_int(args->first_value())) ? TRUE_NODE : FALSE_NODE; }

// Returns true if x implements IFn. Note that many data structures
// (e.g. sets and maps) implement IFn
static node_idx_t native_is_ifn(env_ptr_t env, list_ptr_t args) {
	int type =  get_node_type(args->first_value());
	return type == NODE_FUNC || type == NODE_NATIVE_FUNC || type == NODE_VECTOR || type == NODE_MAP || type == NODE_HASH_SET || type == NODE_SYMBOL || type == NODE_KEYWORD ? TRUE_NODE : FALSE_NODE;
}

// Return true if x is a symbol or keyword
static node_idx_t native_is_ident(env_ptr_t env, list_ptr_t args) { int type =  get_node_type(args->first_value());	return type == NODE_SYMBOL || type == NODE_KEYWORD ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_symbol(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_SYMBOL ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_keyword(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_KEYWORD ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_list(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_LIST ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_hash_map(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_MAP ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_set(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_HASH_SET ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_vector(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_VECTOR ? TRUE_NODE : FALSE_NODE; }

// Return true if x is a symbol or keyword without a namespace
static node_idx_t native_is_simple_ident(env_ptr_t env, list_ptr_t args) {
	int type =  get_node_type(args->first_value());
	return (type == NODE_SYMBOL || type == NODE_KEYWORD) && get_node(args->first_value())->t_string.find('/') == jo_npos ? TRUE_NODE : FALSE_NODE;
}

// Return true if x is a symbol or keyword with a namespace
static node_idx_t native_is_qualified_ident(env_ptr_t env, list_ptr_t args) {
	int type =  get_node_type(args->first_value());
	return (type == NODE_SYMBOL || type == NODE_KEYWORD) && get_node(args->first_value())->t_string.find('/') != jo_npos ? TRUE_NODE : FALSE_NODE;
}

// Return true if x is a symbol without a namespace
static node_idx_t native_is_simple_symbol(env_ptr_t env, list_ptr_t args) {
	int type =  get_node_type(args->first_value());
	return type == NODE_SYMBOL && get_node(args->first_value())->t_string.find('/') == jo_npos ? TRUE_NODE : FALSE_NODE;
}

// Return true if x is a symbol with a namespace
static node_idx_t native_is_qualified_symbol(env_ptr_t env, list_ptr_t args) {
	int type =  get_node_type(args->first_value());
	return type == NODE_SYMBOL && get_node(args->first_value())->t_string.find('/') != jo_npos ? TRUE_NODE : FALSE_NODE;
}

// Return true if x is a keyword without a namespace
static node_idx_t native_is_simple_keyword(env_ptr_t env, list_ptr_t args) {
	int type =  get_node_type(args->first_value());
	return type == NODE_KEYWORD && get_node(args->first_value())->t_string.find('/') == jo_npos ? TRUE_NODE : FALSE_NODE;
}

// Return true if x is a keyword with a namespace
static node_idx_t native_is_qualified_keyword(env_ptr_t env, list_ptr_t args) {
	int type =  get_node_type(args->first_value());
	return type == NODE_KEYWORD && get_node(args->first_value())->t_string.find('/') != jo_npos ? TRUE_NODE : FALSE_NODE;
}

// (next coll) 
// Returns a seq of the items after the first. Calls seq on its
// argument.  If there are no more items, returns nil.
static node_idx_t native_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	auto r = node->seq_rest();
	if(!r.second) return NIL_NODE;
	return r.first;
}

static node_idx_t native_nfirst(env_ptr_t env, list_ptr_t args) { return native_next(env, list_va(native_first(env, args))); }
static node_idx_t native_nnext(env_ptr_t env, list_ptr_t args) { return native_next(env, list_va(native_next(env, args))); }
static node_idx_t native_fnext(env_ptr_t env, list_ptr_t args) { return native_first(env, list_va(native_next(env, args))); }

// like next, but always returns a list
static node_idx_t native_rest(env_ptr_t env, list_ptr_t args) { return get_node(args->first_value())->seq_rest().first; }

static node_idx_t native_when_not(env_ptr_t env, list_ptr_t args) { return !get_node_bool(args->first_value()) ? eval_node_list(env, args->rest()) : NIL_NODE; }

// (let [a 1 b 2] (+ a b))
static node_idx_t native_let(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(!node->is_vector()) {
		warnf("let: expected vector\n");
		return NIL_NODE;
	}
	vector_ptr_t list_list = node->as_vector();
	if(list_list->size() % 2 != 0) {
		warnf("let: expected even number of elements\n");
		return NIL_NODE;
	}
	env_ptr_t env2 = new_env(env);
	for(vector_t::iterator i = list_list->begin(); i;) {
		node_idx_t key_idx = *i++; // TODO: should this be eval'd?
		node_idx_t value_idx = eval_node(env2, *i++);
		node_let(env2, key_idx, value_idx);
	}
	return eval_node_list(env2, args->rest());
}

static node_idx_t native_apply(env_ptr_t env, list_ptr_t args) {
	// collect the arguments, if its a list add the whole list, then eval it
	list_ptr_t arg_list = new_list();
	for(list_t::iterator it(args); it; it++) {
		node_idx_t arg_idx = eval_node(env, *it);
		node_t *arg = get_node(arg_idx);
		if(arg->is_list()) {
			arg_list->conj_inplace(*arg->as_list().ptr);
		} else if(arg->is_seq()) {
			if(!seq_iterate(arg_idx, [&](node_idx_t node_idx) { arg_list->push_back_inplace(node_idx); return true; })) {
				arg_list->push_back_inplace(arg_idx);
			}
		} else {
			arg_list->push_back_inplace(arg_idx);
		}
	}
	return eval_list(env, arg_list);
}

// (reduced x)
// Wraps x in a way such that a reduce will terminate with the value x
static node_idx_t native_reduced(env_ptr_t env, list_ptr_t args) {
	node_idx_t ret_idx = new_node(NODE_REDUCED, 0);
	get_node(ret_idx)->t_extra = args->first_value();
	return ret_idx;
}

static node_idx_t native_unreduced(env_ptr_t env, list_ptr_t args) {
	node_idx_t fvi = args->first_value();
	node_t *fv = get_node(fvi);
	if(fv->type == NODE_REDUCED) {
		return fv->t_extra;
	}
	return fvi;
}

static node_idx_t native_ensure_reduced(env_ptr_t env, list_ptr_t args) {
	node_idx_t fvi = args->first_value();
	node_t *fv = get_node(fvi);
	if(fv->type == NODE_REDUCED) {
		return fvi;
	}
	node_idx_t ret_idx = new_node(NODE_REDUCED, 0);
	get_node(ret_idx)->t_extra = fvi;
	return ret_idx;
}

static node_idx_t native_is_reduced(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_REDUCED ? TRUE_NODE : FALSE_NODE; }

// (reduce f coll)
// (reduce f val coll)
// f should be a function of 2 arguments. 
// If val is not supplied, returns the result of applying f to the first 2 items in coll, 
//  then applying f to that result and the 3rd item, etc. 
// If coll contains no items, f must accept no arguments as well, and reduce returns the result of calling f with no arguments.  
// If coll has only 1 item, it is returned and f is not called.  
// If val is supplied, returns the result of applying f to val and the first item in coll, 
//  then applying f to that result and the 2nd item, etc. 
// If coll contains no items, returns val and f is not called.
static node_idx_t native_reduce(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f_idx = *it++;
	// (reduce f coll)
	// returns the result of applying f to the first 2 items in coll, then applying f to that result and the 3rd item, etc. 
	// If coll contains no items, f must accept no arguments as well, and reduce returns the result of calling f with no arguments.  
	// If coll has only 1 item, it is returned and f is not called.  
	if(args->size() == 2) {
		node_idx_t coll_idx = eval_node(env, *it++);
		node_idx_t reti = INV_NODE;
		seq_iterate(coll_idx, [env,&reti,f_idx](node_idx_t node_idx) {
			if(reti == INV_NODE) {
				reti = node_idx;
			} else {
				reti = eval_va(env, f_idx, reti, node_idx);
			}
			if(get_node_type(reti) == NODE_REDUCED) {
				reti = get_node(reti)->t_extra;
				return false;
			}
			return true;
		});
		return reti;
	}
	// (reduce f val coll)
	// returns the result of applying f to val and the first item in coll,
	//  then applying f to that result and the 2nd item, etc.
	// If coll contains no items, returns val and f is not called.
	if(args->size() == 3) {
		node_idx_t reti = eval_node(env, *it++);
		node_idx_t coll = eval_node(env, *it++);
		seq_iterate(coll, [env,&reti,f_idx](node_idx_t node_idx) {
			reti = eval_va(env, f_idx, reti, node_idx);
			if(get_node_type(reti) == NODE_REDUCED) {
				reti = get_node(reti)->t_extra;
				return false;
			}
			return true;
		});
		return reti;
	}
	return NIL_NODE;
}

// eval each arg in turn, return if any eval to false
static node_idx_t native_and(env_ptr_t env, list_ptr_t args) {
	for(list_t::iterator it(args); it; it++) {
		if(!get_node_bool(eval_node(env, *it))) {
			return FALSE_NODE;
		}
	}
	return TRUE_NODE;
}

static node_idx_t native_or(env_ptr_t env, list_ptr_t args) {
	for(list_t::iterator it(args); it; it++) {
		if(get_node_bool(eval_node(env, *it))) {
			return TRUE_NODE;
		}
	}
	return FALSE_NODE;
}

static node_idx_t native_not(env_ptr_t env, list_ptr_t args) { return !get_node_bool(args->first_value()) ? TRUE_NODE : FALSE_NODE; }

static node_idx_t native_reverse(env_ptr_t env, list_ptr_t args) {
	node_idx_t node_idx = args->first_value();
    node_t *node = get_node(node_idx);
	if(node->is_string()) return new_node_string(node->as_string().reverse());
	if(node->is_list()) return new_node_list(node->as_list()->reverse());
	if(node->is_vector()) return new_node_vector(node->as_vector()->reverse());
	if(node->is_hash_map()) return node_idx; // nonsensical to reverse an "unordered" map. just give back the input.
	if(node->is_hash_set()) return node_idx; // nonsensical to reverse an "unordered" set. just give back the input.
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(node_idx);
		list_ptr_t list_list = new_list();
		for(; !lit.done(); lit.next()) {
			list_list->push_front_inplace(lit.val);
		}
		return new_node_list(list_list);
	}
	return NIL_NODE;
}

static node_idx_t native_is_reversible(env_ptr_t env, list_ptr_t args) {
	node_idx_t node_idx = args->first_value();
	node_t *node = get_node(node_idx);
	if(node->is_string()) return TRUE_NODE;
	if(node->is_list()) return TRUE_NODE;
	if(node->is_vector()) return TRUE_NODE;
	if(node->is_lazy_list()) return TRUE_NODE;
	return FALSE_NODE;
}

static node_idx_t native_eval(env_ptr_t env, list_ptr_t args) { return eval_node(env, args->first_value()); }

// (into) 
// (into to)
// (into to from)
// Returns a new coll consisting of to-coll with all of the items of
//  from-coll conjoined.
// A non-lazy concat
static node_idx_t native_into(env_ptr_t env, list_ptr_t args) {
	node_idx_t to = args->first_value();
	node_idx_t from = args->second_value();
	if(get_node_type(to) == NODE_LIST) {
		list_ptr_t ret = new_list(*get_node(to)->t_list);
		seq_iterate(from, [&ret](node_idx_t item) { ret->push_back_inplace(item); return true; });
		return new_node_list(ret);
	}
	if(get_node_type(to) == NODE_VECTOR) {
		vector_ptr_t ret = new_vector(*get_node(to)->as_vector());
		seq_iterate(from, [&ret](node_idx_t item) { ret->push_back_inplace(item); return true; });
		return new_node_vector(ret);
	}
	if(get_node_type(to) == NODE_MAP) {
		hash_map_ptr_t ret = new_hash_map(*get_node(to)->as_hash_map());
		seq_iterate(from, [&ret](node_idx_t item) { 
			if(get_node_type(item) == NODE_MAP) {
				ret = ret->conj(get_node(item)->as_hash_map().ptr, node_eq);
			}
			return true;
		});
		return new_node_map(ret);
	}
	if(get_node_type(to) == NODE_HASH_SET) {
		hash_set_ptr_t ret = new_hash_set(*get_node(to)->as_hash_set());
		seq_iterate(from, [&ret](node_idx_t item) { ret = ret->assoc(item, node_eq); return true; });
		return new_node_hash_set(ret);
	}
	return NIL_NODE;
}

// Compute the time to execute arguments
static node_idx_t native_time(env_ptr_t env, list_ptr_t args) {
	double time_start = jo_time();
	eval_node_list(env, args);
	double time_end = jo_time();
	return new_node_float(time_end - time_start);
}


// (hash-map)(hash-map & keyvals)
// keyvals => key val
// Returns a new hash map with supplied mappings.  If any keys are
// equal, they are handled as if by repeated uses of assoc.
static node_idx_t native_hash_map(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	hash_map_ptr_t map = new_hash_map();
	while(it) {
		node_idx_t k = eval_node(env, *it++);
		if(!it) {
			break;
		}
		node_idx_t v = eval_node(env, *it++);
		map = map->assoc(k, v, node_eq);
	}
	return new_node_map(map);
}

// (assoc map key val)(assoc map key val & kvs)
// assoc[iate]. When applied to a map, returns a new map of the
// same (hashed/sorted) type, that contains the mapping of key(s) to
// val(s). When applied to a vector, returns a new vector that
// contains val at index. Note - index must be <= (count vector).
static node_idx_t native_assoc(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t map_idx = *it++;
	if(map_idx == NIL_NODE) {
		hash_map_ptr_t map = new_hash_map();
		while(it) {
			node_idx_t key_idx = *it++;
			node_idx_t val_idx = *it++;
			map = map->assoc(key_idx, val_idx, node_eq);
		}
		return new_node_map(map);
	}
	node_t *map_node = get_node(map_idx);
	if(map_node->is_hash_map()) {
		hash_map_ptr_t map = map_node->as_hash_map();
		while(it) {
			node_idx_t key_idx = *it++;
			node_idx_t val_idx = *it++;
			map = map->assoc(key_idx, val_idx, node_eq);
		}
		return new_node_map(map);
	} 
	if(map_node->is_hash_set()) {
		hash_set_ptr_t set = map_node->as_hash_set();
		while(it) {
			set = set->assoc(*it++, node_eq);
		}
		return new_node_hash_set(set);
	} 
	if(map_node->is_vector()) {
		vector_ptr_t vec = map_node->as_vector();
		while(it) {
			node_idx_t key_idx = *it++;
			node_idx_t val_idx = *it++;
			vec = vec->assoc(get_node_int(key_idx), val_idx);
		}
		return new_node_vector(vec);
	}
	if(map_node->is_vector2d()) {
		vector2d_ptr_t vec = map_node->as_vector2d();
		while(it) {
			node_idx_t key_idx = *it++;
			node_idx_t val_idx = *it++;
			node_t *key_node = get_node(key_idx);
			size_t x = get_node_int(key_node->seq_first().first);
			size_t y = get_node_int(key_node->seq_second().first);
			vec = vec->set_new(x, y, val_idx);
		}
		return new_node_vector2d(vec);
	}
	warnf("assoc: not a map, set, or vector\n");
	return NIL_NODE;
}

// (dissoc map)(dissoc map key)(dissoc map key & ks)
// dissoc[iate]. Returns a new map of the same (hashed/sorted) type,
// that does not contain a mapping for key(s).
static node_idx_t native_dissoc(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t map_idx = *it++;
	node_t *map_node = get_node(map_idx);
	if(map_node->is_hash_map()) {
		hash_map_ptr_t map = map_node->as_hash_map();
		for(; it; it++) {
			map = map->dissoc(*it, node_eq);
		}
		return new_node_map(map);
	} 
	return NIL_NODE;
}

// (disj set)(disj set key)(disj set key & ks)
// disj[oin]. Returns a new set of the same (hashed/sorted) type, that
// does not contain key(s).
static node_idx_t native_disj(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t set_idx = *it++;
	node_t *set_node = get_node(set_idx);
	if(set_node->is_hash_set()) {
		hash_set_ptr_t set = set_node->as_hash_set();
		for(; it; it++) {
			set = set->dissoc(*it, node_eq);
		}
		return new_node_hash_set(set);
	} 
	return NIL_NODE;
}

// (get map key)(get map key not-found)
// Returns the value mapped to key, not-found or nil if key not present.
static node_idx_t native_get(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t map_idx = *it++;
	node_idx_t key_idx = *it++;
	node_idx_t not_found_idx = it ? *it++ : NIL_NODE;
	node_t *map_node = get_node(map_idx);
	node_t *key_node = get_node(key_idx);
	node_t *not_found_node = get_node(not_found_idx);
	if(map_node->is_hash_map()) {
		auto entry = map_node->as_hash_map()->find(key_idx, node_eq);
		if(entry.third) {
			return entry.second;
		}
		return not_found_idx;
	}
	if(map_node->is_hash_set()) {
		auto entry = map_node->as_hash_set()->find(key_idx, node_eq);
		if(entry.second) {
			return entry.first;
		}
		return not_found_idx;
	}
	if(map_node->is_vector()) {
		long long vec_idx = key_node->as_int();
		if(vec_idx < 0 || vec_idx > map_node->as_vector()->size()) {
			return not_found_idx;
		}
		return map_node->as_vector()->nth(key_node->as_int());
	}
	return NIL_NODE;
}

// (assoc-in m [k & ks] v)
// Associates a value in a nested associative structure, where ks is a
// sequence of keys and v is the new value and returns a new nested structure.
// If any levels do not exist, hash-maps will be created.
static node_idx_t native_assoc_in(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t map_idx = *it++;
	node_idx_t key_idx = *it++;
	node_idx_t val_idx = *it++;
	node_t *key_node = get_node(key_idx);
	if(!key_node->is_vector()) {
		warnf("assoc-in: key must be a vector");
		return NIL_NODE;
	}
	vector_ptr_t key_vector = key_node->as_vector();
	if(key_vector->size() > 1) {
		node_idx_t map2 = native_get(env, list_va(map_idx, key_vector->first_value()));
		node_idx_t val2 = native_assoc_in(env, list_va(map2, new_node_vector(key_vector->rest()), val_idx));
		return native_assoc(env, list_va(map_idx, key_vector->first_value(), val2));
	}
	return native_assoc(env, list_va(map_idx, key_vector->first_value(), val_idx));
}

// (update m k f)(update m k f x)(update m k f x y)(update m k f x y z)(update m k f x y z & more)
// 'Updates' a value in an associative structure, where k is a
// key and f is a function that will take the old value
// and any supplied args and return the new value, and returns a new
// structure.  If the key does not exist, nil is passed as the old value.
static node_idx_t native_update(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 3) {
		warnf("update: at least 3 arguments required");
		return NIL_NODE;
	}
	list_t::iterator it(args);
	node_idx_t map_idx = *it++;
	node_idx_t key_idx = *it++;
	node_idx_t f_idx = *it++;
	list_ptr_t rest = args->rest(it);
	node_idx_t val_idx = eval_va(env, f_idx, native_get(env, list_va(map_idx, key_idx)));
	return native_assoc(env, rest->push_front3(map_idx, key_idx, val_idx));
}

// (update-in m ks f & args)
// 'Updates' a value in a nested associative structure, where ks is a
// sequence of keys and f is a function that will take the old value
// and any supplied args and return the new value, and returns a new
// nested structure.  If any levels do not exist, hash-maps will be
// created.
static node_idx_t native_update_in(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 3) {
		warnf("update-in: at least 3 arguments required");
		return NIL_NODE;
	}
	list_t::iterator it(args);
	node_idx_t map_idx = *it++;
	node_idx_t key_idx = *it++;
	node_idx_t f_idx = *it++;
	node_t *key_node = get_node(key_idx);
	if(!key_node->is_vector()) {
		warnf("assoc-in: key must be a vector");
		return NIL_NODE;
	}
	vector_ptr_t key_vector = key_node->as_vector();
	if(key_vector->size() > 1) {
		node_idx_t map2 = native_get(env, list_va(map_idx, key_vector->first_value()));
		node_idx_t val2 = native_update_in(env, list_va(map2, new_node_vector(key_vector->rest()), f_idx));
		return native_assoc(env, list_va(map_idx, key_vector->first_value(), val2));
	} else {
		list_ptr_t rest = args->rest(it);
		node_idx_t map2 = native_get(env, list_va(map_idx, key_vector->first_value()));
		return native_assoc(env, list_va(map_idx, key_vector->first_value(), eval_list(env, rest->push_front2(f_idx, map2))));
	}
}

// (comp)(comp f)(comp f g)(comp f g & fs)
// Takes a set of functions and returns a fn that is the composition
// of those fns.  The returned fn takes a variable number of args,
// applies the rightmost of fns to the args, the next
// fn (right-to-left) to the result, etc.
static node_idx_t native_comp(env_ptr_t env, list_ptr_t args) {
	list_ptr_t rargs = args->reverse();
	list_t::iterator it(args); // TODO: maybe reverse iterator (would be faster)
	node_idx_t ret = new_node_native_function("comp-lambda", [=](env_ptr_t env, list_ptr_t args) {
		list_t::iterator it(rargs);
		node_idx_t ret = NIL_NODE;
		if(it) {
			ret = eval_list(env, new_list()->push_back_inplace(*it++)->conj_inplace(*args));
			while(it) {
				ret = eval_list(env, new_list()->push_back_inplace(*it++)->push_back_inplace(ret));
			}
		}
		return ret;
	}, false);
	return ret;
}

// (partial f)(partial f arg1)(partial f arg1 arg2)(partial f arg1 arg2 arg3)
// (partial f arg1 arg2 arg3 & more)
// Takes a function f and fewer than the normal arguments to f, and
// returns a fn that takes a variable number of additional args. When
// called, the returned function calls f with args + additional args.
static node_idx_t native_partial(env_ptr_t env, list_ptr_t args) {
	return new_node_native_function("partial-lambda", [=](env_ptr_t env, list_ptr_t args2) { return eval_list(env, args->conj(*args2)); }, false);
}

// (shuffle coll)
// Return a random permutation of coll
static node_idx_t native_shuffle(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t coll_idx = *it++;
	int type = get_node_type(coll_idx);
	if(type == NODE_LIST)     return new_node_list(get_node(coll_idx)->t_list->shuffle());
	if(type == NODE_VECTOR)   return new_node_vector(get_node(coll_idx)->as_vector()->shuffle());
	if(type == NODE_MAP)      return coll_idx;
	if(type == NODE_HASH_SET) return coll_idx;
	return NIL_NODE;
}

// (random-sample prob)(random-sample prob coll)
// Returns items from coll with random probability of prob (0.0 -
// 1.0).  Returns a transducer when no collection is provided.
static node_idx_t native_random_sample(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t prob_idx = *it++;
	node_idx_t coll_idx = it ? *it++ : NIL_NODE;
	node_t *prob_node = get_node(prob_idx);
	node_t *coll_node = get_node(coll_idx);
	double prob = prob_node->as_float();
	prob = prob < 0 ? 0 : prob > 1 ? 1 : prob;
	if(coll_node->is_list()) return new_node_list(coll_node->t_list->random_sample(prob));
	if(coll_node->is_vector()) return new_node_vector(coll_node->as_vector()->random_sample(prob));
	if(coll_node->is_lazy_list()) {
		list_ptr_t ret = new_list();
		for(lazy_list_iterator_t lit(coll_idx); lit; lit.next()) {
			if(jo_random_float() < prob) {
				ret->push_back_inplace(lit.val);
			}
		}
		return new_node_list(ret);
	}
	return NIL_NODE;
}

// (is form)(is form msg)
// Generic assertion macro.  'form' is any predicate test.
// 'msg' is an optional message to attach to the assertion.
// 
// Example: (is (= 4 (+ 2 2)) "Two plus two should be 4")
//  Special forms:
//  (is (thrown? c body)) checks that an instance of c is thrown from
// body, fails if not; then returns the thing thrown.
//  (is (thrown-with-msg? c re body)) checks that an instance of c is
// thrown AND that the message on the exception matches (with
// re-find) the regular expression re.
static node_idx_t native_is(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t form_idx = *it++;
	node_idx_t msg_idx = it ? *it++ : NIL_NODE;
	node_t *form_node = get_node(eval_node(env, form_idx));
	node_t *msg_node = get_node(eval_node(env, msg_idx));
	if(!form_node->as_bool()) {
		if(msg_node->is_string()) {
			printf("%s\n", msg_node->t_string.c_str());
		} else {
			printf("Assertion failed\n");
			native_print(env, list_va(form_idx));
			printf("\n");
		}
	}
	return NIL_NODE;
}

// (identity x)
// Returns its argument.
static node_idx_t native_identity(env_ptr_t env, list_ptr_t args) { return args->first_value(); }

// (nthrest coll n)
// Returns the nth rest of coll, coll when n is 0.
static node_idx_t native_nthrest(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t coll_idx = *it++;
	node_idx_t n_idx = *it++;
	node_t *coll_node = get_node(coll_idx);
	node_t *n_node = get_node(n_idx);
	long long n = n_node->as_int();
	if(n <= 0) return coll_idx;
	if(coll_node->is_string()) {
		jo_string &str = coll_node->t_string;
		if(n < 0) n = str.size() + n;
		if(n < 0 || n >= str.size()) return new_node_string("");
		return new_node_string(str.substr(n));
	}
	if(coll_node->is_list()) return new_node_list(coll_node->t_list->drop(n));
	if(coll_node->is_vector()) return new_node_vector(coll_node->as_vector()->drop(n));
	if(coll_node->is_lazy_list()) {
		lazy_list_iterator_t lit(coll_idx);
		lit.nth(n);
		if(lit.done()) {
			return NIL_NODE;
		}
		return new_node_list(lit.all());
	}
	return NIL_NODE;
}

static node_idx_t native_nthnext(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t coll_idx = *it++;
	node_idx_t n_idx = *it++;
	node_t *coll_node = get_node(coll_idx);
	node_t *n_node = get_node(n_idx);
	long long n = n_node->as_int();
	if(n <= 0) return NIL_NODE;
	if(coll_node->is_string()) {
		jo_string &str = coll_node->t_string;
		if(n < 0) n = str.size() + n;
		if(n < 0 || n >= str.size()) return new_node_string("");
		return new_node_string(str.substr(n));
	}
	if(coll_node->is_list()) {
		if(n >= coll_node->t_list->size()) return NIL_NODE;
		return new_node_list(coll_node->t_list->take(n));
	}
	if(coll_node->is_vector()) {
		if(n >= coll_node->as_vector()->size()) return NIL_NODE;
		return new_node_vector(coll_node->as_vector()->take(n));
	}
	if(coll_node->is_lazy_list()) {
		lazy_list_iterator_t lit(coll_idx);
		lit.nth(n);
		if(lit.done()) {
			return NIL_NODE;
		}
		return new_node_list(lit.all());
	}
	return NIL_NODE;
}

// (-> x & forms)
// Threads the expr through the forms. Inserts x as the
// second item in the first form, making a list of it if it is not a
// list already. If there are more forms, inserts the first form as the
// second item in second form, etc.
static node_idx_t native_thread(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t x_idx = eval_node(env, *it++);
	for(; it; it++) {
		node_idx_t form_idx = *it;
		node_t *form_node = get_node(form_idx);
		list_ptr_t args2;
		if(get_node_type(*it) == NODE_LIST) {
			list_ptr_t form_list = form_node->t_list;
			args2 = form_list->rest();
			args2->push_front2_inplace(form_list->first_value(), x_idx);
		} else {
			args2 = new_list();
			args2->push_front2_inplace(*it, x_idx);
		}
		x_idx = eval_list(env, args2);
	}
	return x_idx;
}

// (->> x & forms)
// Threads the expr through the forms. Inserts x as the
// last item in the first form, making a list of it if it is not a
// list already. If there are more forms, inserts the first form as the
// last item in second form, etc.
static node_idx_t native_thread_last(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t x_idx = eval_node(env, *it++);
	for(; it; it++) {
		node_idx_t form_idx = *it;
		node_t *form_node = get_node(form_idx);
		list_ptr_t args2;
		if(get_node_type(*it) == NODE_LIST) {
			args2 = form_node->t_list->clone();
			args2->push_back_inplace(x_idx);
		} else {
			args2 = new_list();
			args2->push_front2_inplace(*it, x_idx);
		}
		x_idx = eval_list(env, args2);
	}
	return x_idx;
}


// (as-> expr name & forms)
// Binds name to expr, evaluates the first form in the lexical context
// of that binding, then binds name to that result, repeating for each
// successive form, returning the result of the last form.
static node_idx_t native_as_thread(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t expr_idx = eval_node(env, *it++);
	node_idx_t name_idx = *it++;
	jo_string name = get_node(name_idx)->t_string;
	env_ptr_t env2 = new_env(env);
	env2->set_temp(name, expr_idx);
	for(; it; it++) {
		node_idx_t form_idx = *it;
		node_t *form_node = get_node(form_idx);
		list_ptr_t args2;
		if(get_node_type(*it) == NODE_LIST) {
			list_ptr_t form_list = form_node->t_list;
			args2 = form_list->rest();
			args2->push_front_inplace(form_list->first_value());
		} else {
			args2 = new_list();
			args2->push_front_inplace(*it);
		}
		env2->set_temp(name, expr_idx);
		expr_idx = eval_list(env2, args2);
	}
	return expr_idx;
}

// (every? pred coll)
// Returns true if (pred x) is logical true for every x in coll, else
// false.
static node_idx_t native_is_every(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t pred_idx = *it++;
	node_idx_t coll_idx = *it++;
	node_t *pred_node = get_node(pred_idx);
	node_t *coll_node = get_node(coll_idx);
	if(!pred_node->can_eval()) return FALSE_NODE;
	node_idx_t ret = TRUE_NODE;
	if(!seq_iterate(coll_idx, [&](node_idx_t item) {
		if(!get_node_bool(eval_va(env, pred_idx, item))) {
			ret = FALSE_NODE;
			return false; // early out
		}
		return true; // keep goin
	})) {
		warnf("is_not_any: seq_iterate failed");
		return FALSE_NODE;
	}
	return ret;
}

// (not-every? pred coll)
// Returns false if (pred x) is logical true for every x in
// coll, else true.
static node_idx_t native_is_not_every(env_ptr_t env, list_ptr_t args) { return native_is_every(env, args) == TRUE_NODE ? FALSE_NODE : TRUE_NODE; }
static node_idx_t native_is_seqable(env_ptr_t env, list_ptr_t args) { return get_node(args->first_value())->is_seq() ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_any(env_ptr_t env, list_ptr_t args) { return TRUE_NODE; }
static node_idx_t native_boolean(env_ptr_t env, list_ptr_t args) { return get_node_bool(args->first_value()) ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_boolean(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_BOOL ? TRUE_NODE : FALSE_NODE; }

// (not-any? pred coll)
// Returns false if (pred x) is logical true for any x in coll,
// else true.
static node_idx_t native_is_not_any(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t pred_idx = *it++;
	node_idx_t coll_idx = *it++;
	node_t *pred_node = get_node(pred_idx);
	node_t *coll_node = get_node(coll_idx);
	if(!pred_node->can_eval()) return FALSE_NODE;
	node_idx_t ret = TRUE_NODE;
	if(!seq_iterate(coll_idx, [&](node_idx_t item) {
		if(get_node_bool(eval_va(env, pred_idx, item))) {
			ret = FALSE_NODE;
			return false; // early out
		}
		return true; // keep goin
	})) {
		warnf("is_not_any: seq_iterate failed");
		return FALSE_NODE;
	}
	return ret;
}

// (array-map)(array-map & keyvals)
// Constructs an array-map. If any keys are equal, they are handled as
// if by repeated uses of assoc.
static node_idx_t native_array_map(env_ptr_t env, list_ptr_t args) {
	if(args->size() & 1) {
		warnf("(array-map) requires an even number of arguments");
		return NIL_NODE;
	}
	hash_map_ptr_t map = new_hash_map();
	for(list_t::iterator it(args); it; ) {
		node_idx_t key_idx = *it++;
		node_idx_t val_idx = *it++;
		map = map->assoc(key_idx, val_idx, node_eq);
	}
	return new_node_map(map);
}

// (butlast coll)
// Return a seq of all but the last item in coll, in linear time
static node_idx_t native_butlast(env_ptr_t env, list_ptr_t args) {
	node_idx_t coll_idx = args->first_value();
	node_t *coll_node = get_node(coll_idx);
	if(coll_node->is_list()) {
		list_ptr_t coll_list = coll_node->t_list;
		if(coll_list->size() < 2) {
			return NIL_NODE;
		}
		list_ptr_t ret = new_list();
		for(list_t::iterator it(coll_list); it; ) {
			node_idx_t item_idx = *it++;
			if(!it) {
				break;
			}
			ret->push_back_inplace(*it);
		}
		return new_node_list(ret);
	}
	if(coll_node->is_vector()) {
		vector_ptr_t coll_vector = coll_node->as_vector();
		if(coll_vector->size() < 2) {
			return NIL_NODE;
		}
		vector_ptr_t ret = new_vector();
		for(vector_t::iterator it = coll_vector->begin(); it; ) {
			node_idx_t item_idx = *it++;
			if(!it) {
				break;
			}
			ret->push_back_inplace(*it);
		}
		return new_node_vector(ret);
	}
	if(coll_node->is_lazy_list()) {
		lazy_list_iterator_t lit(coll_idx);
		list_ptr_t ret = new_list();
		for(; !lit.done(); ) {
			node_idx_t item_idx = lit.val;
			lit.next();
			if(lit.done()) {
				break;
			}
			ret->push_back_inplace(lit.val);
		}
		return new_node_list(ret);
	}
	return NIL_NODE;
}

// (complement f)
// Takes a fn f and returns a fn that takes the same arguments as f,
// has the same effects, if any, and returns the opposite truth value.
static node_idx_t native_complement(env_ptr_t env, list_ptr_t args) {
	node_idx_t f_idx = args->first_value();
	node_t *f_node = get_node(f_idx);
	if(!f_node->is_func() && !f_node->is_native_func()) {
		warnf("(complement) requires a function");
		return NIL_NODE;
	}
	return new_node_native_function( "native_complement_fn", [f_idx](env_ptr_t env, list_ptr_t args) -> node_idx_t {
		return get_node_bool(eval_list(env, args->push_front(f_idx))) ? FALSE_NODE : TRUE_NODE;
	}, false);
}

// (cond-> 1         ; we start with 1
//    true inc       ; the condition is true so (inc 1) => 2
//    false (* 42)   ; the condition is false so the operation is skipped
//    (= 2 2) (* 3)) ; (= 2 2) is true so (* 2 3) => 6 
//;;=> 6
//;; notice that the threaded value gets used in 
//;; only the form and not the test part of the clause.
static node_idx_t native_cond_thread(env_ptr_t env, list_ptr_t args) {
	if((args->size() & 1) == 0) {
		warnf("(cond->) requires an odd number of arguments");
		return NIL_NODE;
	}

	list_t::iterator it(args);
	node_idx_t value_idx = eval_node(env, *it++);
	while(it) {
		node_idx_t test_idx = eval_node(env, *it++);
		node_idx_t form_idx = *it++;
		if(test_idx == TRUE_NODE) {
			int form_type = get_node_type(form_idx);
			if(form_type == NODE_SYMBOL || form_type == NODE_FUNC || form_type == NODE_NATIVE_FUNC) {
				list_ptr_t form_args = new_list();
				form_args->push_front2_inplace(form_idx, value_idx);
				value_idx = eval_list(env, form_args);
			} else if(form_type == NODE_LIST) {
				list_ptr_t form_args = get_node(form_idx)->t_list;
				node_idx_t sym = form_args->first_value();
				form_args = form_args->pop_front();
				form_args->push_front2_inplace(sym, value_idx);
				value_idx = eval_list(env, form_args);
			} else {
				warnf("(cond->) requires a symbol or list");
				return NIL_NODE;
			}
		}
	}
	return value_idx;
}

static node_idx_t native_cond_thread_last(env_ptr_t env, list_ptr_t args) {
	if((args->size() & 1) == 0) {
		warnf("(cond->>) requires an odd number of arguments");
		return NIL_NODE;
	}

	list_t::iterator it(args);
	node_idx_t value_idx = eval_node(env, *it++);
	while(it) {
		node_idx_t test_idx = eval_node(env, *it++);
		node_idx_t form_idx = *it++;
		if(test_idx == TRUE_NODE) {
			int form_type = get_node_type(form_idx);
			if(form_type == NODE_SYMBOL || form_type == NODE_FUNC || form_type == NODE_NATIVE_FUNC) {
				list_ptr_t form_args = new_list();
				form_args->push_front2_inplace(form_idx, value_idx);
				value_idx = eval_list(env, form_args);
			} else if(form_type == NODE_LIST) {
				list_ptr_t form_args = get_node_list(form_idx);
				form_args = form_args->push_back(value_idx);
				value_idx = eval_list(env, form_args);
			} else {
				warnf("(cond->>) requires a symbol or list");
				return NIL_NODE;
			}
		}
	}
	return value_idx;
}

// (condp pred expr & clauses)
// Takes a binary predicate, an expression, and a set of clauses.
// Each clause can take the form of either:
//  test-expr result-expr
// test-expr :>> result-fn
// Note :>> is an ordinary keyword.
// For each clause, (pred test-expr expr) is evaluated. If it returns
// logical true, the clause is a match. If a binary clause matches, the
// result-expr is returned, if a ternary clause matches, its result-fn,
// which must be a unary function, is called with the result of the
// predicate as its argument, the result of that call being the return
// value of condp. A single default expression can follow the clauses,
// and its value will be returned if no clause matches. If no default
// expression is provided and no clause matches, an
// IllegalArgumentException is thrown.
static node_idx_t native_condp(env_ptr_t env, list_ptr_t args) {
	long long num_args = args->size();
	if(num_args < 3) {
		warnf("(condp) requires at least 3 arguments\n");
		return NIL_NODE;
	}

	list_t::iterator it(args);
	node_idx_t pred_idx = eval_node(env, *it++);
	node_idx_t expr_idx = eval_node(env, *it++);
	int pred_type = get_node_type(pred_idx);
	if(pred_type != NODE_FUNC && pred_type != NODE_NATIVE_FUNC) {
		warnf("(condp) requires a function as its first argument\n");
		return NIL_NODE;
	}

	long long num_clauses = (num_args - 2) / 2;
	for(long long i = 0; i < num_clauses; i++) {
		node_idx_t test_idx = eval_node(env, *it++);
		node_idx_t form_idx = *it++;
		list_ptr_t test_args = new_list();
		test_args->push_front3_inplace(pred_idx, test_idx, expr_idx);
		if(eval_list(env, test_args) == TRUE_NODE) {
			return eval_node(env, form_idx);
		}
	}

	if(num_clauses * 2 + 2 != num_args) {
		return eval_node(env, *it++);
	}

	warnf("(condp) no clause matched\n");
	return NIL_NODE;
}

// (contains? coll key)
// Returns true if key is present in the given collection, otherwise
// returns false.  Note that for numerically indexed collections like
// vectors and Java arrays, this tests if the numeric key is within the
// range of indexes. 'contains?' operates constant or logarithmic time;
// it will not perform a linear search for a value.  See also 'some'.
static node_idx_t native_is_contains(env_ptr_t env, list_ptr_t args) {
	if(args->size() != 2) {
		warnf("(contains?) requires 2 arguments\n");
		return NIL_NODE;
	}

	node_idx_t coll_idx = args->first_value();
	node_idx_t key_idx = args->second_value();
	int coll_type = get_node_type(coll_idx);
	if(coll_type == NODE_LIST) {
		list_ptr_t coll = get_node_list(coll_idx);
		long long key = get_node_int(key_idx);
		return key >= 0 && key < coll->size() ? TRUE_NODE : FALSE_NODE;
	} else if(coll_type == NODE_VECTOR) {
		vector_ptr_t coll = get_node_vector(coll_idx);
		long long key = get_node_int(key_idx);
		return key >= 0 && key < coll->size() ? TRUE_NODE : FALSE_NODE;
	} else if(coll_type == NODE_MAP) {
		hash_map_ptr_t coll = get_node_map(coll_idx);
		return coll->contains(key_idx, node_eq) ? TRUE_NODE : FALSE_NODE;
	} else if(coll_type == NODE_HASH_SET) {
		hash_set_ptr_t coll = get_node_set(coll_idx);
		return coll->contains(key_idx, node_eq) ? TRUE_NODE : FALSE_NODE;
	}
	warnf("(contains?) requires a collection\n");
	return NIL_NODE;
}

// (counted? coll)
// Returns true if coll implements count in constant time
static node_idx_t native_is_counted(env_ptr_t env, list_ptr_t args) {
	node_idx_t coll_idx = args->first_value();
	int coll_type = get_node_type(coll_idx);
	if(coll_type == NODE_LIST) return TRUE_NODE;
	if(coll_type == NODE_VECTOR) return TRUE_NODE;
	if(coll_type == NODE_MAP) return TRUE_NODE;
	if(coll_type == NODE_HASH_SET) return TRUE_NODE;
	return FALSE_NODE;
}

// (distinct? x)(distinct? x y)(distinct? x y & more)
// Returns true if no two of the arguments are =
static node_idx_t native_is_distinct(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		warnf("(distinct?) requires at least 2 arguments\n");
		return NIL_NODE;
	}

	for(list_t::iterator it(args); it; it++) {
		for(auto it2 = it+1; it2; it2++) {
			if(node_eq(*it, *it2)) {
				return FALSE_NODE;
			}
		}
	}
	return TRUE_NODE;
}

// (empty coll)
// Returns an empty collection of the same category as coll, or nil
// if coll is not a collection.
static node_idx_t native_empty(env_ptr_t env, list_ptr_t args) {
	switch(get_node_type(args->first_value())) {
		case NODE_LIST: return EMPTY_LIST_NODE;
		case NODE_VECTOR: return EMPTY_VECTOR_NODE;
		case NODE_MAP: return EMPTY_MAP_NODE;
		case NODE_HASH_SET: return EMPTY_SET_NODE;
		case NODE_LAZY_LIST: return EMPTY_LIST_NODE;
		default: return NIL_NODE;
	}
}

// (not-empty coll)
// If coll is empty, returns nil, else coll
static node_idx_t native_not_empty(env_ptr_t env, list_ptr_t args) {
	node_idx_t coll_idx = args->first_value();
	if(get_node(coll_idx)->seq_empty()) return NIL_NODE;
	return coll_idx;
}


// (every-pred p)(every-pred p1 p2)(every-pred p1 p2 p3)(every-pred p1 p2 p3 & ps)
// Takes a set of predicates and returns a function f that returns true if all of its
// composing predicates return a logical true value against all of its arguments, else it returns
// false. Note that f is short-circuiting in that it will stop execution on the first
// argument that triggers a logical false result against the original predicates.
static node_idx_t native_every_pred(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 1) {
		warnf("(every-pred) requires at least 1 arguments\n");
		return NIL_NODE;
	}

	return new_node_native_function("every-pred-lambda", [args](env_ptr_t env, list_ptr_t args2) {
		for(list_t::iterator it(args); it; it++) {
			if(!eval_node_list(env, args2->push_front(*it))) {
				return FALSE_NODE;
			}
		}
		return TRUE_NODE;
	}, false);
}

// (find map key)
// Returns the map entry for key, or nil if key not present.
static node_idx_t native_find(env_ptr_t env, list_ptr_t args) {
	if(args->size() != 2) {
		warnf("(find) requires 2 arguments\n");
		return NIL_NODE;
	}
	
	node_idx_t map_idx = eval_node(env, args->first_value());
	node_idx_t key_idx = eval_node(env, args->second_value());
	hash_map_ptr_t map = get_node_map(map_idx);
	auto kv = map->find(key_idx, node_eq);
	if(!kv.third) {
		return NIL_NODE;
	}
	vector_ptr_t vec = new_vector();
	vec->push_back_inplace(kv.first);
	vec->push_back_inplace(kv.second);
	return new_node_vector(vec);
}

// (fnil f x)(fnil f x y)(fnil f x y z)
// Takes a function f, and returns a function that calls f, replacing
// a nil first argument to f with the supplied value x. Higher arity
// versions can replace arguments in the second and third
// positions (y, z). Note that the function f can take any number of
// arguments, not just the one(s) being nil-patched.
static node_idx_t native_fnil(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		warnf("(fnil) requires at least 2 arguments\n");
		return NIL_NODE;
	}

	return new_node_native_function("fnil-lambda", [args](env_ptr_t env, list_ptr_t args2) {
		list_t::iterator it(args);
		list_t::iterator it2(args2);
		node_idx_t f_idx = *it++;
		list_ptr_t new_args = new_list();
		new_args->push_back_inplace(f_idx);
		for(; it && it2; it++, it2++) {
			new_args->push_back_inplace(*it2 == NIL_NODE ? *it : *it2);
		}
		for(; it2; it2++) {
			new_args->push_back_inplace(*it2);
		}
		return eval_list(env, new_args);
	}, false);
}

// (split-at n coll)
// Returns a vector of [(take n coll) (drop n coll)]
static node_idx_t native_split_at(env_ptr_t env, list_ptr_t args) {
	if(args->size() != 2) {
		warnf("(split-at) requires 2 arguments\n");
		return NIL_NODE;
	}
	node_idx_t n_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	long long n = get_node_int(n_idx);
	if(n < 0) {
		warnf("(split-at) requires a positive integer\n");
		return NIL_NODE;
	}
	int coll_type = get_node_type(coll_idx);
	vector_ptr_t vec = new_vector();
	if(coll_type == NODE_LIST) {
		list_ptr_t coll = get_node_list(coll_idx);
		if(n > coll->size()) {
			n = coll->size();
		}
		vec->push_back_inplace(new_node_list(coll->take(n)));
		vec->push_back_inplace(new_node_list(coll->drop(n)));
	} else if(coll_type == NODE_STRING) {
		jo_string coll = get_node_string(coll_idx);
		if(n > coll.size()) {
			n = coll.size();
		}
		jo_string coll2 = coll;
		vec->push_back_inplace(new_node_string(coll.take(n)));
		vec->push_back_inplace(new_node_string(coll2.drop(n)));
	} else if(coll_type == NODE_VECTOR) {
		vector_ptr_t coll = get_node_vector(coll_idx);
		if(n > coll->size()) {
			n = coll->size();
		}
		vec->push_back_inplace(new_node_vector(coll->take(n)));
		vec->push_back_inplace(new_node_vector(coll->drop(n)));
	} else if(coll_type == NODE_LAZY_LIST) {
		list_ptr_t A = new_list();
		list_ptr_t B = new_list();
		lazy_list_iterator_t lit(coll_idx);
		for(; lit; lit.next()) {
			if(A->size() < n) {
				break;
			}
			A->push_back_inplace(lit.val);
		}
		for(; lit; lit.next()) {
			B->push_back_inplace(lit.val);
		}
		vec->push_back_inplace(new_node_list(A));
		vec->push_back_inplace(new_node_list(B));
	} else {
		warnf("(split-at) requires a list or string\n");
		return NIL_NODE;
	}
	return new_node_vector(vec);
}

// (split-with pred coll)
// Returns a vector of [(take-while pred coll) (drop-while pred coll)]
static node_idx_t native_split_with(env_ptr_t env, list_ptr_t args) {
	if(args->size() != 2) {
		warnf("(split-with) requires 2 arguments\n");
		return NIL_NODE;
	}
	node_idx_t pred_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	int coll_type = get_node_type(coll_idx);
	list_ptr_t A = new_list();
	list_ptr_t B = new_list();
	vector_ptr_t vec = new_vector();
	if(coll_type == NODE_LIST) {
		list_ptr_t coll = get_node_list(coll_idx);
		list_t::iterator it(coll);
		for(; it; it++) {
			if(!get_node_bool(eval_va(env, pred_idx, *it))) {
				break;
			}
			A->push_back_inplace(*it);
		}
		for(; it; it++) {
			B->push_back_inplace(*it);
		}
	} else if(coll_type == NODE_STRING) {
		jo_string coll = get_node_string(coll_idx);
		auto it = coll.begin();
		auto it_end = coll.end();
		for(; it != it_end; it++) {
			if(!get_node_bool(eval_va(env, pred_idx, *it))) {
				break;
			}
			A->push_back_inplace(*it);
		}
		for(; it != it_end; it++) {
			B->push_back_inplace(*it);
		}
	} else if(coll_type == NODE_VECTOR) {
		vector_ptr_t coll = get_node_vector(coll_idx);
		auto it = coll->begin();
		for(; it; it++) {
			if(!get_node_bool(eval_va(env, pred_idx, *it))) {
				break;
			}
			A->push_back_inplace(*it);
		}
		for(; it; it++) {
			B->push_back_inplace(*it);
		}
	} else if(coll_type == NODE_LAZY_LIST) {
		lazy_list_iterator_t lit(coll_idx);
		for(; lit; lit.next()) {
			if(!get_node_bool(eval_va(env, pred_idx, lit.val))) {
				break;
			}
			A->push_back_inplace(lit.val);
		}
		for(; lit; lit.next()) {
			B->push_back_inplace(lit.val);
		}
	} else {
		warnf("(split-with) requires a list, string, vector, or lazy-list\n");
		return NIL_NODE;
	}
	vec->push_back_inplace(new_node_list(A));
	vec->push_back_inplace(new_node_list(B));
	return new_node_vector(vec);
}

// (frequencies coll)
// Returns a map from distinct items in coll to the number of times
// they appear.
static node_idx_t native_frequencies(env_ptr_t env, list_ptr_t args) {
	if(args->size() != 1) {
		warnf("(frequencies) requires 1 argument\n");
		return NIL_NODE;
	}
	node_idx_t coll_idx = args->first_value();
	int coll_type = get_node_type(coll_idx);
	hash_map_ptr_t map = new_hash_map();
	if(!seq_iterate(coll_idx, [&](node_idx_t item) {
		map->assoc_inplace(item, new_node_int(get_node_int(map->get(item, node_eq)) + 1), node_eq);
		return true;
	})) {
		warnf("(frequencies) requires a collection\n");
		return NIL_NODE;
	}
	return new_node_map(map);
}

// (get-in m ks)(get-in m ks not-found)
// Returns the value in a nested associative structure,
// where ks is a sequence of keys. Returns nil if the key
// is not present, or the not-found value if supplied.
static node_idx_t native_get_in(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2 || args->size() > 3) {
		warnf("(get-in) requires 2 or 3 arguments\n");
		return NIL_NODE;
	}
	node_idx_t m_idx = args->first_value();
	node_idx_t ks_idx = args->second_value();
	node_idx_t not_found_idx = args->size() == 3 ? args->third_value() : NIL_NODE;
	int ks_type = get_node_type(ks_idx);
	if(ks_type != NODE_VECTOR) {
		warnf("(get-in) requires a vector\n");
		return NIL_NODE;
	}
	node_idx_t result = m_idx;
	vector_ptr_t ks = get_node_vector(ks_idx);
	for(auto it = ks->begin(); it; it++) {
		result = native_get(env, list_va(result, *it));
		if(result == NIL_NODE) {
			return not_found_idx;
		}
	}
	return result;
}

// (group-by f coll)
// Returns a map of the elements of coll keyed by the result of
// f on each element. The value at each key will be a vector of the
// corresponding elements, in the order they appeared in coll.
static node_idx_t native_group_by(env_ptr_t env, list_ptr_t args) {
	if(args->size() != 2) {
		warnf("(group-by) requires 2 arguments\n");
		return NIL_NODE;
	}
	node_idx_t f_idx = args->first_value();
	node_idx_t coll_idx = args->second_value();
	int coll_type = get_node_type(coll_idx);
	hash_map_ptr_t map = new_hash_map();
	seq_iterate(coll_idx, [&](node_idx_t item) {
		node_idx_t key = eval_va(env, f_idx, item);
		node_idx_t V_idx = map->get(key, node_eq);
		node_t *V = get_node(V_idx);
		if(V->type == NODE_NIL) {
			map = map->assoc(key, new_node_vector(new_vector()->push_back(item)), node_eq);
		} else {
			V->as_vector()->push_back_inplace(item);
		}
		return true;
	});
	return new_node_map(map);
}

// (hash x)
// Returns the hash code of its argument. Note this is the hash code
// consistent with =, and thus is different than .hashCode for Integer,
// Short, Byte and Clojure collections.
static node_idx_t native_hash(env_ptr_t env, list_ptr_t args) {
	size_t hash0 = 2166136261, hash1 = 2166136261, hash2 = 2166136261, hash3 = 2166136261;
	size_t hash4 = 2166136261, hash5 = 2166136261, hash6 = 2166136261, hash7 = 2166136261;
	for(list_t::iterator it(args); it; it++) {
		size_t h = jo_hash_value(*it);
		hash0 = (16777619 * hash0) ^ ((h>>0) & 255);
		hash1 = (16777619 * hash1) ^ ((h>>8) & 255);
		hash2 = (16777619 * hash2) ^ ((h>>16) & 255);
		hash3 = (16777619 * hash3) ^ ((h>>24) & 255);
		hash4 = (16777619 * hash4) ^ ((h>>32) & 255);
		hash5 = (16777619 * hash5) ^ ((h>>40) & 255);
		hash6 = (16777619 * hash6) ^ ((h>>48) & 255);
		hash7 = (16777619 * hash7) ^ ((h>>56) & 255);
	}
	size_t hash = ((hash0 ^ hash1) ^ (hash2 ^ hash3)) ^ ((hash4 ^ hash5) ^ (hash6 ^ hash7));
	return new_node_int(hash & 0x7FFFFFFFFFFFFFFFull);
}

// (hash-combine x y)
// Calculates the hashes for x and y and produces a new hash that represents
// the combination of the two.
static node_idx_t native_hash_combine(env_ptr_t env, list_ptr_t args) {
	return new_node_int(jo_hash_value(new_node_list(list_va(args->first_value(), args->second_value()))));
}

// (hash-set)(hash-set & keys)
// Returns a new hash set with supplied keys.  Any equal keys are
// handled as if by repeated uses of conj.
static node_idx_t native_hash_set(env_ptr_t env, list_ptr_t args) {
	hash_set_ptr_t set = new_hash_set();
	if(args->size() == 1) {
		if(seq_iterate(args->first_value(), [&](node_idx_t item) {
			set = set->assoc(item, node_eq);
			return true;
		})) {
			return new_node_hash_set(set);
		}
	}
	list_t::iterator it(args);
	while(it) {
		node_idx_t k = eval_node(env, *it++);
		set = set->assoc(k, node_eq);
	}
	return new_node_hash_set(set);
}

// (identical? x y)
// Tests if 2 arguments are the same object
static node_idx_t native_is_identical(env_ptr_t env, list_ptr_t args) {
	return args->first_value() == args->second_value() ? TRUE_NODE : FALSE_NODE;
}

// (juxt f)(juxt f g)(juxt f g h)(juxt f g h & fs)
// Takes a set of functions and returns a fn that is the juxtaposition
// of those fns.  The returned fn takes a variable number of args, and
// returns a vector containing the result of applying each fn to the
// args (left-to-right).
// ((juxt a b c) x) => [(a x) (b x) (c x)]
static node_idx_t native_juxt(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		warnf("(juxt) requires at least 2 arguments\n");
		return NIL_NODE;
	}
	return new_node_native_function("juxt-lambda", [args](env_ptr_t env, list_ptr_t args2) {
		vector_ptr_t result = new_vector();
		for(list_t::iterator it(args); it; it++) {
			result->push_back_inplace(eval_list(env, args2->push_front(*it)));
		}
		return new_node_vector(result);
	}, false);
}

static node_idx_t native_name(env_ptr_t env, list_ptr_t args) {
	return new_node_string(get_node(args->first_value())->t_string);
}

// (keyword name)(keyword ns name)
// Returns a Keyword with the given namespace and name.  Do not use :
// in the keyword strings, it will be added automatically.
static node_idx_t native_keyword(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 2) {
		return new_node_keyword(get_node_string(args->first_value()) + "/" + get_node_string(args->second_value()));
	}
	return new_node_keyword(get_node_string(args->first_value()));
}

// (letfn [fnspecs*] exprs*)
// fnspec ==> (fname [params*] exprs) or (fname ([params*] exprs)+)
// Takes a vector of function specs and a body, and generates a set of
// bindings of functions to their names. All of the names are available
// in all of the definitions of the functions, as well as the body.
static node_idx_t native_letfn(env_ptr_t env, list_ptr_t args) {
	node_idx_t fnspecs_idx = args->first_value();
	if(get_node_type(fnspecs_idx) != NODE_VECTOR) {
		warnf("(letfn) requires a vector of function specs\n");
		return NIL_NODE;
	}
	env_ptr_t E = new_env(env);
	vector_ptr_t fnspecs = get_node(fnspecs_idx)->as_vector();
	for(auto it = fnspecs->begin(); it; ++it) {
		node_idx_t fnspec_idx = *it;
		if(get_node_type(fnspec_idx) != NODE_LIST) {
			warnf("(letfn) requires a list of function specs\n");
			return NIL_NODE;
		}
		list_ptr_t fnspec = get_node(fnspec_idx)->as_list();
		node_idx_t fname_idx = fnspec->first_value();
		if(get_node_type(fname_idx) != NODE_SYMBOL) {
			warnf("(letfn) requires a symbol for the function name\n");
			return NIL_NODE;
		}
		node_idx_t fn_idx = native_fn(E, fnspec->rest());
		E->set_temp(get_node_string(fname_idx), fn_idx);
	}
	return eval_node_list(E, args->rest());
}

// (load-file name)
// Sequentially read and evaluate the set of forms contained in the file.
static node_idx_t native_load_file(env_ptr_t env, list_ptr_t args) {
	node_idx_t name_idx = args->first_value();
	if(get_node_type(name_idx) != NODE_STRING) {
		warnf("(load-file) requires a string\n");
		return NIL_NODE;
	}
	jo_string name = get_node_string(name_idx);
	FILE *fp = fopen(name.c_str(), "r");
	if(!fp) {
		return NIL_NODE;
	}

	parse_state_t parse_state;
	parse_state.fp = fp;

	// parse the base list
	list_ptr_t main_list = new_list();
	for(node_idx_t next = parse_next(env, &parse_state, 0); next != INV_NODE; next = parse_next(env, &parse_state, 0)) {
		main_list->push_back_inplace(next);
	}
	fclose(fp);

	return eval_node_list(env, main_list);
}

// (load-reader rdr)
// Sequentially read and evaluate the set of forms contained in the
// stream/file
static node_idx_t native_load_reader(env_ptr_t env, list_ptr_t args) {
	node_idx_t rdr_idx = args->first_value();
	if(get_node_type(rdr_idx) != NODE_FILE) {
		warnf("(load-reader) requires a file type\n");
		return NIL_NODE;
	}

	parse_state_t parse_state;
	parse_state.fp = get_node(rdr_idx)->t_file;
	if(!parse_state.fp) {
		return NIL_NODE;
	}

	// parse the base list
	list_ptr_t main_list = new_list();
	for(node_idx_t next = parse_next(env, &parse_state, 0); next != INV_NODE; next = parse_next(env, &parse_state, 0)) {
		main_list->push_back_inplace(next);
	}

	return eval_node_list(env, main_list);
}

// (load-string s)
// Sequentially read and evaluate the set of forms contained in the
// string
static node_idx_t native_load_string(env_ptr_t env, list_ptr_t args) {
	jo_string s = get_node_string(args->first_value());

	parse_state_t parse_state;
	//parse_state.fp = jo_fmemopen((void*)s.c_str(), s.size(), "r");
	parse_state.buf = s.c_str();
	parse_state.buf_end = s.c_str() + s.size();

	// parse the base list
	list_ptr_t main_list = new_list();
	for(node_idx_t next = parse_next(env, &parse_state, 0); next != INV_NODE; next = parse_next(env, &parse_state, 0)) {
		main_list->push_back_inplace(next);
	}

	return eval_node_list(env, main_list);
}

// (loop [bindings*] exprs*)
// Evaluates the exprs in a lexical context in which the symbols in
// the binding-forms are bound to their respective init-exprs or parts
// therein. Acts as a recur target.
static node_idx_t native_loop(env_ptr_t env, list_ptr_t args) {
	node_idx_t res_idx = native_let(env, args);
	node_t *res = get_node(res_idx);
	if(res->type == NODE_RECUR) {
		vector_ptr_t B = get_node_vector(args->first_value());
		env_ptr_t env2 = new_env(env);
		do {
			auto B_it = B->begin();
			list_t::iterator recur_it(res->t_list);
			for(; B_it && recur_it; ) {
				node_idx_t key_idx = *B_it++;
				*B_it++; // old binding
				node_let(env2, key_idx, *recur_it++);
			}
			res_idx = eval_node_list(env2, args->rest());
			res = get_node(res_idx);
		} while(res->type == NODE_RECUR);
	}
	return res_idx;
}

// (recur bindings*)
// Evaluates the exprs in order, then, in parallel, rebinds the bindings of
// the recursion point to the values of the exprs. See
// http://clojure.org/special_forms for more information.
static node_idx_t native_recur(env_ptr_t env, list_ptr_t args) {
	node_idx_t res_idx = new_node(NODE_RECUR, 0);
	node_t *res = get_node(res_idx);
	res->t_list = args;
	return res_idx;
}

// (mapv f coll)(mapv f c1 c2)(mapv f c1 c2 c3)(mapv f c1 c2 c3 & colls)
// Returns a vector consisting of the result of applying f to the
// set of first items of each coll, followed by applying f to the set
// of second items in each coll, until any one of the colls is
// exhausted.  Any remaining items in other colls are ignored. Function
// f should accept number-of-colls arguments.
static node_idx_t native_mapv(env_ptr_t env, list_ptr_t args) {
	node_idx_t f = args->first_value();
	list_ptr_t colls = args->rest();
	vector_ptr_t r = new_vector();
	do {
		list_ptr_t new_colls = new_list();
		list_ptr_t arg_list = new_list();
		arg_list->push_back_inplace(f);
		bool done = false;
		for(list_t::iterator it(colls); it; it++) {
			node_idx_t arg_idx = *it;
			node_t *arg = get_node(arg_idx);
			auto fr = arg->seq_first_rest();
			if(!fr.third) {
				done = true;
				break;
			}
			arg_list->push_back_inplace(fr.first);
			new_colls->push_back_inplace(fr.second);
		}
		if(done) {
			break;
		}
		r->push_back_inplace(eval_list(env, arg_list));
		colls = new_colls;
	} while(true);
	return new_node_vector(r);
}

static node_idx_t native_vector(env_ptr_t env, list_ptr_t args) {
	vector_ptr_t r = new_vector();
	for(list_t::iterator it(args); it; it++) {
		r->push_back_inplace(*it);
	}
	return new_node_vector(r);
}

static node_idx_t native_vector2d(env_ptr_t env, list_ptr_t args) {
	return new_node_vector2d(new_vector2d(get_node_int(args->first_value()), get_node_int(args->second_value())));
}

// (max-key k x)(max-key k x y)(max-key k x y & more)
// Returns the x for which (k x), a number, is greatest.
// If there are multiple such xs, the last one is returned.
static node_idx_t native_max_key(env_ptr_t env, list_ptr_t args) {
	node_idx_t k = args->first_value();
	list_ptr_t colls = args->rest();
	node_idx_t max_idx = INV_NODE;
	double max_val = -INFINITY;
	for(list_t::iterator it(colls); it; it++) {
		double val = get_node_float(eval_va(env, k, *it));
		if(val > max_val) {
			max_idx = *it;
			max_val = val;
		}
	}
	return max_idx;
}

// (min-key k x)(min-key k x y)(min-key k x y & more)
// Returns the x for which (k x), a number, is smallest.
// If there are multiple such xs, the last one is returned.
static node_idx_t native_min_key(env_ptr_t env, list_ptr_t args) {
	node_idx_t k = args->first_value();
	list_ptr_t colls = args->rest();
	node_idx_t min_idx = INV_NODE;
	double min_val = -INFINITY;
	for(list_t::iterator it(colls); it; it++) {
		double val = get_node_float(eval_va(env, k, *it));
		if(val < min_val) {
			min_idx = *it;
			min_val = val;
		}
	}
	return min_idx;
}


static node_idx_t native_key(env_ptr_t env, list_ptr_t args) {
	node_idx_t map_entry = args->first_value();
	node_t *map_entry_node = get_node(map_entry);
	if(map_entry_node->type != NODE_VECTOR) {
		warnf("val: expected vector, got %s\n", map_entry_node->type_name());
		return NIL_NODE;
	}
	vector_ptr_t map_entry_vec = map_entry_node->as_vector();
	if(map_entry_vec->size() != 2) {
		warnf("val: expected vector of size 2, got %zu\n", map_entry_vec->size());
		return NIL_NODE;
	}
	return map_entry_vec->first_value();
}

static node_idx_t native_val(env_ptr_t env, list_ptr_t args) {
	node_idx_t map_entry = args->first_value();
	node_t *map_entry_node = get_node(map_entry);
	if(map_entry_node->type != NODE_VECTOR) {
		warnf("val: expected vector, got %s\n", map_entry_node->type_name());
		return NIL_NODE;
	}
	vector_ptr_t map_entry_vec = map_entry_node->as_vector();
	if(map_entry_vec->size() != 2) {
		warnf("val: expected vector of size 2, got %zu\n", map_entry_vec->size());
		return NIL_NODE;
	}
	return map_entry_vec->nth(1);
}

// (merge & maps)
// Returns a map that consists of the rest of the maps conj-ed onto
// the first.  If a key occurs in more than one map, the mapping from
// the latter (left-to-right) will be the mapping in the result.
static node_idx_t native_merge(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t map_first_idx = *it++;
	node_t *map_first_node = get_node(map_first_idx);
	hash_map_ptr_t r;
	if(map_first_node->type != NODE_MAP) {
		r = new_hash_map();
	} else {
		r = map_first_node->as_hash_map();
	}
	for(; it; it++) {
		node_idx_t map_idx = *it;
		node_t *map_node = get_node(map_idx);
		if(map_node->type != NODE_MAP) {
			continue;
		}
		hash_map_ptr_t map = map_node->as_hash_map();
		for(hash_map_t::iterator it2 = map->begin(); it2; it2++) {
			r = r->assoc(it2->first, it2->second, node_eq);
		}
	}
	if(r->size() == 0) {
		return NIL_NODE;
	}
	return new_node_map(r);
}

// (merge-with f & maps)
// Returns a map that consists of the rest of the maps conj-ed onto
// the first.  If a key occurs in more than one map, the mapping(s)
// from the latter (left-to-right) will be combined with the mapping in
// the result by calling (f val-in-result val-in-latter).
static node_idx_t native_merge_with(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_idx_t f = *it++;
	node_idx_t map_first_idx = *it++;
	node_t *map_first_node = get_node(map_first_idx);
	hash_map_ptr_t r;
	if(map_first_node->type != NODE_MAP) {
		r = new_hash_map();
	} else {
		r = map_first_node->as_hash_map();
	}
	for(; it; it++) {
		node_idx_t map_idx = *it;
		node_t *map_node = get_node(map_idx);
		if(map_node->type != NODE_MAP) {
			continue;
		}
		hash_map_ptr_t map = map_node->as_hash_map();
		for(hash_map_t::iterator it2 = map->begin(); it2; it2++) {
			auto e = r->find(it2->first, node_eq);
			if(e.third) {
				r = r->assoc(it2->first, eval_va(env, f, e.second, it2->second), node_eq);
			} else {
				r = r->assoc(it2->first, it2->second, node_eq);
			}
		}
	}
	if(r->size() == 0) {
		return NIL_NODE;
	}
	return new_node_map(r);
}

// (namespace x)
// Returns the namespace String of a symbol or keyword, or nil if not present.
static node_idx_t native_namespace(env_ptr_t env, list_ptr_t args) {
	node_idx_t sym_idx = args->first_value();
	node_t *sym_node = get_node(sym_idx);
	size_t ns_pos = sym_node->t_string.find_last_of('/');
	if(ns_pos == jo_npos) {
		return NIL_NODE;
	}
	return new_node_string(sym_node->t_string.substr(0, ns_pos));
}

static node_idx_t native_newline(env_ptr_t env, list_ptr_t args) { printf("\n"); return NIL_NODE; }

// (reduce-kv f init coll)
// Reduces an associative collection. f should be a function of 3
// arguments. Returns the result of applying f to init, the first key
// and the first value in coll, then applying f to that result and the
// 2nd key and value, etc. If coll contains no entries, returns init
// and f is not called. Note that reduce-kv is supported on vectors,
// where the keys will be the ordinals.
static node_idx_t native_reduce_kv(env_ptr_t env, list_ptr_t args) {
	if(args->size() != 3) {
		warnf("reduce-kv: expected 3 arguments, got %zu\n", args->size());
		return NIL_NODE;
	}
	list_t::iterator it(args);
	node_idx_t f = *it++;
	node_idx_t init = *it++;
	node_idx_t coll = *it++;
	node_t *coll_node = get_node(coll);
	if(coll_node->type == NODE_VECTOR) {
		vector_ptr_t coll_vec = coll_node->as_vector();
		size_t size = coll_vec->size();
		if(size == 0) {
			return init;
		}
		node_idx_t result = init;
		for(size_t i = 0; i < size; i++) {
			result = eval_va(env, f, result, new_node_int(i), coll_vec->nth(i));
			if(get_node_type(result) == NODE_REDUCED) {
				return get_node(result)->t_extra;
			}
		}
		return result;
	}
	if(coll_node->type == NODE_MAP) {
		hash_map_ptr_t coll_map = coll_node->as_hash_map();
		if(coll_map->size() == 0) {
			return init;
		}
		node_idx_t result = init; 
		for(hash_map_t::iterator it2 = coll_map->begin(); it2; it2++) {
			result = eval_va(env, f, result, it2->first, it2->second);
			if(get_node_type(result) == NODE_REDUCED) {
				return get_node(result)->t_extra;
			}
		}
		return result;
	}
	warnf("reduce-kv: expected a collection, got %s\n", get_node_type_string(coll_node->type));
	return NIL_NODE;
}

static node_idx_t native_read_string(env_ptr_t env, list_ptr_t args) {
	jo_string s = get_node_string(args->first_value());

	parse_state_t parse_state;
	parse_state.buf = s.c_str();
	parse_state.buf_end = s.c_str() + s.size();

	return eval_node_list(env, list_va(parse_next(env, &parse_state, 0)));
}

static node_idx_t native_replace(env_ptr_t env, list_ptr_t args) {
	node_idx_t f = args->first_value();
	node_idx_t coll_idx = args->second_value();
	vector_ptr_t r = new_vector();
	do {
		node_t *coll = get_node(coll_idx);
		auto fr = coll->seq_first_rest();
		if(!fr.third) {
			break;
		}
		node_idx_t tmp = eval_va(env, f, fr.first);
		coll_idx = fr.second;
		r->push_back_inplace(tmp ? tmp : fr.first);
	} while(true);
	return new_node_vector(r);
}

static node_idx_t native_vec(env_ptr_t env, list_ptr_t args) {
	vector_ptr_t r = new_vector();
	seq_iterate(args->first_value(), [&](node_idx_t idx) {
		r->push_back_inplace(idx);
		return true;
	});
	return new_node_vector(r);
}


#include "jo_clojure_math.h"
#include "jo_clojure_string.h"
#include "jo_clojure_system.h"
#include "jo_clojure_io.h"
#include "jo_clojure_lazy.h"
#include "jo_clojure_async.h"
#include "jo_clojure_gif.h"

#ifdef _MSC_VER
#pragma comment(lib,"AdvApi32.lib")
#pragma comment(lib,"User32.lib")
//#pragma comment(lib,"Comdlg32.lib")
//#pragma comment(lib,"Shell32.lib")
//#pragma comment(lib,"legacy_stdio_definitions.lib")
static char real_exe_path[MAX_PATH];
static bool IsRegistered(const char *ext) {
	HKEY hKey;
	char keystr[256];
	sprintf(keystr, "%s.Document\\shell\\open\\command", ext);
	LONG ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, keystr, 0, KEY_READ, &hKey);
	if (ret == ERROR_SUCCESS) {
		DWORD n = 4;
		ret = RegQueryValueExA(hKey, NULL, NULL, NULL, 0, &n);
		if (ret == ERROR_SUCCESS) {
			char *reg_data = (char *)malloc(n);
			ret = RegQueryValueExA(hKey, NULL, NULL, NULL, (unsigned char *)reg_data, &n);
			if (ret == ERROR_SUCCESS && strstr(reg_data, real_exe_path)) {
				return true;
			}
		}
	}
	return false;
}
#endif

int main(int argc, char **argv) {
#ifdef WITH_TELEMETRY
	tmLoadLibrary(TM_RELEASE);
	tm_int32 telemetry_memory_size = 128 * 1024 * 1024;
	char* telemetry_memory = (char*)malloc(telemetry_memory_size);
	tmInitialize(telemetry_memory_size, telemetry_memory);
	tm_error err = tmOpen(0, "jo_clojure", __DATE__ " " __TIME__, "localhost", TMCT_TCP, 4719, TMOF_INIT_NETWORKING, 100);
	tmThreadName(0,0,"main");
	tmProfileThread(0,0,0);
#endif

#ifdef _MSC_VER
    if(argc == 1) {
		GetModuleFileNameA(GetModuleHandle(NULL), real_exe_path, MAX_PATH);
		bool register_clj = !IsRegistered("CLJ") && (MessageBoxA(0, "Do you want to register .CLJ files with this program?", "JO_CLOJURE", MB_OKCANCEL) == 1);
		if(register_clj) {
			char tmp[128];
			sprintf(tmp, "%s.reg", tmpnam(0));
			FILE *fp = fopen(tmp, "w");
			if (fp) {
				char exe_path[MAX_PATH * 2] = {0};
				for (int i = 0, j = 0; real_exe_path[i]; ++i, ++j) {
					exe_path[j] = real_exe_path[i];
					if (exe_path[j] == '\\') {
						exe_path[++j] = '\\';
					}
				}
				fprintf(fp, "Windows Registry Editor Version 5.00\n\n");
				if (register_clj) {
					fprintf(fp, "[HKEY_CLASSES_ROOT\\.clj]\n@=\"CLJ.Document\"\n\n");
					fprintf(fp, "[HKEY_CLASSES_ROOT\\CLJ.Document\\shell\\open\\command]\n@=\"%s %%1\"\n\n", exe_path);
				}
				fclose(fp);
				system(tmp);
				remove(tmp);
			}
		}
	}
#endif

	if(argc <= 1) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		return 1;
	}

	srand(0);
	jo_rnd_state = jo_pcg32_init(0);

	debugf("Setting up environment...\n");

	env_ptr_t env = new_env(NULL);

	// first thing first, alloc special nodes
	{
		new_node(NODE_NIL, NODE_FLAG_LITERAL|NODE_FLAG_PRERESOLVE);
		for(int i = 0; i <= 256; i++) {
			node_idx_t nidx = new_node(NODE_INT, NODE_FLAG_LITERAL|NODE_FLAG_PRERESOLVE);
			assert(nidx == INT_0_NODE+i);
			node_t *n = get_node(nidx);
			n->t_int = i;
		}
		{
			node_idx_t i = new_node(NODE_BOOL, NODE_FLAG_LITERAL|NODE_FLAG_PRERESOLVE);
			node_t *n = get_node(i);
			n->t_bool = false;
		}
		{
			node_idx_t i = new_node(NODE_BOOL, NODE_FLAG_LITERAL|NODE_FLAG_PRERESOLVE);
			node_t *n = get_node(i);
			n->t_bool = true;
		}
		new_node_symbol("quote", NODE_FLAG_PRERESOLVE);
		new_node_symbol("unquote", NODE_FLAG_PRERESOLVE);
		new_node_symbol("unquote-splice", NODE_FLAG_PRERESOLVE);
		new_node_symbol("quasiquote", NODE_FLAG_PRERESOLVE);
		new_node_list(new_list(), NODE_FLAG_PRERESOLVE);
		new_node_vector(new_vector(), NODE_FLAG_PRERESOLVE);
		new_node_map(new_hash_map(), NODE_FLAG_PRERESOLVE);
		new_node_hash_set(new_hash_set(), NODE_FLAG_PRERESOLVE);
		new_node_symbol("%", NODE_FLAG_PRERESOLVE);
		new_node_symbol("%1", NODE_FLAG_PRERESOLVE);
		new_node_symbol("%2", NODE_FLAG_PRERESOLVE);
		new_node_symbol("%3", NODE_FLAG_PRERESOLVE);
		new_node_symbol("%4", NODE_FLAG_PRERESOLVE);
		new_node_symbol("%5", NODE_FLAG_PRERESOLVE);
		new_node_symbol("%6", NODE_FLAG_PRERESOLVE);
		new_node_symbol("%7", NODE_FLAG_PRERESOLVE);
		new_node_symbol("%8", NODE_FLAG_PRERESOLVE);
		new_node_keyword("else", NODE_FLAG_PRERESOLVE);
		new_node_keyword("when", NODE_FLAG_PRERESOLVE);
		new_node_keyword("while", NODE_FLAG_PRERESOLVE);
		new_node_keyword("let", NODE_FLAG_PRERESOLVE);
		new_node_keyword("__PC__", NODE_FLAG_PRERESOLVE);
		new_node_keyword("__ALL__", NODE_FLAG_PRERESOLVE);
		new_node_keyword("__BY__", NODE_FLAG_PRERESOLVE);
		new_node_keyword("__AUTO_DEREF__", NODE_FLAG_PRERESOLVE);
	}

	env->set("nil", NIL_NODE);
	env->set("zero", ZERO_NODE);
	env->set("false", FALSE_NODE);
	env->set("true", TRUE_NODE);
	env->set("quote", new_node_native_function("quote", &native_quote, true, NODE_FLAG_PRERESOLVE));
	env->set("unquote", UNQUOTE_NODE);
	env->set("unquote-splice", UNQUOTE_SPLICE_NODE);
	env->set("quasiquote", new_node_native_function("quasiquote", &native_quasiquote, true, NODE_FLAG_PRERESOLVE));
	env->set("let", new_node_native_function("let", &native_let, true, NODE_FLAG_PRERESOLVE));
	env->set("eval", new_node_native_function("eval", &native_eval, false, NODE_FLAG_PRERESOLVE));
	env->set("print", new_node_native_function("print", &native_print, false, NODE_FLAG_PRERESOLVE));
	env->set("println", new_node_native_function("println", &native_println, false, NODE_FLAG_PRERESOLVE));
	env->set("print-str", new_node_native_function("print-str", &native_print_str, false, NODE_FLAG_PRERESOLVE));
	env->set("println-str", new_node_native_function("println-str", &native_println_str, false, NODE_FLAG_PRERESOLVE));
	env->set("=", new_node_native_function("=", &native_eq, false, NODE_FLAG_PRERESOLVE));
	env->set("not=", new_node_native_function("not=", &native_neq, false, NODE_FLAG_PRERESOLVE));
	env->set("<", new_node_native_function("<", &native_lt, false, NODE_FLAG_PRERESOLVE));
	env->set("<=", new_node_native_function("<=", &native_lte, false, NODE_FLAG_PRERESOLVE));
	env->set(">", new_node_native_function(">", &native_gt, false, NODE_FLAG_PRERESOLVE));
	env->set(">=", new_node_native_function(">=", &native_gte, false, NODE_FLAG_PRERESOLVE));
	env->set("and", new_node_native_function("and", &native_and, true, NODE_FLAG_PRERESOLVE));
	env->set("or", new_node_native_function("or", &native_or, true, NODE_FLAG_PRERESOLVE));
	env->set("not", new_node_native_function("not", &native_not, false, NODE_FLAG_PRERESOLVE));
	env->set("empty?", new_node_native_function("empty?", &native_is_empty, false, NODE_FLAG_PRERESOLVE));
	env->set("not-empty?", new_node_native_function("not-empty?", &native_is_not_empty, false, NODE_FLAG_PRERESOLVE));
	env->set("zero?", new_node_native_function("zero?", &native_is_zero, false, NODE_FLAG_PRERESOLVE));
	env->set("false?", new_node_native_function("false?", &native_is_false, false, NODE_FLAG_PRERESOLVE));
	env->set("true?", new_node_native_function("true?", &native_is_true, false, NODE_FLAG_PRERESOLVE));
	env->set("some?", new_node_native_function("some?", &native_is_some, false, NODE_FLAG_PRERESOLVE));
	env->set("letter?", new_node_native_function("letter?", &native_is_letter, false, NODE_FLAG_PRERESOLVE));
	env->set("do", new_node_native_function("do", &native_do, false, NODE_FLAG_PRERESOLVE));
	env->set("doall", new_node_native_function("doall", &native_doall, true, NODE_FLAG_PRERESOLVE));
	env->set("doall-vec", new_node_native_function("doall-vec", &native_doall_vec, true, NODE_FLAG_PRERESOLVE));
	env->set("dorun", new_node_native_function("dorun", &native_dorun, true, NODE_FLAG_PRERESOLVE));
	env->set("conj", new_node_native_function("conj", &native_conj, true, NODE_FLAG_PRERESOLVE));
	env->set("into", new_node_native_function("info", &native_into, false, NODE_FLAG_PRERESOLVE));
	env->set("pop", new_node_native_function("pop", &native_pop, false, NODE_FLAG_PRERESOLVE));
	env->set("peek", new_node_native_function("peek", &native_peek, false, NODE_FLAG_PRERESOLVE));
	env->set("first", new_node_native_function("first", &native_first, false, NODE_FLAG_PRERESOLVE));
	env->set("second", new_node_native_function("second", &native_second, false, NODE_FLAG_PRERESOLVE));
	env->set("last", new_node_native_function("last", &native_last, false, NODE_FLAG_PRERESOLVE));
	env->set("nth", new_node_native_function("nth", &native_nth, false, NODE_FLAG_PRERESOLVE));
	env->set("rand-nth", new_node_native_function("rand-nth", &native_rand_nth, false, NODE_FLAG_PRERESOLVE));
	env->set("ffirst", new_node_native_function("ffirst", &native_ffirst, false, NODE_FLAG_PRERESOLVE));
	env->set("next", new_node_native_function("next", &native_next, false, NODE_FLAG_PRERESOLVE));
	env->set("nnext", new_node_native_function("nnext", &native_nnext, false, NODE_FLAG_PRERESOLVE));
	env->set("fnext", new_node_native_function("fnext", &native_fnext, false, NODE_FLAG_PRERESOLVE));
	env->set("nfirst", new_node_native_function("nfirst", &native_nfirst, false, NODE_FLAG_PRERESOLVE));
	env->set("rest", new_node_native_function("rest", &native_rest, false, NODE_FLAG_PRERESOLVE));
	env->set("list", new_node_native_function("list", &native_list, false, NODE_FLAG_PRERESOLVE));
	env->set("list?", new_node_native_function("list?", &native_is_list, false, NODE_FLAG_PRERESOLVE));
	env->set("hash-map", new_node_native_function("hash-map", &native_hash_map, false, NODE_FLAG_PRERESOLVE));
	env->set("upper-case", new_node_native_function("upper-case", &native_upper_case, false, NODE_FLAG_PRERESOLVE));
	env->set("var", new_node_native_function("var", &native_var, false, NODE_FLAG_PRERESOLVE));
	env->set("declare", new_node_native_function("declare", &native_declare, false, NODE_FLAG_PRERESOLVE));
	env->set("def", new_node_native_function("def", &native_def, true, NODE_FLAG_PRERESOLVE));
	env->set("defonce", new_node_native_function("defonce", &native_defonce, true, NODE_FLAG_PRERESOLVE));
	env->set("fn", new_node_native_function("fn", &native_fn, true, NODE_FLAG_PRERESOLVE));
	env->set("fn?", new_node_native_function("fn?", &native_is_fn, false, NODE_FLAG_PRERESOLVE));
	env->set("ifn?", new_node_native_function("ifn?", &native_is_ifn, false, NODE_FLAG_PRERESOLVE));
	env->set("ident?", new_node_native_function("ident?", &native_is_ident, false, NODE_FLAG_PRERESOLVE));
	env->set("simple-ident?", new_node_native_function("simple-ident?", &native_is_simple_ident, false, NODE_FLAG_PRERESOLVE));
	env->set("qualified-ident?", new_node_native_function("qualified-ident?", &native_is_qualified_ident, false, NODE_FLAG_PRERESOLVE));
	env->set("symbol?", new_node_native_function("symbol?", &native_is_symbol, false, NODE_FLAG_PRERESOLVE));
	env->set("simple-symbol?", new_node_native_function("simple-symbol?", &native_is_simple_symbol, false, NODE_FLAG_PRERESOLVE));
	env->set("qualified-symbol?", new_node_native_function("qualified-symbol?", &native_is_qualified_symbol, false, NODE_FLAG_PRERESOLVE));
	env->set("keyword?", new_node_native_function("keyword?", &native_is_keyword, false, NODE_FLAG_PRERESOLVE));
	env->set("simple-keyword?", new_node_native_function("simple-keyword?", &native_is_simple_keyword, false, NODE_FLAG_PRERESOLVE));
	env->set("qualified-keyword?", new_node_native_function("qualified-keyword?", &native_is_qualified_keyword, false, NODE_FLAG_PRERESOLVE));
	env->set("indexed?", new_node_native_function("indexed?", &native_is_indexed, false, NODE_FLAG_PRERESOLVE));
	env->set("defn", new_node_native_function("defn", &native_defn, true, NODE_FLAG_PRERESOLVE));
	env->set("defmacro", new_node_native_function("defmacro", &native_defmacro, true, NODE_FLAG_PRERESOLVE));
	env->set("*ns*", new_node_var("nil", NIL_NODE, NODE_FLAG_PRERESOLVE));
	env->set("if", new_node_native_function("if", &native_if, true, NODE_FLAG_PRERESOLVE));
	env->set("if-not", new_node_native_function("if-not", &native_if_not, true, NODE_FLAG_PRERESOLVE));
	env->set("if-let", new_node_native_function("if-let", &native_if_let, true, NODE_FLAG_PRERESOLVE));
	env->set("if-some", new_node_native_function("if-some", &native_if_some, true, NODE_FLAG_PRERESOLVE));
	env->set("when", new_node_native_function("when", &native_when, true, NODE_FLAG_PRERESOLVE));
	env->set("when-let", new_node_native_function("when-let", &native_when_let, true, NODE_FLAG_PRERESOLVE));
	env->set("when-not", new_node_native_function("when-not", &native_when_not, true, NODE_FLAG_PRERESOLVE));
	env->set("while", new_node_native_function("while", &native_while, true, NODE_FLAG_PRERESOLVE));
	env->set("while-not", new_node_native_function("while-not", &native_while_not, true, NODE_FLAG_PRERESOLVE));
	env->set("cond", new_node_native_function("cond", &native_cond, true, NODE_FLAG_PRERESOLVE));
	env->set("condp", new_node_native_function("condp", &native_condp, true, NODE_FLAG_PRERESOLVE));
	env->set("case", new_node_native_function("case", &native_case, true, NODE_FLAG_PRERESOLVE));
	env->set("apply", new_node_native_function("apply", &native_apply, true, NODE_FLAG_PRERESOLVE));
	env->set("reduce", new_node_native_function("reduce", &native_reduce, true, NODE_FLAG_PRERESOLVE));
	env->set("reduced", new_node_native_function("reduced", &native_reduced, false, NODE_FLAG_PRERESOLVE));
	env->set("ensure-reduced", new_node_native_function("ensure-reduced", &native_ensure_reduced, false, NODE_FLAG_PRERESOLVE));
	env->set("unreduced", new_node_native_function("unreduced", &native_unreduced, false, NODE_FLAG_PRERESOLVE));
	env->set("reduced?", new_node_native_function("reduced?", &native_is_reduced, false, NODE_FLAG_PRERESOLVE));
	env->set("delay", new_node_native_function("delay", &native_delay, true, NODE_FLAG_PRERESOLVE));
	env->set("delay?", new_node_native_function("delay?", &native_is_delay, false, NODE_FLAG_PRERESOLVE));
	env->set("constantly", new_node_native_function("constantly", &native_constantly, false, NODE_FLAG_PRERESOLVE));
	env->set("count", new_node_native_function("count", &native_count, false, NODE_FLAG_PRERESOLVE));
	env->set("dotimes", new_node_native_function("dotimes", &native_dotimes, true, NODE_FLAG_PRERESOLVE));
	env->set("doseq", new_node_native_function("doseq", &native_doseq, true, NODE_FLAG_PRERESOLVE));
	env->set("nil?", new_node_native_function("nil?", native_is_nil, false, NODE_FLAG_PRERESOLVE));
	env->set("not-nil?", new_node_native_function("not-nil?", native_is_not_nil, false, NODE_FLAG_PRERESOLVE));
	env->set("time", new_node_native_function("time", &native_time, true, NODE_FLAG_PRERESOLVE));
	env->set("assoc", new_node_native_function("assoc", &native_assoc, false, NODE_FLAG_PRERESOLVE));
	env->set("assoc-in", new_node_native_function("assoc-in", &native_assoc_in, false, NODE_FLAG_PRERESOLVE));
	env->set("dissoc", new_node_native_function("dissoc", &native_dissoc, false, NODE_FLAG_PRERESOLVE));
	env->set("disj", new_node_native_function("disj", &native_disj, false, NODE_FLAG_PRERESOLVE));
	env->set("update", new_node_native_function("update", &native_update, false, NODE_FLAG_PRERESOLVE));
	env->set("update-in", new_node_native_function("update-in", &native_update_in, false, NODE_FLAG_PRERESOLVE));
	env->set("get", new_node_native_function("get", &native_get, false, NODE_FLAG_PRERESOLVE));
	env->set("comp", new_node_native_function("comp", &native_comp, false, NODE_FLAG_PRERESOLVE));
	env->set("partial", new_node_native_function("partial", &native_partial, false, NODE_FLAG_PRERESOLVE));
	env->set("shuffle", new_node_native_function("shuffle", &native_shuffle, false, NODE_FLAG_PRERESOLVE));
	env->set("random-sample", new_node_native_function("random-sample", &native_random_sample, false, NODE_FLAG_PRERESOLVE));
	env->set("is", new_node_native_function("is", &native_is, true, NODE_FLAG_PRERESOLVE));
	env->set("assert", new_node_native_function("assert", &native_is, true, NODE_FLAG_PRERESOLVE));
	env->set("identity", new_node_native_function("identity", &native_identity, false, NODE_FLAG_PRERESOLVE));
	env->set("reverse", new_node_native_function("reverse", &native_reverse, false, NODE_FLAG_PRERESOLVE));
	env->set("nthrest", new_node_native_function("nthrest", &native_nthrest, false, NODE_FLAG_PRERESOLVE));
	env->set("nthnext", new_node_native_function("nthnext", &native_nthnext, false, NODE_FLAG_PRERESOLVE));
	env->set("->", new_node_native_function("->", &native_thread, true, NODE_FLAG_PRERESOLVE));
	env->set("->>", new_node_native_function("->>", &native_thread_last, true, NODE_FLAG_PRERESOLVE));
	env->set("as->", new_node_native_function("as->", &native_as_thread, true, NODE_FLAG_PRERESOLVE));
	env->set("cond->", new_node_native_function("cond->", &native_cond_thread, true, NODE_FLAG_PRERESOLVE));
	env->set("cond->>", new_node_native_function("cond->>", &native_cond_thread_last, true, NODE_FLAG_PRERESOLVE));
	env->set("every?", new_node_native_function("every?", &native_is_every, false, NODE_FLAG_PRERESOLVE));
	env->set("not-every?", new_node_native_function("not-every?", &native_is_not_every, false, NODE_FLAG_PRERESOLVE));
	env->set("any?", new_node_native_function("any?", &native_is_any, false, NODE_FLAG_PRERESOLVE));
	env->set("not-any?", new_node_native_function("not-any?", &native_is_not_any, false, NODE_FLAG_PRERESOLVE));
	env->set("array-map", new_node_native_function("array-map", &native_array_map, false, NODE_FLAG_PRERESOLVE));
	env->set("boolean", new_node_native_function("boolean", &native_boolean, false, NODE_FLAG_PRERESOLVE));
	env->set("boolean?", new_node_native_function("boolean?", &native_is_boolean, false, NODE_FLAG_PRERESOLVE));
	env->set("butlast", new_node_native_function("butlast", &native_butlast, false, NODE_FLAG_PRERESOLVE));
	env->set("complement", new_node_native_function("complement", &native_complement, false, NODE_FLAG_PRERESOLVE));
	env->set("contains?", new_node_native_function("contains?", &native_is_contains, false, NODE_FLAG_PRERESOLVE));
	env->set("counted?", new_node_native_function("counted?", &native_is_counted, false, NODE_FLAG_PRERESOLVE));
	env->set("distinct?", new_node_native_function("distinct?", &native_is_distinct, false, NODE_FLAG_PRERESOLVE));
	env->set("empty", new_node_native_function("empty", &native_empty, false, NODE_FLAG_PRERESOLVE));
	env->set("not-empty", new_node_native_function("not-empty", &native_not_empty, false, NODE_FLAG_PRERESOLVE));
	env->set("every-pred", new_node_native_function("every-pred", &native_every_pred, false, NODE_FLAG_PRERESOLVE));
	env->set("find", new_node_native_function("find", &native_find, false, NODE_FLAG_PRERESOLVE));
	env->set("fnil", new_node_native_function("fnil", &native_fnil, false, NODE_FLAG_PRERESOLVE));
	env->set("split-at", new_node_native_function("split-at", &native_split_at, false, NODE_FLAG_PRERESOLVE));
	env->set("split-with", new_node_native_function("split-with", &native_split_with, false, NODE_FLAG_PRERESOLVE));
	env->set("map?", new_node_native_function("map?", &native_is_hash_map, false, NODE_FLAG_PRERESOLVE));
	env->set("set?", new_node_native_function("set?", &native_is_set, false, NODE_FLAG_PRERESOLVE));
	env->set("vector?", new_node_native_function("vector?", &native_is_vector, false, NODE_FLAG_PRERESOLVE));
	env->set("frequencies", new_node_native_function("frequencies", &native_frequencies, false, NODE_FLAG_PRERESOLVE));
	env->set("get-in", new_node_native_function("get-in", &native_get_in, false, NODE_FLAG_PRERESOLVE));
	env->set("group-by", new_node_native_function("group-by", &native_group_by, false, NODE_FLAG_PRERESOLVE));
	env->set("hash", new_node_native_function("hash", &native_hash, false, NODE_FLAG_PRERESOLVE));
	env->set("hash-ordered-coll", new_node_native_function("hash-ordered-coll", &native_hash, false, NODE_FLAG_PRERESOLVE));
	env->set("hash-unordered-coll", new_node_native_function("hash-unordered-coll", &native_hash, false, NODE_FLAG_PRERESOLVE));
	env->set("hash-combine", new_node_native_function("hash-combine", &native_hash_combine, false, NODE_FLAG_PRERESOLVE));
	env->set("hash-set", new_node_native_function("hash-set", &native_hash_set, false, NODE_FLAG_PRERESOLVE));
	env->set("set", new_node_native_function("set", &native_hash_set, false, NODE_FLAG_PRERESOLVE));
	env->set("identical?", new_node_native_function("identical?", &native_is_identical, false, NODE_FLAG_PRERESOLVE));
	env->set("juxt", new_node_native_function("juxt", &native_juxt, false, NODE_FLAG_PRERESOLVE));
	env->set("name", new_node_native_function("name", &native_name, false, NODE_FLAG_PRERESOLVE));
	env->set("keyword", new_node_native_function("keyword", &native_keyword, false, NODE_FLAG_PRERESOLVE));
	env->set("letfn", new_node_native_function("letfn", &native_letfn, true, NODE_FLAG_PRERESOLVE));
	env->set("load-file", new_node_native_function("load-file", &native_load_file, false, NODE_FLAG_PRERESOLVE));
	env->set("load-reader", new_node_native_function("load-reader", &native_load_reader, false, NODE_FLAG_PRERESOLVE));
	env->set("load-string", new_node_native_function("load-string", &native_load_string, false, NODE_FLAG_PRERESOLVE));
	env->set("read-string", new_node_native_function("read-string", &native_read_string, false, NODE_FLAG_PRERESOLVE));
	env->set("loop", new_node_native_function("loop", &native_loop, true, NODE_FLAG_PRERESOLVE));
	env->set("recur", new_node_native_function("recur", &native_recur, false, NODE_FLAG_PRERESOLVE));
	env->set("mapv", new_node_native_function("mapv", &native_mapv, false, NODE_FLAG_PRERESOLVE));
	env->set("vector", new_node_native_function("vector", &native_vector, false, NODE_FLAG_PRERESOLVE));
	env->set("vector2d", new_node_native_function("vector2d", &native_vector2d, false, NODE_FLAG_PRERESOLVE));
	env->set("max-key", new_node_native_function("max-key", &native_max_key, false, NODE_FLAG_PRERESOLVE));
	env->set("min-key", new_node_native_function("min-key", &native_min_key, false, NODE_FLAG_PRERESOLVE));
	env->set("key", new_node_native_function("key", &native_key, false, NODE_FLAG_PRERESOLVE));
	env->set("val", new_node_native_function("val", &native_val, false, NODE_FLAG_PRERESOLVE));
	env->set("merge", new_node_native_function("merge", &native_merge, false, NODE_FLAG_PRERESOLVE));
	env->set("merge-with", new_node_native_function("merge-with", &native_merge_with, false, NODE_FLAG_PRERESOLVE));
	env->set("namespace", new_node_native_function("namespace", &native_namespace, false, NODE_FLAG_PRERESOLVE));
	env->set("newline", new_node_native_function("newline", &native_newline, false, NODE_FLAG_PRERESOLVE));
	env->set("reduce-kv", new_node_native_function("reduce-kv", &native_reduce_kv, false, NODE_FLAG_PRERESOLVE));
	env->set("replace", new_node_native_function("replace", &native_replace, false, NODE_FLAG_PRERESOLVE));
	env->set("reversible?", new_node_native_function("reversible?", &native_is_reversible, false, NODE_FLAG_PRERESOLVE));
	env->set("seqable?", new_node_native_function("seqable?", &native_is_seqable, false, NODE_FLAG_PRERESOLVE));
	env->set("vec", new_node_native_function("vec", &native_vec, false, NODE_FLAG_PRERESOLVE));

	jo_clojure_math_init(env);
	jo_clojure_string_init(env);
	jo_clojure_system_init(env);
	jo_clojure_lazy_init(env);
	jo_clojure_io_init(env);
	jo_clojure_async_init(env);
	jo_clojure_gif_init(env);
	
	FILE *fp = fopen(argv[1], "r");
	if(!fp) {
		return 0;
	}
	
	debugf("Parsing...\n");

	parse_state_t parse_state;
	parse_state.fp = fp;

	// parse the base list
	list_ptr_t main_list = new_list();
	for(node_idx_t next = parse_next(env, &parse_state, 0); next != INV_NODE; next = parse_next(env, &parse_state, 0)) {
		main_list->push_back_inplace(next);
	}
	fclose(fp);

	debugf("Evaluating...\n");

	// start the GC
	std::thread gc_thread[num_garbage_sectors];
	
	for(int i = 0; i < num_garbage_sectors; i++) {
		gc_thread[i] = std::thread(collect_garbage, i);
	}

	{
		node_idx_t res_idx = eval_node_list(env, main_list);
		//native_println(env, list_va(res_idx));
		printf("%s\n", get_node(res_idx)->as_string(3).c_str());
	}

	debugf("Waiting for GC to stop...\n");
#if 0
	garbage_nodes.close();
	gc_thread.join();
#else
	for(int i = 0; i < num_garbage_sectors; ++i) {
		garbage_nodes[i].close();
	}
	for(int i = 0; i < num_garbage_sectors; ++i) {
		gc_thread[i].join();
	}
#endif

	debugf("nodes.size() = %zu\n", nodes.size());
	for(int i = 0; i < num_free_sectors; ++i) {
		debugf("free_nodes.size() = %zu\n", free_nodes[i].size());
	}
	debugf("atom_retries = %zu\n", atom_retries.load());
	debugf("stm_retries = %zu\n", stm_retries.load());

	/*
	for(int i = -20; i <= 20; i++) {
		for(int j = -20; j <= 20; j++) {
			jo_bigint big_i = i;
			jo_bigint big_j = j;
			if((big_i + big_j).to_int() != i + j) {
				printf("FAILED: %d + %d was %d, should be %d\n", i, j, (big_i + big_j).to_int(), i + j);
				debug_break();
				printf("FAILED: %d + %d was %d, should be %d\n", i, j, (big_i + big_j).to_int(), i + j);
			}
			if((big_i - big_j).to_int() != i - j) {
				printf("FAILED: %d - %d was %d, should be %d\n", i, j, (big_i - big_j).to_int(), i - j);
				debug_break();
				printf("FAILED: %d - %d was %d, should be %d\n", i, j, (big_i - big_j).to_int(), i - j);
			}
		}
	}
	*/
	//jo_float f("1.23456789");
	//jo_string f_str = f.to_string();
	//printf("f = %s\n", f_str.c_str());

#ifdef WITH_TELEMETRY
	tmClose(0);
	tmShutdown();
	free(telemetry_memory);
#endif
}
