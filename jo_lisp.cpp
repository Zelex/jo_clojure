// TODO: arbitrary precision numbers... really? do I need to support this?
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include "debugbreak.h"
#include "jo_stdcpp.h"

//#define debugf printf
#define debugf sizeof

#define warnf printf
//#define warnf sizeof


enum {
	// hard coded nodes
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
	NODE_SET,
	NODE_MAP,
	NODE_NATIVE_FUNC,
	NODE_FUNC,
	NODE_VAR,
	NODE_DELAY,

	// node flags
	NODE_FLAG_MACRO        = 1<<0,
	NODE_FLAG_STRING       = 1<<1, // string or symbol or keyword
	NODE_FLAG_LAZY         = 1<<2, // unused
	NODE_FLAG_LITERAL      = 1<<3,
	NODE_FLAG_LITERAL_ARGS = 1<<4,
};

struct env_t;
struct node_t;
struct lazy_list_iterator_t;

struct node_idx_t {
	int idx;
	node_idx_t() = default;
	node_idx_t(int idx) : idx(idx) {}
	operator int() const { return idx; }
	node_idx_t& operator=(int idx) { this->idx = idx; return *this; }
	bool operator==(const node_idx_t& other) const { return idx == other.idx; }
	bool operator!=(const node_idx_t& other) const { return idx != other.idx; }
	bool operator==(int idx) const { return this->idx == idx; }
	bool operator!=(int idx) const { return this->idx != idx; }
};

typedef jo_persistent_list<node_idx_t> list_t;
//typedef jo_persistent_vector_bidirectional<node_idx_t> list_t;
typedef jo_shared_ptr<list_t> list_ptr_t;

typedef jo_persistent_vector<node_idx_t> vector_t;
typedef jo_shared_ptr<vector_t> vector_ptr_t;

typedef jo_persistent_unordered_map<node_idx_t, node_idx_t> map_t;
typedef jo_shared_ptr<map_t> map_ptr_t;

typedef jo_shared_ptr<env_t> env_ptr_t;

typedef std::function<node_idx_t(env_ptr_t, list_ptr_t)> native_func_t;
typedef jo_shared_ptr<native_func_t> native_func_ptr_t;

typedef node_idx_t (*native_function_t)(env_ptr_t env, list_ptr_t args);

static list_ptr_t new_list() { return list_ptr_t(new list_t()); }
static list_ptr_t new_list(node_idx_t v) { list_t *l = new list_t(); l->push_back_inplace(v); return list_ptr_t(l); }
static vector_ptr_t new_vector() { return vector_ptr_t(new vector_t()); }
static map_ptr_t new_map() { return map_ptr_t(new map_t()); }

static inline node_t *get_node(node_idx_t idx);
static inline int get_node_type(node_idx_t idx);
static inline int get_node_type(const node_t *n);
static inline int get_node_flags(node_idx_t idx);
static inline jo_string get_node_string(node_idx_t idx);
static inline node_idx_t get_node_var(node_idx_t idx);
static inline bool get_node_bool(node_idx_t idx);
static inline list_ptr_t get_node_list(node_idx_t idx);
static inline vector_ptr_t get_node_vector(node_idx_t idx);
static inline map_ptr_t get_node_map(node_idx_t idx);
static inline int get_node_int(node_idx_t idx);
static inline float get_node_float(node_idx_t idx);
static inline vector_ptr_t get_node_func_args(node_idx_t idx);
static inline list_ptr_t get_node_func_body(node_idx_t idx);
static inline node_idx_t get_node_lazy_fn(node_idx_t idx);
static inline node_idx_t get_node_lazy_fn(const node_t *n);

static node_idx_t new_node_list(list_ptr_t nodes, int flags = 0);
static node_idx_t new_node_string(const jo_string &s);
static node_idx_t new_node_map(map_ptr_t nodes, int flags = 0);
static node_idx_t new_node_vector(vector_ptr_t nodes, int flags = 0);
static node_idx_t new_node_lazy_list(node_idx_t lazy_fn);
static node_idx_t new_node_var(const jo_string &name, node_idx_t value);

static bool node_eq(env_ptr_t env, node_idx_t n1i, node_idx_t n2i);
static bool node_lt(env_ptr_t env, node_idx_t n1i, node_idx_t n2i);
static bool node_lte(env_ptr_t env, node_idx_t n1i, node_idx_t n2i);

static node_idx_t eval_node(env_ptr_t env, node_idx_t root);
static node_idx_t eval_node_list(env_ptr_t env, list_ptr_t list);
static node_idx_t eval_list(env_ptr_t env, list_ptr_t list, int list_flags=0);
static node_idx_t eval_va(env_ptr_t env, int num, ...);

static void print_node(node_idx_t node, int depth = 0, bool same_line=false);
static void print_node_type(node_idx_t node);
static void print_node_list(list_ptr_t nodes, int depth = 0);
static void print_node_vector(vector_ptr_t nodes, int depth = 0);
static void print_node_map(map_ptr_t nodes, int depth = 0);


struct env_t {
	// for iterating them all, otherwise unused.
	list_ptr_t vars;
	// TODO: need persistent version of vars_map
	struct fast_val_t {
		node_idx_t var;
		node_idx_t value;
		fast_val_t() : var(NIL_NODE), value(NIL_NODE) {}
		fast_val_t(node_idx_t var, node_idx_t value) : var(var), value(value) {}
	};
	std::unordered_map<std::string, fast_val_t> vars_map;
	env_ptr_t parent;

	env_t(env_ptr_t p) : vars(new_list()), vars_map(), parent(p) {}

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

	node_idx_t get(const jo_string &name) const {
		return find(name.c_str()).value;
	}

	bool has(const jo_string &name) const {
		return find(name).var != NIL_NODE;
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
		node_idx_t idx = new_node_var(name, value);
		vars = vars->push_front(idx);
		vars_map[name.c_str()] = fast_val_t(idx, value);
	}

	// sets the map only, but cannot iterate it. for dotimes and stuffs.
	void set_temp(const jo_string &name, node_idx_t value) {
		vars_map[name.c_str()] = fast_val_t(NIL_NODE, value);
	}

	template<typename T1, typename T2>
	void set_temp(T1 key_list, T2 value_list) {
		auto k = key_list->begin();
		auto v = value_list->begin();
		for(; k && v; k++,v++) {
			node_idx_t key_idx = *k;
			node_idx_t value_idx = *v;
			if(get_node_type(key_idx) == NODE_LIST && get_node_type(value_idx) == NODE_LIST) {
				set_temp(get_node_list(key_idx), get_node_list(value_idx));
			} else {
				set_temp(get_node_string(key_idx), value_idx);
			}
		}
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

static env_ptr_t new_env(env_ptr_t parent) { return env_ptr_t(new env_t(parent)); }

struct lazy_list_iterator_t {
	env_ptr_t env;
	node_idx_t cur;
	node_idx_t val;
	jo_vector<node_idx_t> next_list;
	int next_idx;

	lazy_list_iterator_t(env_ptr_t env, const node_t *node) : env(env), cur(), val(NIL_NODE), next_list(), next_idx() {
		if(get_node_type(node) == NODE_LAZY_LIST) {
			cur = eval_node(env, get_node_lazy_fn(node));
			if(!done()) {
				val = get_node_list(cur)->first_value();
			}
		}
	}

	lazy_list_iterator_t(env_ptr_t env, node_idx_t node_idx) : env(env), cur(node_idx), val(NIL_NODE), next_list(), next_idx() {
		if(get_node_type(cur) == NODE_LAZY_LIST) {
			cur = eval_node(env, get_node_lazy_fn(cur));
			if(!done()) {
				val = get_node_list(cur)->first_value();
			}
		}
	}

	bool done() const {
		return next_idx >= next_list.size() && !get_node_list(cur);
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

	node_idx_t next_fn(int n) {
		for(int i = 0; i < n; i++) {
			next();
		}
		if(done()) {
			return NIL_NODE;
		}
		return new_node_list(get_node_list(cur)->rest());
	}

	node_idx_t nth(int n) {
		node_idx_t res = val;
		while(n-- > 0 && !done()) {
			next();
		}
		return val;
	}

	operator bool() const {
		return !done();
	}

	list_ptr_t all(int n = INT_MAX) {
		list_ptr_t res = new_list();
		while(!done() && n-- > 0) {
			res->push_back_inplace(val);
			next();
		}
		return res;
	}

	// fetch the next N values, and put them into next_list
	void prefetch(int n) {
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
	int type;
	int flags;
	jo_string t_string;
	list_ptr_t t_list;
	vector_ptr_t t_vector;
	map_ptr_t t_map;
	native_func_ptr_t t_native_function;
	struct {
		vector_ptr_t args;
		list_ptr_t body;
		env_ptr_t env;
	} t_func;
	union {
		node_idx_t t_var; // link to the variable
		bool t_bool;
		// most implementations combine these as "number", but at the moment that sounds silly
		int t_int;
		double t_float;
		node_idx_t t_delay; // cached result
		node_idx_t t_lazy_fn;
	};

	node_t() : type(), flags(), t_string(), t_list(), t_vector(), t_map(), t_native_function(), t_func(), t_int() {}

	bool is_symbol() const { return type == NODE_SYMBOL; }
	bool is_keyword() const { return type == NODE_KEYWORD; }
	bool is_list() const { return type == NODE_LIST; }
	bool is_vector() const { return type == NODE_VECTOR; }
	bool is_map() const { return type == NODE_MAP; }
	bool is_lazy_list() const { return type == NODE_LAZY_LIST; }
	bool is_string() const { return type == NODE_STRING; }
	bool is_func() const { return type == NODE_FUNC; }
	bool is_native_func() const { return type == NODE_NATIVE_FUNC; }
	bool is_macro() const { return flags & NODE_FLAG_MACRO;}
	bool is_float() const { return type == NODE_FLOAT; }
	bool is_int() const { return type == NODE_INT; }

	bool is_seq() const { return is_list() || is_lazy_list() || is_map() || is_vector(); }
	bool can_eval() const { return is_symbol() || is_keyword() || is_list() || is_func() || is_native_func(); }

	node_idx_t seq_first(env_ptr_t env) const {
		if(is_list()) {
			return t_list->first_value();
		} else if(is_vector()) {
			return t_vector->first_value();
		} else if(is_map()) {
			return t_map->first_value();
		} else if(is_lazy_list()) {
			lazy_list_iterator_t lit(env, this);
			return lit.val;
		}
		return NIL_NODE;
	}

	node_idx_t seq_rest(env_ptr_t env) const {
		if(is_list()) {
			return new_node_list(t_list->rest());
		} else if(is_vector()) {
			return new_node_vector(t_vector->rest());
		} else if(is_map()) {
			return new_node_map(t_map->rest());
		} else if(is_lazy_list()) {
			lazy_list_iterator_t lit(env, this);
			return new_node_lazy_list(lit.next_fn());
		}
		return NIL_NODE;
	}

	jo_pair<node_idx_t, node_idx_t> seq_first_rest(env_ptr_t env) const {
		if(is_list()) {
			return jo_pair<node_idx_t, node_idx_t>(t_list->first_value(), new_node_list(t_list->rest()));
		} else if(is_vector()) {
			return jo_pair<node_idx_t, node_idx_t>(t_vector->first_value(), new_node_vector(t_vector->rest()));
		} else if(is_map()) {
			return jo_pair<node_idx_t, node_idx_t>(t_map->first_value(), new_node_map(t_map->rest()));
		} else if(is_lazy_list()) {
			lazy_list_iterator_t lit(env, this);
			return jo_pair<node_idx_t, node_idx_t>(lit.val, new_node_lazy_list(lit.next_fn()));
		}
		return jo_pair<node_idx_t, node_idx_t>(NIL_NODE, NIL_NODE);
	}

	bool seq_empty(env_ptr_t env) const {
		if(is_list()) {
			return t_list->empty();
		} else if(is_vector()) {
			return t_vector->empty();
		} else if(is_map()) {
			return t_map->size() == 0;
		} else if(is_lazy_list()) {
			lazy_list_iterator_t lit(env, this);
			return lit.done();
		}
		return true;
	}

	list_ptr_t &as_list() { return t_list; }
	vector_ptr_t &as_vector() { return t_vector; }
	map_ptr_t &as_map() { return t_map; }

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
			case NODE_SET:
			case NODE_MAP:    return true; // TODO
			default:          return false;
		}
	}

	int as_int() const {
		switch(type) {
		case NODE_BOOL:   return t_bool;
		case NODE_INT:    return t_int;
		case NODE_FLOAT:  return (int)t_float;
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

	jo_string as_string() const {
		switch(type) {
		case NODE_BOOL:   return t_bool ? "true" : "false";
		case NODE_INT:    
			// only letter? TODO
		 	if(jo_isletter(t_int)) {
				return jo_string(t_int);
			}
			return va("%i", t_int);
		case NODE_FLOAT:  return va("%f", t_float);
		}
		return t_string;
	}

	jo_string type_as_string() const {
		switch(type) {
		case NODE_BOOL:    return "bool";
		case NODE_INT:     return "int";
		case NODE_FLOAT:   return "float";
		case NODE_STRING:  return "string";
		case NODE_LIST:    return "list";
		case NODE_LAZY_LIST: return "lazy-list";
		case NODE_VECTOR:  return "vector";
		case NODE_SET:     return "set";
		case NODE_MAP:     return "map";
		case NODE_NATIVE_FUNC: return "native_function";
		case NODE_VAR:	   return "var";
		case NODE_SYMBOL:  return "symbol";
		case NODE_KEYWORD: return "keyword";
		}
		return "unknown";		
	}
};

static jo_pinned_vector<node_t> nodes;
static jo_vector<node_idx_t> free_nodes; // available for allocation...

static inline node_t *get_node(node_idx_t idx) { return &nodes[idx]; }
static inline int get_node_type(node_idx_t idx) { return get_node(idx)->type; }
static inline int get_node_type(const node_t *n) { return n->type; }
static inline int get_node_flags(node_idx_t idx) { return get_node(idx)->flags; }
static inline jo_string get_node_string(node_idx_t idx) { return get_node(idx)->as_string(); }
static inline node_idx_t get_node_var(node_idx_t idx) { return get_node(idx)->t_var; }
static inline bool get_node_bool(node_idx_t idx) { return get_node(idx)->as_bool(); }
static inline list_ptr_t get_node_list(node_idx_t idx) { return get_node(idx)->as_list(); }
static inline vector_ptr_t get_node_vector(node_idx_t idx) { return get_node(idx)->as_vector(); }
static inline map_ptr_t get_node_map(node_idx_t idx) { return get_node(idx)->as_map(); }
static inline int get_node_int(node_idx_t idx) { return get_node(idx)->as_int(); }
static inline float get_node_float(node_idx_t idx) { return get_node(idx)->as_float(); }
static inline jo_string get_node_type_string(node_idx_t idx) { return get_node(idx)->type_as_string(); }
static inline vector_ptr_t get_node_func_args(node_idx_t idx) { return get_node(idx)->t_func.args; }
static inline list_ptr_t get_node_func_body(node_idx_t idx) { return get_node(idx)->t_func.body; }
static inline node_idx_t get_node_lazy_fn(node_idx_t idx) { return get_node(idx)->t_lazy_fn; }
static inline node_idx_t get_node_lazy_fn(const node_t *n) { return n->t_lazy_fn; }

static inline node_idx_t alloc_node() {
	if (free_nodes.size()) {
		node_idx_t idx = free_nodes.back();
		free_nodes.pop_back();
		return idx;
	}
	node_idx_t idx = nodes.size();
	nodes.push_back(node_t());
	//nodes.shrink_to_fit();
	return idx;
}

static inline void free_node(node_idx_t idx) {
	free_nodes.push_back(idx);
}

// TODO: Should prefer to allocate nodes next to existing nodes which will be linked (for cache coherence)
static inline node_idx_t new_node(const node_t *n) {
	if(free_nodes.size()) {
		int ni = free_nodes.pop_back();
		nodes[ni] = *n;
		return ni;
	}
	nodes.push_back(*n);
	//nodes.shrink_to_fit();
	return nodes.size()-1;
}

static node_idx_t new_node(int type) {
	node_t n;
	n.type = type;
	return new_node(&n);
}

static node_idx_t new_node_list(list_ptr_t nodes, int flags) {
	node_idx_t idx = new_node(NODE_LIST);
	node_t *n = get_node(idx);
	n->t_list = nodes;
	n->flags |= flags;
	return idx;
}

static node_idx_t new_node_map(map_ptr_t nodes, int flags) {
	node_idx_t idx = new_node(NODE_MAP);
	node_t *n = get_node(idx);
	n->t_map = nodes;
	n->flags |= flags;
	return idx;
}

static node_idx_t new_node_vector(vector_ptr_t nodes, int flags) {
	node_idx_t idx = new_node(NODE_VECTOR);
	node_t *n = get_node(idx);
	n->t_vector = nodes;
	n->flags |= flags;
	return idx;
}

static node_idx_t new_node_lazy_list(node_idx_t lazy_fn) {
	node_idx_t idx = new_node(NODE_LAZY_LIST);
	get_node(idx)->t_lazy_fn = lazy_fn;
	get_node(idx)->flags |= NODE_FLAG_LAZY;
	return idx;
}

static node_idx_t new_node_native_function(std::function<node_idx_t(env_ptr_t,list_ptr_t)> f, bool is_macro) {
	node_t n;
	n.type = NODE_NATIVE_FUNC;
	n.t_native_function = new native_func_t(f);
	n.flags |= is_macro ? NODE_FLAG_MACRO : 0;
	n.flags |= NODE_FLAG_LITERAL;
	return new_node(&n);
}

static node_idx_t new_node_native_function(const char *name, std::function<node_idx_t(env_ptr_t,list_ptr_t)> f, bool is_macro) {
	node_t n;
	n.type = NODE_NATIVE_FUNC;
	n.t_native_function = new native_func_t(f);
	n.t_string = name;
	n.flags |= is_macro ? NODE_FLAG_MACRO : 0;
	n.flags |= NODE_FLAG_LITERAL;
	return new_node(&n);
}

static node_idx_t new_node_bool(bool b) {
	return b ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t new_node_int(int i) {
	if(i >= 0 && i <= 256) {
		return INT_0_NODE + i;
	}
	node_t n;
	n.type = NODE_INT;
	n.t_int = i;
	n.flags |= NODE_FLAG_LITERAL;
	return new_node(&n);
}

static node_idx_t new_node_float(double f) {
	node_t n;
	n.type = NODE_FLOAT;
	n.t_float = f;
	n.flags |= NODE_FLAG_LITERAL;
	return new_node(&n);
}

static node_idx_t new_node_string(const jo_string &s) {
	node_t n;
	n.type = NODE_STRING;
	n.t_string = s;
	n.flags |= NODE_FLAG_LITERAL | NODE_FLAG_STRING;
	return new_node(&n);
}

static node_idx_t new_node_symbol(const jo_string &s) {
	node_t n;
	n.type = NODE_SYMBOL;
	n.t_string = s;
	n.flags |= NODE_FLAG_STRING;
	return new_node(&n);
}

static node_idx_t new_node_keyword(const jo_string &s) {
	node_t n;
	n.type = NODE_KEYWORD;
	n.t_string = s;
	n.flags |= NODE_FLAG_LITERAL | NODE_FLAG_STRING;
	return new_node(&n);
}

static node_idx_t new_node_var(const jo_string &name, node_idx_t value) {
	node_t n;
	n.type = NODE_VAR;
	n.t_string = name;
	n.t_var = value;
	return new_node(&n);
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
	FILE *fp;
	int line_num;
	int getc() {
		int c = fgetc(fp);
		if(c == '\n') {
			line_num++;
		}
		return c;
	}
	void ungetc(int c) {
		if(c == '\n') {
			line_num--;
		}
		::ungetc(c, fp);
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
 
	if(c == '\\') {
		tok.type = TOK_SYMBOL;
		int val = state->getc();
		tok.str = va("%i", val);
		return tok;
	}
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
		// vector, list, map
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
			if(get_node(*it)->t_vector.ptr) {
				list_ptr_t sub_list = get_symbols_vector_r(get_node(*it)->t_vector);
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
	for(list_t::iterator it = list->begin(); it; it++) {
		int type = get_node_type(*it);
		if(type == NODE_SYMBOL) {
			symbol_list->push_back_inplace(*it);			
		} else if(type == NODE_MAP) {
			printf("TODO: map @ %i\n", __LINE__);
		} else if(type == NODE_VECTOR) {
			if(get_node(*it)->t_vector.ptr) {
				list_ptr_t sub_list = get_symbols_vector_r(get_node(*it)->t_vector);
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
		return new_node_string(tok.str.c_str());
	} 
	if(is_num(c) || (c == '-' && is_num(c2))) {
		// floating point
		if(tok.str.find('.') != jo_string_npos) {
			float float_val = atof(tok_ptr);
			debugf("float: %f\n", float_val);
			return new_node_float(float_val);
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
		return new_node_int(int_val);
	} 
	if(tok.type == TOK_KEYWORD) {
		debugf("keyword: %s\n", tok.str.c_str());
		if(tok.str == "else") return K_ELSE_NODE;
		if(tok.str == "when") return K_WHEN_NODE;
		if(tok.str == "while") return K_WHILE_NODE;
		if(tok.str == "let") return K_LET_NODE;
		return new_node_keyword(tok.str.c_str());
	}
	if(tok.type == TOK_SYMBOL) {
		debugf("symbol: %s\n", tok.str.c_str());
		if(env->has(tok.str.c_str())) {
			//debugf("pre-resolve symbol: %s\n", tok.str.c_str());
			return env->get(tok.str.c_str());
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
		return new_node_symbol(tok.str.c_str());
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
		n.t_list = new_list();
		n.t_list->push_back_inplace(env->get("quote"));
		while(next != INV_NODE) {
			n.t_list->push_back_inplace(next);
			next = parse_next(env, state, ')');
		}
		debugf("list end\n");
		return new_node(&n);
	}

	// parse unquote shorthand
	if(tok.type == TOK_SEPARATOR && tok.str == "unquote") {
		node_idx_t inner = parse_next(env, state, stop_on_sep);
		if(inner == INV_NODE) {
			return INV_NODE;
		}
		node_t n;
		n.type = NODE_LIST;
		n.t_list = new_list();
		n.t_list->push_back_inplace(UNQUOTE_NODE);
		n.t_list->push_back_inplace(inner);
		return new_node(&n);
	}

	// parse unquote shorthand
	if(tok.type == TOK_SEPARATOR && tok.str == "unquote-splice") {
		node_idx_t inner = parse_next(env, state, stop_on_sep);
		if(inner == INV_NODE) {
			return INV_NODE;
		}
		node_t n;
		n.type = NODE_LIST;
		n.t_list = new_list();
		n.t_list->push_back_inplace(UNQUOTE_SPLICE_NODE);
		n.t_list->push_back_inplace(inner);
		return new_node(&n);
	}

	if(tok.type == TOK_SEPARATOR && tok.str == "quasiquote") {
		node_idx_t inner = parse_next(env, state, stop_on_sep);
		if(inner == INV_NODE) {
			return INV_NODE;
		}
		node_t n;
		n.type = NODE_LIST;
		n.t_list = new_list();
		n.t_list->push_back_inplace(env->get("quasiquote"));
		n.t_list->push_back_inplace(inner);
		return new_node(&n);
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
		n.t_list = new_list();
		n.t_list->push_back_inplace(env->get("fn"));
		list_ptr_t body = new_list();
		while(next != INV_NODE) {
			body->push_back_inplace(next);
			next = parse_next(env, state, ')');
		}
		list_ptr_t symbol_list = get_symbols_list_r(body);
		vector_ptr_t arg_list = new_vector();
		int num_args_used = 0;
		for(list_t::iterator it = symbol_list->begin(); it; ++it) {
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
		return new_node(&n);
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
		return new_node(&n);
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
		n.t_vector = new_vector(); 
		int common_flags = ~0;
		while(next != INV_NODE) {
			common_flags &= get_node_flags(next);
			n.t_vector->push_back_inplace(next);
			next = parse_next(env, state, ']');
		}
		if(common_flags & NODE_FLAG_LITERAL) {
			n.flags |= NODE_FLAG_LITERAL;
		}
		// @ If its all the same type, convert to simple array of values of type
		debugf("vector end\n");
		return new_node(&n);
	}

	// parse map (hashmap)
	// like a list, but with keys and values alternating
	if(c == '{') {
		debugf("map begin\n");
		node_t n;
		n.type = NODE_MAP;
		n.t_map = new_map();
		node_idx_t next = parse_next(env, state, '}');
		node_idx_t next2 = next != INV_NODE ? parse_next(env, state, '}') : INV_NODE;
		if(next == INV_NODE || next2 == INV_NODE) {
			debugf("map end\n");
			return EMPTY_MAP_NODE;
		}
		while(next != INV_NODE && next2 != INV_NODE) {
			n.t_map->assoc_inplace(next, next2, [env](const node_idx_t &a, const node_idx_t &b) {
				return node_eq(env, a, b);
			});
			next = parse_next(env, state, '}');
			next2 = next != INV_NODE ? parse_next(env, state, '}') : INV_NODE;
		}
		debugf("map end\n");
		return new_node(&n);
	}

	// parse set

	return INV_NODE;
}


// eval a list of nodes
static node_idx_t eval_list(env_ptr_t env, list_ptr_t list, int list_flags) {
	list_t::iterator it = list->begin();
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
	) {
		node_idx_t sym_idx = n1i;
		int sym_type = n1_type;
		int sym_flags = n1_flags;
		if(n1_type == NODE_LIST) {
			sym_idx = eval_list(env, get_node(n1i)->t_list);
			sym_type = get_node_type(sym_idx);
		} else if(n1_flags & NODE_FLAG_STRING) {
			sym_idx = env->get(get_node_string(n1i));
			sym_type = get_node_type(sym_idx);
		}
		sym_flags = get_node(sym_idx)->flags;

		// get the symbol's value
		if(sym_type == NODE_NATIVE_FUNC) {
			debugf("nativefn: %s\n", get_node_string(sym_idx).c_str());
			if((sym_flags|list_flags) & (NODE_FLAG_MACRO|NODE_FLAG_LITERAL_ARGS)) {
				return (*get_node(sym_idx)->t_native_function.ptr)(env, list->rest());
			}

			list_ptr_t args = new_list();
			for(; it; it++) {
				args->push_back_inplace(eval_node(env, *it));
			}
			// call the function
			return (*get_node(sym_idx)->t_native_function.ptr)(env, args);
		} else if(sym_type == NODE_FUNC || sym_type == NODE_DELAY) {
			vector_ptr_t proto_args = get_node(sym_idx)->t_func.args;
			list_ptr_t proto_body = get_node(sym_idx)->t_func.body;
			env_ptr_t proto_env = get_node(sym_idx)->t_func.env;
			env_ptr_t fn_env = new_env(proto_env);
			list_ptr_t args1(list->rest());

			if(sym_type == NODE_DELAY && get_node(sym_idx)->t_delay != INV_NODE) {
				return get_node(sym_idx)->t_delay;
			}

			if(proto_args.ptr) {
				int is_macro = (sym_flags|list_flags) & NODE_FLAG_MACRO;
				// For each argument in arg_symbols, 
				// grab the corresponding argument in args1
				// and evaluate it
				// and create a new_node_var 
				// and insert it to the head of env
				vector_t::iterator i = proto_args->begin();
				list_t::iterator i2 = args1->begin();
				for (; i && i2; i++, i2++) {
					int i_type = get_node_type(*i);
					int i2_type = get_node_type(*i2);
					if(i_type == NODE_SYMBOL) {
						fn_env->set_temp(get_node_string(*i), is_macro ? *i2 : eval_node(env, *i2));
					} else if(i_type == NODE_VECTOR && i2_type == NODE_VECTOR) {
						for(vector_t::iterator i3 = get_node(*i)->t_vector->begin(), i4 = get_node(*i2)->t_vector->begin(); i3 && i4; i3++, i4++) {
							fn_env->set_temp(get_node_string(*i3), is_macro ? *i4 : eval_node(env, *i4));
						}
					} else if(i_type == NODE_LIST && i2_type == NODE_LIST) {
						for(list_t::iterator i3 = get_node(*i)->t_list->begin(), i4 = get_node(*i2)->t_list->begin(); i3 && i4; i3++, i4++) {
							fn_env->set_temp(get_node_string(*i3), is_macro ? *i4 : eval_node(env, *i4));
						}
					} else {
						fn_env->set_temp(get_node_string(*i), *i2);
					}
				}
			}

			// Evaluate all statements in the body list
			node_idx_t last = NIL_NODE;
			for(list_t::iterator i = proto_body->begin(); i; i++) {
				last = eval_node(fn_env, *i);
			}

			if(sym_type == NODE_DELAY) {
				get_node(sym_idx)->t_delay = last;
			}
			return last;
		} else if(sym_type == NODE_MAP) {
			// lookup the key in the map
			int n2i = eval_node(env, *it++);
			int n3i = it ? eval_node(env, *it++) : NIL_NODE;
			auto it2 = get_node(sym_idx)->t_map->find(n2i, [env](const node_idx_t &a, const node_idx_t &b) {
				return node_eq(env, a, b);
			});
			if(it2.third) {
				return it2.second;
			}
			return n3i;
		} else if(sym_type == NODE_KEYWORD) {
			// lookup the key in the map
			int n2i = eval_node(env, *it++);
			int n3i = it ? eval_node(env, *it++) : NIL_NODE;
			if(get_node_type(n2i) == NODE_MAP) {
				auto it2 = get_node(n2i)->t_map->find(sym_idx, [env](const node_idx_t &a, const node_idx_t &b) {
					return node_eq(env, a, b);
				});
				if(it2.third) {
					return it2.second;
				}
			}
			return n3i;
		}
	}
	return new_node_list(list);
}

static node_idx_t eval_node(env_ptr_t env, node_idx_t root) {
	int flags = get_node_flags(root);
	//if(flags & NODE_FLAG_LITERAL) { return root; }

	int type = get_node_type(root);
	if(type == NODE_LIST) {
		return eval_list(env, get_node(root)->t_list, flags);
	} else if(type == NODE_SYMBOL) {
		node_idx_t sym_idx = env->get(get_node_string(root));
		if(sym_idx == NIL_NODE) {
			return root;
		}
		return sym_idx;//eval_node(env, sym_idx);
	} else if(type == NODE_VECTOR) {
		if(flags & NODE_FLAG_LITERAL) { return root; }
		// resolve all symbols in the vector
		vector_ptr_t new_vec = new_vector();
		for(vector_t::iterator it = get_node(root)->t_vector->begin(); it; it++) {
			new_vec->push_back_inplace(eval_node(env, *it));
		}
		return new_node_vector(new_vec);
	} else if(type == NODE_MAP) {
		if(flags & NODE_FLAG_LITERAL) { return root; }
		// resolve all symbols in the map
		map_ptr_t newmap = new_map();
		for(map_t::iterator it = get_node(root)->t_map->begin(); it; it++) {
			newmap->assoc_inplace(eval_node(env, it->first), eval_node(env, it->second));
		}
		return new_node_map(newmap);
	}
	return root;
}

static node_idx_t eval_node_list(env_ptr_t env, list_ptr_t list) {
	node_idx_t res = NIL_NODE;
	for(list_t::iterator it = list->begin(); it; it++) {
		res = eval_node(env, *it);
	}
	return res;
}

static node_idx_t eval_va(env_ptr_t env, int num, ...) {
	va_list ap;
	va_start(ap, num);
	list_ptr_t a = new_list();
	for(int i = 0; i < num; i++) {
		a->push_back_inplace(node_idx_t(va_arg(ap, int)));
	}
	va_end(ap);
	return eval_list(env, a);
}

// Print the node heirarchy
static void print_node(node_idx_t node, int depth, bool same_line) {
#if 1
	int type = get_node_type(node);
	int flags = get_node_flags(node);
	if(type == NODE_LIST) {
		list_ptr_t list = get_node(node)->t_list;
		printf("(");
		for(list_t::iterator it = list->begin(); it; it++) {
			print_node(*it, depth+1, it);
			printf(" ");
		}
		printf(")");
	} else if(type == NODE_LAZY_LIST) {
		printf("%*s(<lazy-list> ", depth, "");
		print_node(get_node(node)->t_lazy_fn, depth + 1);
		printf("%*s)", depth, "");
	} else if(type == NODE_VECTOR) {
		vector_ptr_t vector = get_node(node)->t_vector;
		printf("[");
		for(vector_t::iterator it = vector->begin(); it; it++) {
			print_node(*it, depth+1, it);
			printf(" ");
		}
		printf("]");
	} else if(type == NODE_MAP) {
		map_ptr_t map = get_node(node)->t_map;
		if(map->size() == 0) {
			printf("{}");
			return;
		}
		printf("{");
		for(map_t::iterator it = map->begin(); it; it++) {
			print_node(it->first, depth+1, it != map->end());
			printf(" ");
			print_node(it->second, depth+1, it != map->end());
			printf(" ");
		}
		printf("}");
	} else if(type == NODE_SYMBOL) {
		printf("%s", get_node_string(node).c_str());
	} else if(type == NODE_KEYWORD) {
		printf(":%s", get_node_string(node).c_str());
	} else if(type == NODE_STRING) {
		printf("\"%s\"", get_node_string(node).c_str());
	} else if(type == NODE_NATIVE_FUNC) {
		printf("%s", get_node_string(node).c_str());
	} else if(type == NODE_FUNC) {
		print_node_vector(get_node_func_args(node), depth+1);
		print_node_list(get_node_func_body(node), depth+1);
	} else if(type == NODE_DELAY) {
		printf("<delay>");
	} else if(type == NODE_FLOAT) {
		printf("%f", get_node_float(node));
	} else if(type == NODE_INT) {
		printf("%d", get_node_int(node));
	} else if(type == NODE_BOOL) {
		printf("%s", get_node_bool(node) ? "true" : "false");
	} else if(type == NODE_NIL) {
		printf("nil");
	} else {
		printf("<<%s>>", get_node_type_string(node).c_str());
	}
#else
	node_t *n = get_node(node);
	if(n->type == NODE_LIST) {
		printf("%*s(\n", depth, "");
		print_node_list(n->t_list, depth + 1);
		printf("%*s)\n", depth, "");
	} else if(n->type == NODE_LAZY_LIST) {
		printf("%*s(lazy-list\n", depth, "");
		print_node(n->t_lazy_fn, depth + 1);
		printf("%*s)\n", depth, "");
	} else if(n->type == NODE_STRING) {
		printf("%*s\"%s\"\n", depth, "", get_node_string(node).c_str());
	} else if(n->type == NODE_SYMBOL) {
		printf("%*s%s\n", depth, "", get_node_string(node).c_str());
	} else if(n->type == NODE_KEYWORD) {
		printf("%*s:%s\n", depth, "", get_node_string(node).c_str());
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
		printf("%*s%s = ", depth, "", get_node_string(node).c_str());
		print_node(n->t_var, depth + 1);
	} else if(n->type == NODE_MAP) {
		printf("%*s<map>\n", depth, "");
	} else if(n->type == NODE_SET) {
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
	for(list_t::iterator i = nodes->begin(); i; i++) {
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
	printf("%s", get_node(i)->type_as_string().c_str());
}


// Generic iterator... 
struct seq_iterator_t {
	int type;
	node_idx_t val;
	bool is_done;
	list_t::iterator it;
	vector_t::iterator vit;
	map_t::iterator mit;
	lazy_list_iterator_t lit;

	seq_iterator_t(env_ptr_t env, node_idx_t node_idx) : type(), val(NIL_NODE), is_done(), it(), vit(), mit(), lit(env, node_idx) {
		type = get_node_type(node_idx);
		val = INV_NODE;
		if(type == NODE_LIST) {
			it = get_node(node_idx)->as_list()->begin();
			if(!done()) {
				val = *it;
			}
		} else if(type == NODE_VECTOR) {
			vit = get_node(node_idx)->as_vector()->begin();
			if(!done()) {
				val = *vit;
			}
		} else if(type == NODE_MAP) {
			mit = get_node(node_idx)->as_map()->begin();
			if(!done()) {
				val = mit->second;
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
		} else if(type == NODE_LAZY_LIST) {
			lit.next();
			val = lit ? lit.val : INV_NODE;
		}
		return val;
	}

	node_idx_t nth(int n) {
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

	void prefetch(int n) {
		if(type == NODE_LAZY_LIST) {
			lit.prefetch(n);
		}
	}
};

static bool node_eq(env_ptr_t env, node_idx_t n1i, node_idx_t n2i) {
	if(n1i == INV_NODE || n2i == INV_NODE) {
		return false;
	}

	node_t *n1 = get_node(n1i);
	node_t *n2 = get_node(n2i);

	if(n1->type == NODE_NIL || n2->type == NODE_NIL) {
		return n1->type == NODE_NIL && n2->type == NODE_NIL;
	} else if(n1->is_func() && n2->is_func()) {
		#if 1 // ?? is this correct?
			return n1i == n2i;
		#else // actual function comparison.... not spec compliant it seems
		{ 
			vector_t::iterator i1 = n1->t_func.args->begin();
			vector_t::iterator i2 = n2->t_func.args->begin();
			for(; i1 && i2; ++i1, ++i2) {
				if(!node_eq(env, *i1, *i2)) {
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
				if(!node_eq(env, *i1, *i2)) {
					return false;
				}
			}
			if(i1 || i2) {
				return false;
			}
		}
		return true;
		#endif
	} else if(n1->is_seq() && n2->is_seq()) {
		// in this case we want to iterate over the sequences and compare
		// each element
		seq_iterator_t i1(env, n1i), i2(env, n2i);
		while(i1.val != INV_NODE && i2.val != INV_NODE) {
			if(!node_eq(env, i1.val, i2.val)) {
				return false;
			}
			i1.next();
			i2.next();
		}
		if(i1.val != INV_NODE || i2.val != INV_NODE) {
			return false;
		}
		return true;
	} else if(n1->type == NODE_BOOL && n2->type == NODE_BOOL) {
		return n1->t_bool == n2->t_bool;
	} else if(n1->type == NODE_INT && n2->type == NODE_INT) {
		return n1->t_int == n2->t_int;
	} else if(n1->type == NODE_FLOAT || n2->type == NODE_FLOAT) {
		return n1->as_float() == n2->as_float();
	} else if(n1->flags & n2->flags & NODE_FLAG_STRING) {
		return n1->t_string == n2->t_string;
	}
	return false;
}

static bool node_lt(env_ptr_t env, node_idx_t n1i, node_idx_t n2i) {
	node_t *n1 = get_node(n1i);
	node_t *n2 = get_node(n2i);
	if(n1->type == NODE_NIL || n2->type == NODE_NIL) {
		return false;
	} else if(n1->is_seq() && n2->is_seq()) {
		// in this case we want to iterate over the sequences and compare
		// each element
		seq_iterator_t i1(env, n1i), i2(env, n2i);
		for(; i1 && i2; i1.next(), i2.next()) {
			if(!node_lt(env, i1.val, i2.val)) {
				return false;
			}
		}
		if(i1.val != INV_NODE || i2.val != INV_NODE) {
			if(!node_lt(env, i1.val, i2.val)) {
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

static bool node_lte(env_ptr_t env, node_idx_t n1i, node_idx_t n2i) {
	node_t *n1 = get_node(n1i);
	node_t *n2 = get_node(n2i);
	if(n1->type == NODE_NIL || n2->type == NODE_NIL) {
		return false;
	} else if(n1->is_seq() && n2->is_seq()) {
		// in this case we want to iterate over the sequences and compare
		// each element
		seq_iterator_t i1(env, n1i), i2(env, n2i);
		for(; i1 && i2; i1.next(), i2.next()) {
			if(!node_lte(env, i1.val, i2.val)) {
				return false;
			}
		}
		if(i1.val != INV_NODE || i2.val != INV_NODE) {
			if(!node_lte(env, i1.val, i2.val)) {
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

// jo_hash_value of node_idx_t
template <>
size_t jo_hash_value(node_idx_t n) {
	node_t *n1 = get_node(n);
	if(n1->type == NODE_NIL) {
		return 0;
	} else if(n1->is_seq()) {
		uint32_t res = 0;
		seq_iterator_t i(NULL, n);
		for(; i; i.next()) {
			res = (res * 31) + jo_hash_value(i.val);
		}
		if(i.val != INV_NODE) {
			res = (res * 31) + jo_hash_value(i.val);
		}
		return res;
	} else if(n1->type == NODE_BOOL) {
		return n1->t_bool ? 1 : 0;
	} else if(n1->flags & NODE_FLAG_STRING) {
		return jo_hash_value(n1->t_string.c_str());
	} else if(n1->type == NODE_INT) {
		return n1->t_int;
	} else if(n1->type == NODE_FLOAT) {
		return jo_hash_value(n1->as_float());
	}
	return 0;
}

// Tests the equality between two or more objects
static node_idx_t native_eq(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return TRUE_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = node_eq(env, n1, n2);
	for(; i && ret; i++) {
		ret &= node_eq(env, n1, *i);
	}
	return ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_neq(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return NIL_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = node_eq(env, n1, n2);
	for(; i && ret; i++) {
		ret = ret && node_eq(env, n1, *i);
	}
	return !ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_lt(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return TRUE_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = node_lt(env, n1, n2);
	for(; i && ret; i++) {
		ret = ret && node_lt(env, n2, *i);
		n2 = *i;
	}
	return ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_lte(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return TRUE_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = node_lte(env, n1, n2);
	for(; i && ret; i++) {
		ret = ret && node_lte(env, n2, *i);
		n2 = *i;
	}
	return ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_gt(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return TRUE_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = !node_lte(env, n1, n2);
	for(; i && ret; i++) {
		ret = ret && !node_lte(env, n2, *i);
		n2 = *i;
	}
	return ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_gte(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return TRUE_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = !node_lt(env, n1, n2);
	for(; i && ret; i++) {
		ret = ret && !node_lt(env, n2, *i);
		n2 = *i;
	}
	return ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_if(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return NIL_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t cond = eval_node(env, *i++);
	node_idx_t when_true = *i++;
	node_idx_t when_false = i ? *i++ : NIL_NODE;
	return eval_node(env, get_node(cond)->as_bool() ? when_true : when_false);
}

static node_idx_t native_if_not(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return NIL_NODE;
	}
	list_t::iterator i = args->begin();
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
	list_t::iterator i = args->begin();
	node_idx_t bindings = *i++;
	node_t *bindings_node = get_node(bindings);
	if(bindings_node->type != NODE_VECTOR || bindings_node->t_vector->size() != 2) {
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
	list_t::iterator i = args->begin();
	node_idx_t bindings = *i++;
	node_t *bindings_node = get_node(bindings);
	if(bindings_node->type != NODE_VECTOR || bindings_node->t_vector->size() != 2) {
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

static node_idx_t native_print(env_ptr_t env, list_ptr_t args) {
	for(list_t::iterator i = args->begin(); i; i++) {
		printf("%s", get_node(*i)->as_string().c_str());
	}
	return NIL_NODE;
}

static node_idx_t native_println(env_ptr_t env, list_ptr_t args) {
	native_print(env, args);
	printf("\n");
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
	list_t::iterator i = args->begin();

	if(args->size() == 1) {
		node_idx_t coll = eval_node(env, args->first_value());
		node_t *n = get_node(coll);
		if(!n->is_seq()) {
			return NIL_NODE;
		}
		seq_iterator_t it(env, coll);
		return new_node_list(it.all());
	}

	if(args->size() == 2) {
		int n = get_node(eval_node(env, *i++))->as_int();
		node_idx_t coll = eval_node(env, *i++);
		node_t *n4 = get_node(coll);
		if(!n4->is_seq()) {
			return NIL_NODE;
		}
		list_ptr_t ret = new list_t();
		seq_iterator_t it(env, coll);
		for(; it && n; it.next(), n--) {
			ret->push_back_inplace(it.val);
		}
		if(it.val != NIL_NODE && n) {
			ret->push_back_inplace(it.val);
		}
		return new_node_list(ret);
	}

	return NIL_NODE;
}

static node_idx_t native_while(env_ptr_t env, list_ptr_t args) {
	list_t::iterator i = args->begin();
	node_idx_t cond_idx = *i++;
	node_idx_t cond = eval_node(env, cond_idx);
	node_idx_t ret = NIL_NODE;
	while(get_node(cond)->as_bool()) {
		for(list_t::iterator j = i; j; j++) {
			ret = eval_node(env, *j);
		}
		cond = eval_node(env, cond_idx);
	}
	return ret;
}

static node_idx_t native_quote(env_ptr_t env, list_ptr_t args) { 
	return new_node_list(args);
}

static node_idx_t native_list(env_ptr_t env, list_ptr_t args) { 
	return new_node_list(args); 
}

static node_idx_t native_unquote(env_ptr_t env, list_ptr_t args) { 
	return args->first_value(); 
}

static node_idx_t native_quasiquote_1(env_ptr_t env, node_idx_t arg) {
	node_t *n = get_node(arg);
	if(n->type == NODE_VECTOR) {
		vector_ptr_t ret = new_vector();
		for(auto i = n->t_vector->begin(); i; i++) {
			node_t *n2 = get_node(*i);
			if(n2->type == NODE_LIST && n2->t_list->first_value() == UNQUOTE_SPLICE_NODE) {
				node_idx_t n3_idx = n2->t_list->second_value();
				node_t *n3 = get_node(n3_idx);
				if(n3->is_seq()) {
					seq_iterator_t it(env, n3_idx);
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
		for(auto i = n->as_list()->begin(); i; i++) {
			node_t *n2 = get_node(*i);
			if(n2->type == NODE_LIST && n2->t_list->first_value() == UNQUOTE_SPLICE_NODE) {
				//node_idx_t n3_idx = n2->t_list->second_value();
				node_idx_t n3_idx = eval_node(env, n2->t_list->second_value());
				node_t *n3 = get_node(n3_idx);
				if(n3->is_seq()) {
					seq_iterator_t it(env, n3_idx);
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
		map_ptr_t ret = new_map();
		for(auto i = n->t_map->begin(); i; i++) {
			ret->assoc_inplace(i->first, native_quasiquote_1(env, i->second));
		}
		return new_node_map(ret);
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
	list_t::iterator i = args->begin();
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
	list_t::iterator i = args->begin();
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
	for(list_t::iterator i = args->begin(); i; i++) {
		node_t *n = get_node(*i);
		if(n->is_symbol()) {
			env->set(n->as_string(), NIL_NODE);
		}
	}
	return NIL_NODE;
}

static node_idx_t native_fn_internal(env_ptr_t env, list_ptr_t args, const jo_string &private_fn_name, int flags) {
	list_t::iterator i = args->begin();
	if(get_node_type(*i) == NODE_VECTOR) {
		node_idx_t reti = new_node(NODE_FUNC);
		node_t *ret = get_node(reti);
		ret->flags |= flags;
		ret->t_func.args = get_node(*i)->t_vector;
		ret->t_func.body = args->rest();
		if(private_fn_name.empty()) {
			ret->t_string = jo_string("<anonymous>");
			ret->t_func.env = env;
		} else {
			ret->t_string = private_fn_name;
			env_ptr_t private_env = new_env(env);
			private_env->set(private_fn_name, reti);
			ret->t_func.env = private_env;
		}
		return reti;
	}
	return NIL_NODE;
}

static node_idx_t native_fn_macro(env_ptr_t env, list_ptr_t args, bool macro) {
	list_t::iterator i = args->begin();
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
			int num_args = args->size();
			for(auto i = fn_list->begin(); i; i++) {
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

static node_idx_t native_fn(env_ptr_t env, list_ptr_t args) {
	return native_fn_macro(env, args, false);
}

static node_idx_t native_macro(env_ptr_t env, list_ptr_t args) {
	return native_fn_macro(env, args, true);
}

// (defn name doc-string? attr-map? [params*] prepost-map? body)
// (defn name doc-string? attr-map? ([params*] prepost-map? body) + attr-map?)
// Same as (def name (fn [params* ] exprs*)) or (def
// name (fn ([params* ] exprs*)+)) with any doc-string or attrs added
// to the var metadata. prepost-map defines a map with optional keys
// :pre and :post that contain collections of pre or post conditions.
static node_idx_t native_defn(env_ptr_t env, list_ptr_t args) {
	list_t::iterator i = args->begin();
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
	list_t::iterator i = args->begin();
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

static node_idx_t native_is_zero(env_ptr_t env, list_ptr_t args) {
	node_idx_t n = args->first_value();
	if(get_node_type(n) == NODE_INT) {
		return get_node(n)->as_int() == 0 ? TRUE_NODE : FALSE_NODE;
	}
	if(get_node_type(n) == NODE_FLOAT) {
		return get_node(n)->as_float() == 0.0 ? TRUE_NODE : FALSE_NODE;
	}
	return FALSE_NODE;
}

// This statement takes a set of test/expression pairs. 
// It evaluates each test one at a time. 
// If a test returns logical true, ‘cond’ evaluates and returns the value of the corresponding expression 
// and doesn't evaluate any of the other tests or expressions. ‘cond’ returns nil.
static node_idx_t native_cond(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	while(it) {
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
	list_t::iterator it = args->begin();
	node_idx_t cond_idx = eval_node(env, *it++);
	node_idx_t next = *it++;
	while(it) {
		node_idx_t body_idx = *it++;
		node_idx_t value_idx = eval_node(env, next);
		node_t *value = get_node(value_idx);
		if(node_eq(env, value_idx, cond_idx)) {
			return eval_node(env, body_idx);
		}
		next = *it++;
	}
	// default: 
	return eval_node(env, next);
}

static node_idx_t native_dotimes(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
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
	int times = get_node(value_idx)->as_int();
	jo_string name = get_node(name_idx)->as_string();
	env_ptr_t env2 = new_env(env);
	node_idx_t ret = NIL_NODE;
	for(int i = 0; i < times; ++i) {
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
	list_t::iterator it = args->begin();
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
	list_ptr_t value_list = value->as_list();
	node_idx_t ret = NIL_NODE;
	env_ptr_t env2 = new_env(env);
	for(list_t::iterator it2 = value_list->begin(); it2; it2++) {
		env2->set_temp(name, *it2);
		for(list_t::iterator it3 = it; it3; it3++) { 
			ret = eval_node(env2, *it3);
		}
	}
	return ret;
}

static node_idx_t native_delay(env_ptr_t env, list_ptr_t args) {
	node_idx_t reti = new_node(NODE_DELAY);
	node_t *ret = get_node(reti);
	//ret->t_func.args  // no args for delays...
	ret->t_func.body = args;
	ret->t_func.env = env;
	ret->t_delay = INV_NODE;
	return reti;
}

static node_idx_t native_when(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t ret = NIL_NODE;
	if(get_node_bool(*it++)) {
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
	list_t::iterator it = args->begin();
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
		if(get_node_type(key_idx) == NODE_VECTOR && get_node_type(value_idx) == NODE_LIST) {
			env2->set_temp(get_node_vector(key_idx), get_node_list(value_idx));
		} else {
			env2->set_temp(get_node_string(key_idx), value_idx);
		}
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
	list_t::iterator it = args->begin();
	node_idx_t first_idx = *it++;
	int first_type = get_node_type(first_idx);
	list_ptr_t list;
	vector_ptr_t vec;
	if(first_type == NODE_NIL) {
		list = new_list();
	} else if(first_type == NODE_LIST) {
		list = get_node(first_idx)->as_list();
	} else if(first_type == NODE_VECTOR) {
		vec = get_node(first_idx)->as_vector();
	} else {
		list = new_list();
		list->push_front_inplace(first_idx);
	}
	if(list.ptr) {
		for(; it; it++) {
			node_idx_t second_idx = *it;
			node_t *second = get_node(second_idx);
			list = list->push_front(second_idx);
		}
		return new_node_list(list);
	} else {
		for(; it; it++) {
			node_idx_t second_idx = *it;
			node_t *second = get_node(second_idx);
			vec = vec->push_back(second_idx);
		}
		return new_node_vector(vec);
	}
}

static node_idx_t native_pop(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	if(list->is_list()) {
		list_ptr_t list_list = list->as_list();
		if(list_list->size() == 0) {
			return NIL_NODE;
		}
		return new_node_list(list_list->pop());
	}
	if(list->is_vector()) {
		vector_ptr_t list_vec = list->as_vector();
		if(list_vec->size() == 0) {
			return NIL_NODE;
		}
		return new_node_vector(list_vec->pop());
	}
	if(list->is_string()) {
		jo_string list_str = list->as_string();
		if(list_str.size() == 0) {
			return NIL_NODE;
		}
		return new_node_string(list_str.substr(1));
	}
	if(list->is_lazy_list()) {
		return list->seq_rest(env);
	}
	return NIL_NODE;
}

static node_idx_t native_peek(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	if(list->is_list()) {
		list_ptr_t list_list = list->t_list;
		if(list_list->size() == 0) {
			return NIL_NODE;
		}
		return list_list->first_value();
	}
	if(list->is_vector()) {
		vector_ptr_t list_vec = list->as_vector();
		if(list_vec->size() == 0) {
			return NIL_NODE;
		}
		return list_vec->first_value();
	}
	if(list->is_string()) {
		jo_string s = list->as_string();
		if(s.size() == 0) {
			return NIL_NODE;
		}
		return new_node_int(s.c_str()[0]);
	}
	if(list->is_lazy_list()) {
		return list->seq_first(env);
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
	list_t::iterator it = args->begin();
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	if(list->is_string()) {
		return new_node_int(list->as_string().size());
	}
	if(list->is_list()) {
		list_ptr_t list_list = list->as_list();
		return new_node_int(list_list->size());
	}
	if(list->is_vector()) {
		vector_ptr_t list_vec = list->as_vector();
		return new_node_int(list_vec->size());
	}
	if(list->is_map()) {
		map_ptr_t list_map = list->as_map();
		return new_node_int(list_map->size());
	}
	if(list->is_lazy_list()) {
		lazy_list_iterator_t lit(env, list_idx);
		return new_node_int(lit.all()->size());
	}
	return ZERO_NODE;
}

static node_idx_t native_is_delay(env_ptr_t env, list_ptr_t args) {	return get_node_type(args->first_value()) == NODE_DELAY ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_empty(env_ptr_t env, list_ptr_t args) { return get_node(args->first_value())->seq_empty(env) ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_false(env_ptr_t env, list_ptr_t args) { return get_node_bool(args->first_value()) ? FALSE_NODE : TRUE_NODE; }
static node_idx_t native_is_true(env_ptr_t env, list_ptr_t args) { return get_node_bool(args->first_value()) ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_some(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) != NODE_NIL ? TRUE_NODE : FALSE_NODE; }

// (first coll)
// Returns the first item in the collection. Calls seq on its argument. If coll is nil, returns nil.
static node_idx_t native_first(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_string()) {
		return new_node_int(node->as_string().c_str()[0]);
	}
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		if(list_list->size() == 0) {
			return NIL_NODE;
		}
		return list_list->first_value();
	}
	if(node->is_vector()) {
		vector_ptr_t list_vec = node->as_vector();
		if(list_vec->size() == 0) {
			return NIL_NODE;
		}
		return list_vec->first_value();
	}
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, node_idx);
		return lit.val;
	}
	return NIL_NODE;
}

static node_idx_t native_second(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_string()) {
		jo_string &str = node->t_string;
		if(str.size() < 2) {
			return NIL_NODE;
		}
		return new_node_int(node->as_string().c_str()[1]);
	}
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		if(list_list->size() <= 1) {
			return NIL_NODE;
		}
		return list_list->nth(1);
	}
	if(node->is_vector()) {
		vector_ptr_t list_vec = node->as_vector();
		if(list_vec->size() <= 1) {
			return NIL_NODE;
		}
		return list_vec->nth(1);
	}
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, node_idx);
		return lit.nth(1);
	}
	return NIL_NODE;
}

static node_idx_t native_last(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_string()) {
		jo_string &str = node->t_string;
		if(str.size() < 1) {
			return NIL_NODE;
		}
		return new_node_int(node->as_string().c_str()[str.size() - 1]);
	}
	if(node->is_list()) {
		return node->as_list()->last_value();
	}
	if(node->is_vector()) {
		return node->as_vector()->last_value();
	}
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, node_idx);
		for(; !lit.done(); lit.next()) {}
		return lit.val;
	}
	return NIL_NODE;
}

static node_idx_t native_nth(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	node_idx_t n_idx = *it++;
	int n = get_node(n_idx)->as_int();
	if(list->is_string()) {
		jo_string &str = list->t_string;
		if(n < 0) {
			n = str.size() + n;
		}
		if(n < 0 || n >= str.size()) {
			return NIL_NODE;
		}
		return new_node_int(str.c_str()[n]);
	}
	if(list->is_list()) {
		if(n < 0 || n >= list->as_list()->size()) {
			return NIL_NODE;
		}
		return list->as_list()->nth(n);
	}
	if(list->is_vector()) {
		if(n < 0 || n >= list->as_vector()->size()) {
			return NIL_NODE;
		}
		return list->as_vector()->nth(n);
	}
	if(list->is_lazy_list()) {
		lazy_list_iterator_t lit(env, list_idx);
		return lit.nth(n);
	}
	return NIL_NODE;
}

// (rand-nth coll)
// Return a random element of the (sequential) collection. Will have
// the same performance characteristics as nth for the given
// collection.
static node_idx_t native_rand_nth(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	if(list->is_string()) {
		jo_string &str = list->t_string;
		if(str.size() == 0) {
			return NIL_NODE;
		}
		return new_node_int(str.c_str()[rand() % str.size()]);
	}
	if(list->is_list()) {
		size_t list_size = list->as_list()->size();
		if(list_size > 0) {
			int n = rand() % list_size;
			return list->as_list()->nth(n);
		}
	}
	if(list->is_vector()) {
		size_t list_size = list->as_vector()->size();
		if(list_size > 0) {
			int n = rand() % list_size;
			return list->as_vector()->nth(n);
		}
	}
	return NIL_NODE;
}

// equivalent to (first (first x))
static node_idx_t native_ffirst(env_ptr_t env, list_ptr_t args) { return native_first(env, new_list(native_first(env, args))); }
static node_idx_t native_is_fn(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_FUNC ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_int(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_INT ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_int(env_ptr_t env, list_ptr_t args) { return new_node_int(get_node(args->first_value())->as_int()); }
static node_idx_t native_is_indexed(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_VECTOR ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_letter(env_ptr_t env, list_ptr_t args) { return jo_isletter(get_node_int(args->first_value())) ? TRUE_NODE : FALSE_NODE; }

// Returns true if x implements IFn. Note that many data structures
// (e.g. sets and maps) implement IFn
static node_idx_t native_is_ifn(env_ptr_t env, list_ptr_t args) {
	int type =  get_node_type(args->first_value());
	return type == NODE_FUNC || type == NODE_NATIVE_FUNC || type == NODE_VECTOR || type == NODE_MAP || type == NODE_SET || type == NODE_SYMBOL || type == NODE_KEYWORD ? TRUE_NODE : FALSE_NODE;
}

// Return true if x is a symbol or keyword
static node_idx_t native_is_ident(env_ptr_t env, list_ptr_t args) {
	int type =  get_node_type(args->first_value());
	return type == NODE_SYMBOL || type == NODE_KEYWORD ? TRUE_NODE : FALSE_NODE;
}

// (next coll) 
// Returns a seq of the items after the first. Calls seq on its
// argument.  If there are no more items, returns nil.
static node_idx_t native_next(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		if(list_list->size() == 0) {
			return NIL_NODE;
		}
		return new_node_list(list_list->rest());
	}
	if(node->is_vector()) {
		vector_ptr_t vec = node->as_vector();
		if(vec->size() == 0) {
			return NIL_NODE;
		}
		return new_node_vector(vec->rest());
	}
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, node_idx);
		if(lit.done()) {
			return NIL_NODE;
		}
		return new_node_lazy_list(lit.next_fn());
	}
	return NIL_NODE;
}

static node_idx_t native_nfirst(env_ptr_t env, list_ptr_t args) { return native_next(env, new_list(native_first(env, args))); }
static node_idx_t native_nnext(env_ptr_t env, list_ptr_t args) { return native_next(env, new_list(native_next(env, args))); }
static node_idx_t native_fnext(env_ptr_t env, list_ptr_t args) { return native_first(env, new_list(native_next(env, args))); }

// like next, but always returns a list
static node_idx_t native_rest(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_string()) {
		jo_string &str = node->t_string;
		if(str.size() == 0) {
			return NIL_NODE;
		}
		return new_node_string(str.substr(1));
	}
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		return new_node_list(list_list->rest());
	}
	if(node->is_vector()) {
		vector_ptr_t vec = node->as_vector();
		return new_node_vector(vec->rest());	
	}
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, node_idx);
		if(lit.done()) {
			return new_node_list(new_list()); // empty list
		}
		return new_node_lazy_list(lit.next_fn());
	}
	return NIL_NODE;
}

static node_idx_t native_when_not(env_ptr_t env, list_ptr_t args) { return !get_node_bool(args->first_value()) ? eval_node_list(env, args->rest()) : NIL_NODE; }

// (let [a 1 b 2] (+ a b))
static node_idx_t native_let(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(!node->is_vector()) {
		printf("let: expected vector\n");
		return NIL_NODE;
	}
	vector_ptr_t list_list = node->as_vector();
	if(list_list->size() % 2 != 0) {
		printf("let: expected even number of elements\n");
		return NIL_NODE;
	}
	env_ptr_t env2 = new_env(env);
	for(vector_t::iterator i = list_list->begin(); i;) {
		node_idx_t key_idx = *i++; // TODO: should this be eval'd?
		node_idx_t value_idx = eval_node(env2, *i++);
		node_t *key = get_node(key_idx);
		env2->set_temp(key->as_string(), value_idx);
	}
	return eval_node_list(env2, args->rest());
}

static node_idx_t native_apply(env_ptr_t env, list_ptr_t args) {
	// collect the arguments, if its a list add the whole list, then eval it
	list_ptr_t arg_list = new_list();
	for(list_t::iterator it = args->begin(); it; it++) {
		node_idx_t arg_idx = eval_node(env, *it);
		node_t *arg = get_node(arg_idx);
		if(arg->is_list()) {
			arg_list->conj_inplace(*arg->as_list().ptr);
		} else if(arg->is_lazy_list()) {
			for(lazy_list_iterator_t lit(env, arg_idx); !lit.done(); lit.next()) {
				arg_list->push_back_inplace(lit.val);
			}
		} else if(arg->is_vector()) {
			for(vector_t::iterator vit = arg->as_vector()->begin(); vit; vit++) {
				arg_list->push_back_inplace(*vit);
			}
		} else {
			arg_list->push_back_inplace(arg_idx);
		}
	}
	return eval_list(env, arg_list);
}

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
	list_t::iterator it = args->begin();
	node_idx_t f_idx = *it++;
	// (reduce f coll)
	// returns the result of applying f to the first 2 items in coll, then applying f to that result and the 3rd item, etc. 
	// If coll contains no items, f must accept no arguments as well, and reduce returns the result of calling f with no arguments.  
	// If coll has only 1 item, it is returned and f is not called.  
	if(args->size() == 2) {
		node_idx_t coll_idx = eval_node(env, *it++);
		node_t *coll = get_node(coll_idx);
		if(coll->is_list()) {
			list_ptr_t list_list = coll->as_list();
			if(list_list->size() == 0) {
				return eval_va(env, 1, f_idx);
			}
			list_t::iterator it2 = list_list->begin();
			node_idx_t reti = *it2++;
			while(it2) {
				reti = eval_va(env, 3, f_idx, reti, *it2++);
			}
			return reti;
		}
		if(coll->is_vector()) {
			vector_ptr_t vector_list = coll->as_vector();
			if(vector_list->size() == 0) {
				return eval_va(env, 1, f_idx);
			}
			vector_t::iterator it2 = vector_list->begin();
			node_idx_t reti = *it2++;
			while(it2) {
				reti = eval_va(env, 3, f_idx, reti, *it2++);
			}
			return reti;
		}
		if(coll->is_lazy_list()) {
			lazy_list_iterator_t lit(env, coll_idx);
			node_idx_t reti = lit.val;
			for(lit.next(); !lit.done(); lit.next()) {
				reti = eval_va(env, 3, f_idx, reti, lit.val);
			}
			return reti;
		}
		warnf("reduce: expected list or lazy list\n");
		return NIL_NODE;
	}
	// (reduce f val coll)
	// returns the result of applying f to val and the first item in coll,
	//  then applying f to that result and the 2nd item, etc.
	// If coll contains no items, returns val and f is not called.
	if(args->size() == 3) {
		node_idx_t reti = eval_node(env, *it++);
		node_idx_t coll = eval_node(env, *it++);
		node_t *coll_node = get_node(coll);
		if(coll_node->is_list()) {
			list_ptr_t list_list = coll_node->as_list();
			if(list_list->size() == 0) {
				return reti;
			}
			list_t::iterator it2 = list_list->begin();
			while(it2) {
				reti = eval_va(env, 3, f_idx, reti, *it2++);
			}
			return reti;
		}
		if(coll_node->is_vector()) {
			vector_ptr_t vector_list = coll_node->as_vector();
			if(vector_list->size() == 0) {
				return reti;
			}
			vector_t::iterator it2 = vector_list->begin();
			while(it2) {
				reti = eval_va(env, 3, f_idx, reti, *it2++);
			}
			return reti;
		}
		if(coll_node->is_lazy_list()) {
			lazy_list_iterator_t lit(env, coll);
			for(; !lit.done(); lit.next()) {
				reti = eval_va(env, 3, f_idx, reti, lit.val);
			}
			return reti;
		}
		warnf("reduce: expected list or lazy list\n");
		return NIL_NODE;
	}
	return NIL_NODE;
}

// eval each arg in turn, return if any eval to false
static node_idx_t native_and(env_ptr_t env, list_ptr_t args) {
	for(list_t::iterator it = args->begin(); it; it++) {
		if(!get_node_bool(eval_node(env, *it))) {
			return FALSE_NODE;
		}
	}
	return TRUE_NODE;
}

static node_idx_t native_or(env_ptr_t env, list_ptr_t args) {
	for(list_t::iterator it = args->begin(); it; it++) {
		if(get_node_bool(eval_node(env, *it))) {
			return TRUE_NODE;
		}
	}
	return FALSE_NODE;
}

static node_idx_t native_not(env_ptr_t env, list_ptr_t args) { return !get_node_bool(args->first_value()) ? TRUE_NODE : FALSE_NODE; }

static node_idx_t native_reverse(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
	int node_idx = *it++;
    node_t *node = get_node(node_idx);
	if(node->is_string()) {
		return new_node_string(node->as_string().reverse());
	}
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		return new_node_list(list_list->reverse());
	}
	if(node->is_vector()) {
		vector_ptr_t vector_list = node->as_vector();
		return new_node_vector(vector_list->reverse());
	}
	if(node->is_map()) {
		return node_idx; // nonsensical to reverse an "unordered" map. just give back the input.
	}
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, node_idx);
		list_ptr_t list_list = new_list();
		for(; !lit.done(); lit.next()) {
			list_list->push_front_inplace(lit.val);
		}
		return new_node_list(list_list);
	}
	return NIL_NODE;
}

static node_idx_t native_eval(env_ptr_t env, list_ptr_t args) { return eval_node(env, args->first_value()); }

// (into) 
// (into to)
// (into to from)
// Returns a new coll consisting of to-coll with all of the items of
//  from-coll conjoined.
// A non-lazy concat
static node_idx_t native_into(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t to = *it++;
	node_idx_t from = *it++;
	if(get_node_type(to) == NODE_LIST) {
		list_ptr_t ret = new list_t(*get_node(to)->t_list);
		if(get_node_type(from) == NODE_LIST) {
			for(list_t::iterator it = get_node(from)->t_list->begin(); it; it++) {
				ret->push_front_inplace(*it);
			}
		} else if(get_node_type(from) == NODE_VECTOR) {
			for(vector_t::iterator it = get_node(from)->t_vector->begin(); it; it++) {
				ret->push_front_inplace(*it);
			}
		} else if(get_node_type(from) == NODE_LAZY_LIST) {
			for(lazy_list_iterator_t lit(env, from); !lit.done(); lit.next()) {
				ret->push_front_inplace(lit.val);
			}
		} else if(get_node_type(from) == NODE_MAP) {
			map_ptr_t from_map = get_node(from)->t_map;
			for(map_t::iterator it = from_map->begin(); it != from_map->end(); it++) {
				ret->push_front_inplace(it->second);
			}
		}
		return new_node_list(ret);
	}
	if(get_node_type(to) == NODE_VECTOR) {
		vector_ptr_t ret = new vector_t(*get_node(to)->t_vector);
		if(get_node_type(from) == NODE_LIST) {
			for(list_t::iterator it = get_node(from)->t_list->begin(); it; it++) {
				ret->push_front_inplace(*it);
			}
		} else if(get_node_type(from) == NODE_VECTOR) {
			for(vector_t::iterator it = get_node(from)->t_vector->begin(); it; it++) {
				ret->push_front_inplace(*it);
			}
		} else if(get_node_type(from) == NODE_LAZY_LIST) {
			for(lazy_list_iterator_t lit(env, from); !lit.done(); lit.next()) {
				ret->push_front_inplace(lit.val);
			}
		} else if(get_node_type(from) == NODE_MAP) {
			map_ptr_t from_map = get_node(from)->t_map;
			for(map_t::iterator it = from_map->begin(); it != from_map->end(); it++) {
				ret->push_front_inplace(it->second);
			}
		}
		return new_node_vector(ret);
	}
	if(get_node_type(to) == NODE_MAP) {
		map_ptr_t ret = new map_t(*get_node(to)->t_map);
		if(get_node_type(from) == NODE_LIST) {
			for(list_t::iterator it = get_node(from)->t_list->begin(); it; it++) {
				if(get_node_type(*it) == NODE_MAP) {
					ret = ret->conj(get_node(*it)->t_map.ptr);
				}
			}
		}
		if(get_node_type(from) == NODE_VECTOR) {
			for(vector_t::iterator it = get_node(from)->t_vector->begin(); it; it++) {
				if(get_node_type(*it) == NODE_MAP) {
					ret = ret->conj(get_node(*it)->t_map.ptr);
				}
			}
		}
		if(get_node_type(from) == NODE_LAZY_LIST) {
			for(lazy_list_iterator_t lit(env, from); !lit.done(); lit.next()) {
				if(get_node_type(lit.val) == NODE_MAP) {
					ret = ret->conj(get_node(lit.val)->t_map.ptr);
				}
			}
		}
		if(get_node_type(from) == NODE_MAP) {
			ret = ret->conj(get_node(from)->t_map.ptr);
		}
		return new_node_map(ret);
	}
	return NIL_NODE;
}

// Compute the time to execute arguments
static node_idx_t native_time(env_ptr_t env, list_ptr_t args) {
	double time_start = jo_time();
	for(list_t::iterator it = args->begin(); it; it++) {
		node_idx_t node_idx = eval_node(env, *it);
	}
	double time_end = jo_time();
	return new_node_float(time_end - time_start);
}


// (hash-map)(hash-map & keyvals)
// keyvals => key val
// Returns a new hash map with supplied mappings.  If any keys are
// equal, they are handled as if by repeated uses of assoc.
static node_idx_t native_hash_map(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	map_ptr_t map = new_map();
	while(it) {
		node_idx_t k = eval_node(env, *it++);
		if(!it) {
			break;
		}
		node_idx_t v = eval_node(env, *it++);
		map->assoc_inplace(k, v, [env](node_idx_t k, node_idx_t v) {
			return node_eq(env, k, v);
		});
	}
	return new_node_map(map);
}

// (assoc map key val)(assoc map key val & kvs)
// assoc[iate]. When applied to a map, returns a new map of the
// same (hashed/sorted) type, that contains the mapping of key(s) to
// val(s). When applied to a vector, returns a new vector that
// contains val at index. Note - index must be <= (count vector).
static node_idx_t native_assoc(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t map_idx = *it++;
	node_idx_t key_idx = *it++;
	node_idx_t val_idx = *it++;
	node_t *map_node = get_node(map_idx);
	node_t *key_node = get_node(key_idx);
	if(map_node->is_map()) {
		map_ptr_t map = map_node->t_map->assoc(key_idx, val_idx, [env](node_idx_t k, node_idx_t v) {
			return node_eq(env, k, v);
		});
		return new_node_map(map);
	} 
	if(map_node->is_vector()) {
		vector_ptr_t vector = map_node->t_vector->assoc(key_node->as_int(), val_idx);
		return new_node_vector(vector);
	}
	return NIL_NODE;
}

// (dissoc map)(dissoc map key)(dissoc map key & ks)
// dissoc[iate]. Returns a new map of the same (hashed/sorted) type,
// that does not contain a mapping for key(s).
static node_idx_t native_dissoc(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t map_idx = *it++;
	node_idx_t key_idx = *it++;
	node_t *map_node = get_node(map_idx);
	if(map_node->is_map()) {
		map_ptr_t map = map_node->t_map->dissoc(key_idx, [env](node_idx_t k, node_idx_t v) {
			return node_eq(env, k, v);
		});
		return new_node_map(map);
	} 
	if(map_node->is_vector()) {
		vector_ptr_t vector = map_node->t_vector->dissoc(get_node_int(key_idx));
		return new_node_vector(vector);
	}
	return NIL_NODE;
}

// (get map key)(get map key not-found)
// Returns the value mapped to key, not-found or nil if key not present.
static node_idx_t native_get(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t map_idx = *it++;
	node_idx_t key_idx = *it++;
	node_idx_t not_found_idx = it ? *it++ : NIL_NODE;
	node_t *map_node = get_node(map_idx);
	node_t *key_node = get_node(key_idx);
	node_t *not_found_node = get_node(not_found_idx);
	if(map_node->is_map()) {
		auto entry = map_node->t_map->find(key_idx, [env](node_idx_t k, node_idx_t v) {
			return node_eq(env, k, v);
		});
		if(entry.third) {
			return entry.second;
		}
		return not_found_idx;
	}
	if(map_node->is_vector()) {
		int vec_idx = key_node->as_int();
		if(vec_idx < 0 || vec_idx > map_node->t_vector->size()) {
			return not_found_idx;
		}
		return map_node->t_vector->nth(key_node->as_int());
	}
	return NIL_NODE;
}

// (comp)(comp f)(comp f g)(comp f g & fs)
// Takes a set of functions and returns a fn that is the composition
// of those fns.  The returned fn takes a variable number of args,
// applies the rightmost of fns to the args, the next
// fn (right-to-left) to the result, etc.
static node_idx_t native_comp(env_ptr_t env, list_ptr_t args) {
	list_ptr_t rargs = args->reverse();
	list_t::iterator it = args->begin(); // TODO: maybe reverse iterator (would be faster)
	node_idx_t ret = new_node_native_function("comp-lambda", [=](env_ptr_t env, list_ptr_t args) {
		list_t::iterator it = rargs->begin();
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
	list_t::iterator it = args->begin();
	node_idx_t coll_idx = *it++;
	int type = get_node_type(coll_idx);
	if(type == NODE_LIST)   return new_node_list(get_node(coll_idx)->t_list->shuffle());
	if(type == NODE_VECTOR) return new_node_vector(get_node(coll_idx)->t_vector->shuffle());
	if(type == NODE_MAP)    return coll_idx;
	return NIL_NODE;
}

// (random-sample prob)(random-sample prob coll)
// Returns items from coll with random probability of prob (0.0 -
// 1.0).  Returns a transducer when no collection is provided.
static node_idx_t native_random_sample(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t prob_idx = *it++;
	node_idx_t coll_idx = it ? *it++ : NIL_NODE;
	node_t *prob_node = get_node(prob_idx);
	node_t *coll_node = get_node(coll_idx);
	float prob = prob_node->as_float();
	prob = prob < 0 ? 0 : prob > 1 ? 1 : prob;
	if(coll_node->is_list()) {
		return new_node_list(coll_node->t_list->random_sample(prob));
	}
	if(coll_node->is_vector()) {
		return new_node_vector(coll_node->t_vector->random_sample(prob));
	}
	if(coll_node->is_lazy_list()) {
		list_ptr_t ret = new_list();
		for(lazy_list_iterator_t lit(env, coll_idx); lit; lit.next()) {
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
	list_t::iterator it = args->begin();
	node_idx_t form_idx = *it++;
	node_idx_t msg_idx = it ? *it++ : NIL_NODE;
	node_t *form_node = get_node(eval_node(env, form_idx));
	node_t *msg_node = get_node(eval_node(env, msg_idx));
	if(!form_node->as_bool()) {
		if(msg_node->is_string()) {
			printf("%s\n", msg_node->t_string.c_str());
		} else {
			printf("Assertion failed\n");
			print_node(form_idx);
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
	list_t::iterator it = args->begin();
	node_idx_t coll_idx = *it++;
	node_idx_t n_idx = *it++;
	node_t *coll_node = get_node(coll_idx);
	node_t *n_node = get_node(n_idx);
	int n = n_node->as_int();
	if(n <= 0) {
		return coll_idx;
	}
	if(coll_node->is_string()) {
		jo_string &str = coll_node->t_string;
		if(n < 0) {
			n = str.size() + n;
		}
		if(n < 0 || n >= str.size()) {
			return new_node_string("");
		}
		return new_node_string(str.substr(n));
	}
	if(coll_node->is_list()) {
		return new_node_list(coll_node->t_list->drop(n));
	}
	if(coll_node->is_vector()) {
		return new_node_vector(coll_node->t_vector->drop(n));
	}
	if(coll_node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, coll_idx);
		lit.nth(n);
		if(lit.done()) {
			return NIL_NODE;
		}
		return new_node_list(lit.all());
	}
	return NIL_NODE;
}

static node_idx_t native_nthnext(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t coll_idx = *it++;
	node_idx_t n_idx = *it++;
	node_t *coll_node = get_node(coll_idx);
	node_t *n_node = get_node(n_idx);
	int n = n_node->as_int();
	if(n <= 0) {
		return NIL_NODE;
	}
	if(coll_node->is_string()) {
		jo_string &str = coll_node->t_string;
		if(n < 0) {
			n = str.size() + n;
		}
		if(n < 0 || n >= str.size()) {
			return new_node_string("");
		}
		return new_node_string(str.substr(n));
	}
	if(coll_node->is_list()) {
		if(n >= coll_node->t_list->size()) {
			return NIL_NODE;
		}
		return new_node_list(coll_node->t_list->take(n));
	}
	if(coll_node->is_vector()) {
		if(n >= coll_node->t_vector->size()) {
			return NIL_NODE;
		}
		return new_node_vector(coll_node->t_vector->take(n));
	}
	if(coll_node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, coll_idx);
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
	list_t::iterator it = args->begin();
	node_idx_t x_idx = eval_node(env, *it++);
	for(; it; it++) {
		node_idx_t form_idx = *it;
		node_t *form_node = get_node(form_idx);
		list_ptr_t args2;
		if(get_node_type(*it) == NODE_LIST) {
			list_ptr_t form_list = form_node->t_list;
			args2 = form_list->rest();
			args2->push_front_inplace(x_idx);
			args2->push_front_inplace(form_list->first_value());
		} else {
			args2 = new_list();
			args2->push_front_inplace(x_idx);
			args2->push_front_inplace(*it);
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
	list_t::iterator it = args->begin();
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
			args2->push_front_inplace(x_idx);
			args2->push_front_inplace(*it);
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
	list_t::iterator it = args->begin();
	node_idx_t expr_idx = eval_node(env, *it++);
	node_idx_t name_idx = eval_node(env, *it++);
	jo_string name = get_node(name_idx)->t_string;
	env_ptr_t env2 = new_env(env);
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
	list_t::iterator it = args->begin();
	node_idx_t pred_idx = *it++;
	node_idx_t coll_idx = *it++;
	node_t *pred_node = get_node(pred_idx);
	node_t *coll_node = get_node(coll_idx);
	if(!pred_node->can_eval()) {
		return new_node_bool(false);
	}
	if(coll_node->is_list()) {
		list_ptr_t coll_list = coll_node->t_list;
		for(list_t::iterator it = coll_list->begin(); it; it++) {
			if(!get_node_bool(eval_va(env, 2, pred_idx, *it))) {
				return new_node_bool(false);
			}
		}
		return new_node_bool(true);
	}
	if(coll_node->is_vector()) {
		vector_ptr_t coll_vector = coll_node->t_vector;
		for(vector_t::iterator it = coll_vector->begin(); it; it++) {
			if(!get_node_bool(eval_va(env, 2, pred_idx, *it))) {
				return new_node_bool(false);
			}
		}
		return new_node_bool(true);
	}
	if(coll_node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, coll_idx);
		for(; !lit.done(); lit.next()) {
			if(!get_node_bool(eval_va(env, 2, pred_idx, lit.val))) {
				return new_node_bool(false);
			}
		}
		return new_node_bool(true);
	}
	return new_node_bool(false);
}

static node_idx_t native_is_seqable(env_ptr_t env, list_ptr_t args) { return get_node(args->first_value())->is_seq() ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_any(env_ptr_t env, list_ptr_t args) { return TRUE_NODE; }
static node_idx_t native_boolean(env_ptr_t env, list_ptr_t args) { return get_node_bool(args->first_value()) ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_boolean(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_BOOL ? TRUE_NODE : FALSE_NODE; }

// (array-map)(array-map & keyvals)
// Constructs an array-map. If any keys are equal, they are handled as
// if by repeated uses of assoc.
static node_idx_t native_array_map(env_ptr_t env, list_ptr_t args) {
	if(args->size() & 1) {
		warnf("(array-map) requires an even number of arguments");
		return NIL_NODE;
	}
	map_ptr_t map = new_map();
	for(list_t::iterator it = args->begin(); it; ) {
		node_idx_t key_idx = *it++;
		node_idx_t val_idx = *it++;
		map->assoc_inplace(key_idx, val_idx);
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
		for(list_t::iterator it = coll_list->begin(); it; ) {
			node_idx_t item_idx = *it++;
			if(!it) {
				break;
			}
			ret->push_back_inplace(*it);
		}
		return new_node_list(ret);
	}
	if(coll_node->is_vector()) {
		vector_ptr_t coll_vector = coll_node->t_vector;
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
		lazy_list_iterator_t lit(env, coll_idx);
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

	list_t::iterator it = args->begin();
	node_idx_t value_idx = eval_node(env, *it++);
	while(it) {
		node_idx_t test_idx = eval_node(env, *it++);
		node_idx_t form_idx = *it++;
		if(test_idx == TRUE_NODE) {
			int form_type = get_node_type(form_idx);
			if(form_type == NODE_SYMBOL || form_type == NODE_FUNC || form_type == NODE_NATIVE_FUNC) {
				list_ptr_t form_args = new_list();
				form_args->push_front_inplace(value_idx);
				form_args->push_front_inplace(form_idx);
				value_idx = eval_list(env, form_args);
			} else if(form_type == NODE_LIST) {
				list_ptr_t form_args = get_node(form_idx)->t_list;
				node_idx_t sym = form_args->first_value();
				form_args = form_args->pop_front();
				form_args->push_front_inplace(value_idx);
				form_args->push_front_inplace(sym);
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

	list_t::iterator it = args->begin();
	node_idx_t value_idx = eval_node(env, *it++);
	while(it) {
		node_idx_t test_idx = eval_node(env, *it++);
		node_idx_t form_idx = *it++;
		if(test_idx == TRUE_NODE) {
			int form_type = get_node_type(form_idx);
			if(form_type == NODE_SYMBOL || form_type == NODE_FUNC || form_type == NODE_NATIVE_FUNC) {
				list_ptr_t form_args = new_list();
				form_args->push_front_inplace(value_idx);
				form_args->push_front_inplace(form_idx);
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
	int num_args = args->size();
	if(num_args < 3) {
		warnf("(condp) requires at least 3 arguments\n");
		return NIL_NODE;
	}

	list_t::iterator it = args->begin();
	node_idx_t pred_idx = eval_node(env, *it++);
	node_idx_t expr_idx = eval_node(env, *it++);
	int pred_type = get_node_type(pred_idx);
	if(pred_type != NODE_FUNC && pred_type != NODE_NATIVE_FUNC) {
		warnf("(condp) requires a function as its first argument\n");
		return NIL_NODE;
	}

	int num_clauses = (num_args - 2) / 2;
	for(int i = 0; i < num_clauses; i++) {
		node_idx_t test_idx = eval_node(env, *it++);
		node_idx_t form_idx = *it++;
		list_ptr_t test_args = new_list();
		test_args->push_front_inplace(expr_idx);
		test_args->push_front_inplace(test_idx);
		test_args->push_front_inplace(pred_idx);
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

	node_idx_t coll_idx = eval_node(env, args->first_value());
	node_idx_t key_idx = eval_node(env, args->second_value());
	int coll_type = get_node_type(coll_idx);
	if(coll_type == NODE_LIST) {
		list_ptr_t coll = get_node_list(coll_idx);
		return coll->contains(key_idx) ? TRUE_NODE : FALSE_NODE;
	} else if(coll_type == NODE_VECTOR) {
		vector_ptr_t coll = get_node_vector(coll_idx);
		return coll->contains(key_idx) ? TRUE_NODE : FALSE_NODE;
	} else if(coll_type == NODE_MAP) {
		map_ptr_t coll = get_node_map(coll_idx);
		return coll->contains(key_idx, [env](node_idx_t a, node_idx_t b) {
			return node_eq(env, a, b);
		}) ? TRUE_NODE : FALSE_NODE;
	}
	warnf("(contains?) requires a collection\n");
	return NIL_NODE;
}

// (counted? coll)
// Returns true if coll implements count in constant time
static node_idx_t native_is_counted(env_ptr_t env, list_ptr_t args) {
	node_idx_t coll_idx = eval_node(env, args->first_value());
	int coll_type = get_node_type(coll_idx);
	if(coll_type == NODE_LIST) {
		return TRUE_NODE;
	} else if(coll_type == NODE_VECTOR) {
		return TRUE_NODE;
	} else if(coll_type == NODE_MAP) {
		return TRUE_NODE;
	}
	return FALSE_NODE;
}


#include "jo_lisp_math.h"
#include "jo_lisp_string.h"
#include "jo_lisp_system.h"
#include "jo_lisp_lazy.h"

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
#ifdef _MSC_VER
    if(argc == 1) {
		GetModuleFileNameA(GetModuleHandle(NULL), real_exe_path, MAX_PATH);
		bool register_clj = !IsRegistered("CLJ") && (MessageBoxA(0, "Do you want to register .CLJ files with this program?", "JO_LISP", MB_OKCANCEL) == 1);
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

	if(0) {
		// test persistent vectors
		jo_persistent_vector<int> *pv = new jo_persistent_vector<int>();
		for(int i = 0; i < 100; i++) { pv->push_back_inplace(i); }
		for(int i = 0; i < 33; i++) { pv->pop_front_inplace(); }
		for(int i = 0; i < 10; i++) { pv->pop_back_inplace(); }

		// test iterators
		jo_persistent_vector<int>::iterator it = pv->begin();
		for(int i = 0; it; it++, ++i) {
			if(*it != 33 + i) {
				fprintf(stderr, "iterator test failed\n");
				return 1;
			}
			//printf("%d\n", *it);
		}

		delete pv;

		printf("\n");
		// test persistent vectors

	}

	if(0) {
		// test bidirectional persistent vectors
		jo_persistent_vector_bidirectional<int> *pv = new jo_persistent_vector_bidirectional<int>();
		for(int i = 0; i < 10; i++) { pv->push_front_inplace(-i); }
		for(int i = 0; i < 1; i++) { pv->push_back_inplace(i); }
		//for(int i = 0; i < 1; i++) { pv->pop_front_inplace(); }
		for(int i = 0; i < 3; i++) { pv->pop_back_inplace(); }
		for(int i = 0; i < 3; i++) { pv->push_back_inplace(i); }
		//for(int i = 0; i < 1; i++) { pv->push_front_inplace(-i); }
		jo_persistent_vector_bidirectional<int>::iterator it = pv->begin();
		for(; it; it++) {
			printf("%d\n", *it);
		}
		delete pv;
		printf("\n");
		exit(0);
	}

	debugf("Setting up environment...\n");

	env_ptr_t env = new_env(NULL);

	// first thing first, alloc special nodes
	{
		get_node(new_node(NODE_NIL))->flags |= NODE_FLAG_LITERAL;
		for(int i = 0; i <= 256; i++) {
			node_idx_t nidx = new_node(NODE_INT);
			assert(nidx == INT_0_NODE+i);
			node_t *n = get_node(nidx);
			n->t_int = i;
			n->flags |= NODE_FLAG_LITERAL;
		}
		{
			node_idx_t i = new_node(NODE_BOOL);
			node_t *n = get_node(i);
			n->t_bool = false;
			n->flags |= NODE_FLAG_LITERAL;
		}
		{
			node_idx_t i = new_node(NODE_BOOL);
			node_t *n = get_node(i);
			n->t_bool = true;
			n->flags |= NODE_FLAG_LITERAL;
		}
		new_node_symbol("quote");
		new_node_symbol("unquote");
		new_node_symbol("unquote-splice");
		new_node_symbol("quasiquote");
		new_node_list(new_list());
		new_node_vector(new_vector());
		new_node_map(new_map());
		new_node_symbol("%");
		new_node_symbol("%1");
		new_node_symbol("%2");
		new_node_symbol("%3");
		new_node_symbol("%4");
		new_node_symbol("%5");
		new_node_symbol("%6");
		new_node_symbol("%7");
		new_node_symbol("%8");
		new_node_keyword("else");
		new_node_keyword("when");
		new_node_keyword("while");
		new_node_keyword("let");
	}

	env->set("nil", NIL_NODE);
	env->set("zero", ZERO_NODE);
	env->set("false", FALSE_NODE);
	env->set("true", TRUE_NODE);
	env->set("quote", new_node_native_function("quote", &native_quote, true));
	env->set("unquote", UNQUOTE_NODE);
	env->set("unquote-splice", UNQUOTE_SPLICE_NODE);
	env->set("quasiquote", new_node_native_function("quasiquote", &native_quasiquote, true));
	env->set("let", new_node_native_function("let", &native_let, true));
	env->set("eval", new_node_native_function("eval", &native_eval, false));
	env->set("print", new_node_native_function("print", &native_print, false));
	env->set("println", new_node_native_function("println", &native_println, false));
	env->set("=", new_node_native_function("=", &native_eq, false));
	env->set("not=", new_node_native_function("not=", &native_neq, false));
	env->set("<", new_node_native_function("<", &native_lt, false));
	env->set("<=", new_node_native_function("<=", &native_lte, false));
	env->set(">", new_node_native_function(">", &native_gt, false));
	env->set(">=", new_node_native_function(">=", &native_gte, false));
	env->set("and", new_node_native_function("and", &native_and, true));
	env->set("or", new_node_native_function("or", &native_or, true));
	env->set("not", new_node_native_function("not", &native_not, false));
	env->set("empty?", new_node_native_function("empty?", &native_is_empty, false));
	env->set("zero?", new_node_native_function("zero?", &native_is_zero, false));
	env->set("false?", new_node_native_function("false?", &native_is_false, false));
	env->set("true?", new_node_native_function("true?", &native_is_true, false));
	env->set("some?", new_node_native_function("some?", &native_is_some, false));
	env->set("letter?", new_node_native_function("letter?", &native_is_letter, false));
	env->set("do", new_node_native_function("do", &native_do, false));
	env->set("doall", new_node_native_function("doall", &native_doall, true));
	env->set("conj", new_node_native_function("conj", &native_conj, false));
	env->set("into", new_node_native_function("info", &native_into, false));
	env->set("pop", new_node_native_function("pop", &native_pop, false));
	env->set("peek", new_node_native_function("peek", &native_peek, false));
	env->set("first", new_node_native_function("first", &native_first, false));
	env->set("second", new_node_native_function("second", &native_second, false));
	env->set("last", new_node_native_function("last", &native_last, false));
	env->set("nth", new_node_native_function("nth", &native_nth, false));
	env->set("rand-nth", new_node_native_function("rand-nth", &native_rand_nth, false));
	env->set("ffirst", new_node_native_function("ffirst", &native_ffirst, false));
	env->set("next", new_node_native_function("next", &native_next, false));
	env->set("nnext", new_node_native_function("nnext", &native_nnext, false));
	env->set("fnext", new_node_native_function("fnext", &native_fnext, false));
	env->set("nfirst", new_node_native_function("nfirst", &native_nfirst, false));
	env->set("rest", new_node_native_function("rest", &native_rest, false));
	env->set("list", new_node_native_function("list", &native_list, false));
	env->set("hash-map", new_node_native_function("hash-map", &native_hash_map, false));
	env->set("upper-case", new_node_native_function("upper-case", &native_upper_case, false));
	env->set("var", new_node_native_function("var", &native_var, false));
	env->set("declare", new_node_native_function("declare", &native_declare, false));
	env->set("def", new_node_native_function("def", &native_def, true));
	env->set("defonce", new_node_native_function("defonce", &native_defonce, true));
	env->set("fn", new_node_native_function("fn", &native_fn, true));
	env->set("fn?", new_node_native_function("fn?", &native_is_fn, false));
	env->set("ifn?", new_node_native_function("ifn?", &native_is_ifn, false));
	env->set("ident?", new_node_native_function("ident?", &native_is_ident, false));
	env->set("indexed?", new_node_native_function("indexed?", &native_is_indexed, false));
	env->set("int?", new_node_native_function("int?", &native_is_int, false));
	env->set("int", new_node_native_function("int", &native_int, false));
	env->set("integer?", new_node_native_function("integer?", &native_is_int, false));
	env->set("defn", new_node_native_function("defn", &native_defn, true));
	env->set("defmacro", new_node_native_function("defmacro", &native_defmacro, true));
	env->set("*ns*", new_node_var("nil", NIL_NODE));
	env->set("if", new_node_native_function("if", &native_if, true));
	env->set("if-not", new_node_native_function("if-not", &native_if_not, true));
	env->set("if-let", new_node_native_function("if-let", &native_if_let, true));
	env->set("if-some", new_node_native_function("if-some", &native_if_some, true));
	env->set("when", new_node_native_function("when", &native_when, true));
	env->set("when-let", new_node_native_function("when-let", &native_when_let, true));
	env->set("when-not", new_node_native_function("when-not", &native_when_not, true));
	env->set("while", new_node_native_function("while", &native_while, true));
	env->set("cond", new_node_native_function("cond", &native_cond, true));
	env->set("condp", new_node_native_function("condp", &native_condp, true));
	env->set("case", new_node_native_function("case", &native_case, true));
	env->set("apply", new_node_native_function("apply", &native_apply, true));
	env->set("reduce", new_node_native_function("reduce", &native_reduce, true));
	env->set("delay", new_node_native_function("delay", &native_delay, true));
	env->set("delay?", new_node_native_function("delay?", &native_is_delay, false));
	env->set("constantly", new_node_native_function("constantly", &native_constantly, false));
	env->set("count", new_node_native_function("count", &native_count, false));
	env->set("dotimes", new_node_native_function("dotimes", &native_dotimes, true));
	env->set("doseq", new_node_native_function("doseq", &native_doseq, true));
	env->set("nil?", new_node_native_function("nil?", native_is_nil, false));
	env->set("time", new_node_native_function("time", &native_time, true));
	env->set("assoc", new_node_native_function("assoc", &native_assoc, false));
	env->set("dissoc", new_node_native_function("dissoc", &native_dissoc, false));
	env->set("get", new_node_native_function("get", &native_get, false));
	env->set("comp", new_node_native_function("comp", &native_comp, false));
	env->set("partial", new_node_native_function("partial", &native_partial, false));
	env->set("shuffle", new_node_native_function("shuffle", &native_shuffle, false));
	env->set("random-sample", new_node_native_function("random-sample", &native_random_sample, false));
	env->set("is", new_node_native_function("is", &native_is, true));
	env->set("assert", new_node_native_function("assert", &native_is, true));
	env->set("identity", new_node_native_function("identity", &native_identity, false));
	env->set("reverse", new_node_native_function("reverse", &native_reverse, false));
	env->set("nthrest", new_node_native_function("nthrest", &native_nthrest, false));
	env->set("nthnext", new_node_native_function("nthnext", &native_nthnext, false));
	env->set("->", new_node_native_function("->", &native_thread, true));
	env->set("->>", new_node_native_function("->>", &native_thread_last, true));
	env->set("as->", new_node_native_function("as->", &native_as_thread, true));
	env->set("cond->", new_node_native_function("cond->", &native_cond_thread, true));
	env->set("cond->>", new_node_native_function("cond->>", &native_cond_thread_last, true));
	env->set("every?", new_node_native_function("every?", &native_is_every, false));
	env->set("any?", new_node_native_function("any?", &native_is_any, false));
	env->set("array-map", new_node_native_function("array-map", &native_array_map, false));
	env->set("boolean", new_node_native_function("boolean", &native_boolean, false));
	env->set("boolean?", new_node_native_function("boolean?", &native_is_boolean, false));
	env->set("butlast", new_node_native_function("butlast", &native_butlast, false));
	env->set("complement", new_node_native_function("complement", &native_complement, false));
	env->set("contains?", new_node_native_function("contains?", &native_is_contains, false));
	env->set("counted?", new_node_native_function("counted?", &native_is_counted, false));

	jo_lisp_math_init(env);
	jo_lisp_string_init(env);
	jo_lisp_system_init(env);
	jo_lisp_lazy_init(env);
	
	FILE *fp = fopen(argv[1], "r");
	if(!fp) {
		return 0;
	}
	
	debugf("Parsing...\n");

	parse_state_t parse_state;
	parse_state.fp = fp;
	parse_state.line_num = 1;

	// parse the base list
	list_ptr_t main_list = new_list();
	for(node_idx_t next = parse_next(env, &parse_state, 0); next != INV_NODE; next = parse_next(env, &parse_state, 0)) {
		main_list->push_back_inplace(next);
	}
	fclose(fp);

	debugf("Evaluating...\n");

	node_idx_t res_idx = eval_node_list(env, main_list);
	print_node(res_idx, 0);
	printf("\n");

	debugf("nodes.size() = %zu\n", nodes.size());
	debugf("free_nodes.size() = %zu\n", free_nodes.size());

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
}

