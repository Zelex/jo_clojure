// TODO: arbitrary precision numbers... really? do I need to support this?
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string>
#include <map>
#include <unordered_map>
#include "debugbreak.h"
#include "jo_stdcpp.h"

//#define debugf printf
#define debugf sizeof

//#define warnf printf
#define warnf sizeof

static double time_program_start = jo_time();

enum {
	// hard coded nodes
	TMP_NODE = -2,
	INV_NODE = -1,
	NIL_NODE = 0,
	ZERO_NODE,
	INT_0_NODE = ZERO_NODE,
	INT_256_NODE = INT_0_NODE + 256,
	FALSE_NODE,
	TRUE_NODE,
	QUOTE_NODE,



	NODE_NIL = 0,
	NODE_BOOL,
	NODE_INT,
	NODE_FLOAT,
	NODE_STRING,
	NODE_SYMBOL,
	//NODE_REGEX,
	NODE_LIST,
	NODE_LAZY_LIST,
	NODE_VECTOR,
	NODE_SET,
	NODE_MAP,
	NODE_NATIVE_FUNCTION,
	NODE_FUNC,
	NODE_VAR,
	NODE_DELAY,

	NODE_FLAG_MACRO        = 1<<0,
	NODE_FLAG_STRING       = 1<<1, // string or symbol
	NODE_FLAG_LAZY         = 1<<2, // unused
	NODE_FLAG_LITERAL      = 1<<3,
	NODE_FLAG_LITERAL_ARGS = 1<<4,
};

struct env_t;
struct node_t;

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

typedef node_idx_t (*native_function_t)(env_ptr_t env, list_ptr_t args);

static list_ptr_t new_list() { return list_ptr_t(new list_t()); }
static vector_ptr_t new_vector() { return vector_ptr_t(new vector_t()); }
static map_ptr_t new_map() { return map_ptr_t(new map_t()); }

static inline node_t *get_node(node_idx_t idx);
static inline int get_node_type(node_idx_t idx);
static inline int get_node_flags(node_idx_t idx);
static inline jo_string get_node_string(node_idx_t idx);
static inline node_idx_t get_node_var(node_idx_t idx);
static inline bool get_node_bool(node_idx_t idx);
static inline int get_node_int(node_idx_t idx);
static inline float get_node_float(node_idx_t idx);

static node_idx_t new_node_var(const jo_string &name, node_idx_t value);

static bool node_eq(env_ptr_t env, node_idx_t n1i, node_idx_t n2i);
static bool node_lt(env_ptr_t env, node_idx_t n1i, node_idx_t n2i);
static bool node_lte(env_ptr_t env, node_idx_t n1i, node_idx_t n2i);

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
};

static env_ptr_t new_env(env_ptr_t parent) { return env_ptr_t(new env_t(parent)); }

struct node_t {
	int type;
	int flags;
	jo_string t_string;
	list_ptr_t t_list;
	vector_ptr_t t_vector;
	map_ptr_t t_map;
	struct {
		list_ptr_t args;
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
		native_function_t t_native_function;
	};

	bool is_symbol() const { return type == NODE_SYMBOL; }
	bool is_list() const { return type == NODE_LIST; }
	bool is_vector() const { return type == NODE_VECTOR; }
	bool is_map() const { return type == NODE_MAP; }
	bool is_lazy_list() const { return type == NODE_LAZY_LIST; }
	bool is_string() const { return type == NODE_STRING; }
	bool is_func() const { return type == NODE_FUNC; }
	bool is_macro() const { return flags & NODE_FLAG_MACRO;}

	bool is_seq() const { return is_list() || is_lazy_list() || is_map() || is_vector(); }

	list_ptr_t &as_list() { return t_list; }
	vector_ptr_t &as_vector() { return t_vector; }
	map_ptr_t &as_map() { return t_map; }

	bool as_bool() const {
		switch(type) {
			case NODE_BOOL:   return t_bool;
			case NODE_INT:    return t_int != 0;
			case NODE_FLOAT:  return t_float != 0.0;
			case NODE_SYMBOL:
			case NODE_STRING: return t_string.length() != 0;
			case NODE_LIST:
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
		case NODE_STRING: return atof(t_string.c_str());
		}
		return 0;
	}

	jo_string as_string() const {
		switch(type) {
		case NODE_BOOL:   return t_bool ? "true" : "false";
		case NODE_INT:    return va("%i", t_int);
		case NODE_FLOAT:  return va("%f", t_float);
		}
		return t_string;
	}

	jo_string type_as_string() const {
		switch(type) {
		case NODE_BOOL:   return "bool";
		case NODE_INT:    return "int";
		case NODE_FLOAT:  return "float";
		case NODE_STRING: return "string";
		case NODE_LIST:   return "list";
		case NODE_VECTOR: return "vector";
		case NODE_SET:    return "set";
		case NODE_MAP:    return "map";
		case NODE_NATIVE_FUNCTION: return "native_function";
		case NODE_VAR:	  return "var";
		case NODE_SYMBOL: return "symbol";
		}
		return "unknown";		
	}
};

static jo_vector<node_t> nodes;
static jo_vector<node_idx_t> free_nodes; // available for allocation...

static void print_node(node_idx_t node, int depth = 0, bool same_line=false);
static void print_node_type(node_idx_t node);
static void print_node_list(list_ptr_t nodes, int depth = 0);
static void print_node_vector(vector_ptr_t nodes, int depth = 0);
static void print_node_map(map_ptr_t nodes, int depth = 0);

static inline node_t *get_node(node_idx_t idx) {
	return &nodes[idx];
}

static inline int get_node_type(node_idx_t idx) {
	return get_node(idx)->type;
}

static inline int get_node_flags(node_idx_t idx) {
	return get_node(idx)->flags;
}

static inline jo_string get_node_string(node_idx_t idx) {
	return get_node(idx)->as_string();
}

static inline node_idx_t get_node_var(node_idx_t idx) {
	return get_node(idx)->t_var;
}

static inline bool get_node_bool(node_idx_t idx) {
	return get_node(idx)->as_bool();
}

static inline int get_node_int(node_idx_t idx) {
	return get_node(idx)->as_int();
}

static inline float get_node_float(node_idx_t idx) {
	return get_node(idx)->as_float();
}

static inline jo_string get_node_type_string(node_idx_t idx) {
	return get_node(idx)->type_as_string();
}

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
	node_t n = {type};
	return new_node(&n);
}

static node_idx_t new_node_list(list_ptr_t nodes, int flags = 0) {
	node_idx_t idx = new_node(NODE_LIST);
	node_t *n = get_node(idx);
	n->t_list = nodes;
	n->flags |= flags;
	return idx;
}

static node_idx_t new_node_map(map_ptr_t nodes, int flags = 0) {
	node_idx_t idx = new_node(NODE_MAP);
	node_t *n = get_node(idx);
	n->t_map = nodes;
	n->flags |= flags;
	return idx;
}

static node_idx_t new_node_vector(vector_ptr_t nodes, int flags = 0) {
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

static node_idx_t new_node_native_function(native_function_t f, bool is_macro) {
	node_t n = {NODE_NATIVE_FUNCTION};
	n.t_native_function = f;
	n.flags |= is_macro ? NODE_FLAG_MACRO : 0;
	n.flags |= NODE_FLAG_LITERAL;
	return new_node(&n);
}

static node_idx_t new_node_native_function(const char *name, native_function_t f, bool is_macro) {
	node_t n = {NODE_NATIVE_FUNCTION};
	n.t_native_function = f;
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
	node_t n = {NODE_INT};
	n.t_int = i;
	n.flags |= NODE_FLAG_LITERAL;
	return new_node(&n);
}

static node_idx_t new_node_float(double f) {
	node_t n = {NODE_FLOAT};
	n.t_float = f;
	n.flags |= NODE_FLAG_LITERAL;
	return new_node(&n);
}

static node_idx_t new_node_string(const jo_string &s) {
	node_t n = {NODE_STRING};
	n.t_string = s;
	n.flags |= NODE_FLAG_LITERAL | NODE_FLAG_STRING;
	return new_node(&n);
}

static node_idx_t new_node_symbol(const jo_string &s) {
	node_t n = {NODE_SYMBOL};
	n.t_string = s;
	n.flags |= NODE_FLAG_STRING;
	return new_node(&n);
}

static node_idx_t new_node_var(const jo_string &name, node_idx_t value) {
	node_t n = {NODE_VAR};
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

static node_idx_t parse_next(env_ptr_t env, parse_state_t *state, int stop_on_sep) {
	token_t tok = get_token(state);
	debugf("parse_next \"%s\", with '%c'\n", tok.str.c_str(), stop_on_sep);

	if(tok.type == TOK_EOF) {
		// end of list
		return NIL_NODE;
	}

	const char *tok_ptr = tok.str.c_str();
	const char *tok_ptr_end = tok_ptr + tok.str.size();
	int c = tok_ptr[0];
	int c2 = tok_ptr[1];

	if(c == stop_on_sep) {
		// end of list
		return NIL_NODE;
	}

	if(tok.type == TOK_SEPARATOR && c != stop_on_sep && (c == ')' || c == ']' || c == '}')) {
		fprintf(stderr, "unexpected separator '%c', was expecting '%c' on line %i\n", c, stop_on_sep, tok.line);
		return NIL_NODE;
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
	if(is_num(c)) {
		bool is_int = true;
		int int_val = 0;
		double float_val = 0.0;
		// 0x hexadecimal
		if(c == '0' && (c2 == 'x' || c2 == 'X')) {
			tok_ptr += 2;
			// parse hex from tok_ptr
			while(is_alnum(*tok_ptr)) {
				int_val <<= 4;
				int_val += *tok_ptr - '0';
				if(*tok_ptr >= 'a' && *tok_ptr <= 'f') {
					int_val -= 'a' - '0' - 10;
				}
				if(*tok_ptr >= 'A' && *tok_ptr <= 'F') {
					int_val -= 'A' - '0' - 10;
				}
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
			is_int = true;
			// determine if float or int
			if(tok.str.find('.') != jo_string_npos) {
				is_int = false;
				float_val = atof(tok_ptr);
			}
			else {
				int_val = atoi(tok_ptr);
			}
		}
		// Create a new number node
		if(is_int) {
			debugf("int: %d\n", int_val);
			return new_node_int(int_val);
		}
		debugf("float: %f\n", float_val);
		return new_node_float(float_val);
	} 
	if(tok.type == TOK_SYMBOL) {
		debugf("symbol: %s\n", tok.str.c_str());
		if(env->has(tok.str.c_str())) {
			//debugf("pre-resolve symbol: %s\n", tok.str.c_str());
			return env->get(tok.str.c_str());
		}
		return new_node_symbol(tok.str.c_str());
	} 

	// parse quote shorthand
	if(tok.type == TOK_SEPARATOR && tok.str == "quote") {
		debugf("list begin\n");
		node_t n = {NODE_LIST};
		n.t_list = new_list();
		//n.flags |= NODE_FLAG_LITERAL;
		//n.flags |= NODE_FLAG_LITERAL_ARGS;
		n.t_list->push_back_inplace(env->get("quote"));
		node_idx_t next = parse_next(env, state, ')');
		while(next != NIL_NODE) {
			n.t_list->push_back_inplace(next);
			next = parse_next(env, state, ')');
		}
		debugf("list end\n");
		return new_node(&n);
	}

	// parse list
	if(c == '(') {
		debugf("list begin\n");
		node_t n = {NODE_LIST};
		n.t_list = new_list();
		int common_flags = ~0;
		node_idx_t next = parse_next(env, state, ')');
		bool is_native_fn = get_node_type(next) == NODE_NATIVE_FUNCTION;
		while(next != NIL_NODE) {
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
		node_t n = {NODE_LIST};
		n.t_list = new_list(); // TODO: actually vector pls
		int common_flags = ~0;
		node_idx_t next = parse_next(env, state, ']');
		bool is_native_fn = get_node_type(next) == NODE_NATIVE_FUNCTION;
		while(next != NIL_NODE) {
			common_flags &= get_node_flags(next);
			n.t_list->push_back_inplace(next);
			next = parse_next(env, state, ']');
		}
		if(common_flags & NODE_FLAG_LITERAL) {
			if(is_native_fn) {
				n.flags |= NODE_FLAG_LITERAL_ARGS;
			} else {
				n.flags |= NODE_FLAG_LITERAL;
			}
		}
		// @ If its all the same type, convert to simple array of values of type
		debugf("vector end\n");
		return new_node(&n);
	}

	// parse map (hashmap)
	// like a list, but with keys and values alternating
	if(c == '{') {
		debugf("map begin\n");
		node_t n = {NODE_MAP};
		n.t_map = new_map();
		node_idx_t next = parse_next(env, state, '}');
		node_idx_t next2 = next != NIL_NODE ? parse_next(env, state, '}') : NIL_NODE;
		while(next != NIL_NODE && next2 != NIL_NODE) {
			n.t_map->assoc_inplace(next, next2, [env](const node_idx_t &a, const node_idx_t &b) {
				return node_eq(env, a, b);
			});
			next = parse_next(env, state, '}');
			next2 = next != NIL_NODE ? parse_next(env, state, '}') : NIL_NODE;
		}
		debugf("map end\n");
		return new_node(&n);
	}

	// parse set

	return NIL_NODE;
}

static node_idx_t eval_node(env_ptr_t env, node_idx_t root);
static node_idx_t eval_node_list(env_ptr_t env, list_ptr_t list);

// eval a list of nodes
static node_idx_t eval_list(env_ptr_t env, list_ptr_t list, int list_flags=0) {
	list_t::iterator it = list->begin();
	node_idx_t n1i = *it++;
	int n1_type = get_node_type(n1i);
	int n1_flags = get_node_flags(n1i);
	if(n1_type == NODE_LIST 
	|| n1_type == NODE_SYMBOL 
	|| n1_type == NODE_STRING 
	|| n1_type == NODE_NATIVE_FUNCTION
	|| n1_type == NODE_MAP
	) {
		node_idx_t sym_idx = n1i;
		int sym_type = n1_type;
		int sym_flags = n1_flags;
		if(n1_type == NODE_LIST) {
			sym_idx = eval_list(env, get_node(n1i)->t_list);
			sym_type = get_node_type(sym_idx);
		} else if((n1_flags & NODE_FLAG_STRING) == NODE_FLAG_STRING) {
			sym_idx = env->get(get_node_string(n1i));
			sym_type = get_node_type(sym_idx);
		}
		sym_flags = get_node(sym_idx)->flags;

		// get the symbol's value
		if(sym_type == NODE_NATIVE_FUNCTION) {
			if((sym_flags|list_flags) & (NODE_FLAG_MACRO|NODE_FLAG_LITERAL_ARGS)) {
				return get_node(sym_idx)->t_native_function(env, list->rest());
			}

			list_ptr_t args = new_list();
			for(; it; it++) {
				args->push_back_inplace(eval_node(env, *it));
			}
			// call the function
			return get_node(sym_idx)->t_native_function(env, args);
		} else if(sym_type == NODE_FUNC || sym_type == NODE_DELAY) {
			list_ptr_t proto_args = get_node(sym_idx)->t_func.args;
			list_ptr_t proto_body = get_node(sym_idx)->t_func.body;
			env_ptr_t proto_env = get_node(sym_idx)->t_func.env;
			env_ptr_t fn_env = new_env(proto_env);
			list_ptr_t args1(list->rest());

			if(sym_type == NODE_DELAY && get_node(sym_idx)->t_delay != INV_NODE) {
				return get_node(sym_idx)->t_delay;
			}

			if(proto_args.ptr) {
				// For each argument in arg_symbols, 
				// grab the corresponding argument in args1
				// and evaluate it
				// and create a new_node_var 
				// and insert it to the head of env
				for(list_t::iterator i = proto_args->begin(), i2 = args1->begin(); i && i2; i++, i2++) {
					node_idx_t arg_value = eval_node(env, *i2);
					fn_env->set_temp(get_node_string(*i), arg_value);
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
		}
	}
	return n1i;
}

static node_idx_t eval_node(env_ptr_t env, node_idx_t root) {
	node_idx_t n1_idx, n2_idx;
	node_t *n, *n1, *n2;
	node_idx_t i, n1i, n2i;

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
		return eval_node(env, sym_idx);
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
			printf(",");
		}
		printf(")");
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
			printf(",");
		}
		printf("}");
	} else if(type == NODE_SYMBOL) {
		printf("%s", get_node_string(node).c_str());
	} else if(type == NODE_STRING) {
		printf("\"%s\"", get_node_string(node).c_str());
	} else if(type == NODE_NATIVE_FUNCTION) {
		printf("<%s>", get_node_string(node).c_str());
	} else if(type == NODE_FUNC) {
		printf("<function>");
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
		printf("<unknown>");
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
	} else if(n->type == NODE_INT) {
		printf("%*s%d\n", depth, "", n->t_int);
	} else if(n->type == NODE_FLOAT) {
		printf("%*s%f\n", depth, "", n->t_float);
	} else if(n->type == NODE_BOOL) {
		printf("%*s%s\n", depth, "", n->t_bool ? "true" : "false");
	} else if(n->type == NODE_NATIVE_FUNCTION) {
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
	for(list_t::iterator i = nodes->begin(); i; i++) {
		print_node(*i, depth);
	}
}

static void print_node_type(node_idx_t i) {
	printf("%s", get_node(i)->type_as_string().c_str());
}

struct lazy_list_iterator_t {
	env_ptr_t env;
	node_idx_t cur;
	node_idx_t val;
	jo_vector<node_idx_t> next_list;
	int next_idx;

	lazy_list_iterator_t(env_ptr_t env, node_idx_t node_idx) : env(env), cur(node_idx), val(NIL_NODE), next_list(), next_idx() {
		if(get_node(cur)->is_lazy_list()) {
			cur = eval_node(env, get_node(cur)->t_lazy_fn);
			if(!done()) {
				val = get_node(cur)->as_list()->first_value();
			}
		}
	}

	bool done() const {
		return next_idx >= next_list.size() && !get_node(cur)->is_list();
	}

	void next() {
		if(next_idx < next_list.size()) {
			val = next_list[next_idx++];
			return;
		}
		if(done()) {
			return;
		}
		cur = eval_list(env, get_node(cur)->as_list()->rest());
		if(!done()) {
			val = get_node(cur)->as_list()->first_value();
		} else {
			val = NIL_NODE;
		}
	}

	node_idx_t next_fn() {
		if(done()) {
			return NIL_NODE;
		}
		return new_node_list(get_node(cur)->as_list()->rest());
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

	list_ptr_t all() {
		list_ptr_t res = new_list();
		while(!done()) {
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
		if(type == NODE_LIST) {
			it = get_node(node_idx)->as_list()->begin();
			if(!done()) {
				val = *it++;
			}
		} else if(type == NODE_VECTOR) {
			vit = get_node(node_idx)->as_vector()->begin();
			if(!done()) {
				val = *vit++;
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

	void next() {
		if(done()) {
			return;
		}
		if(type == NODE_LIST) {
			val = *it++;
		} else if(type == NODE_VECTOR) {
			val = *vit++;
		} else if(type == NODE_MAP) {
			mit++;
			if(!done()) {
				val = mit->second;
			}
		} else if(type == NODE_LAZY_LIST) {
			lit.next();
			val = lit.val;
		}
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
		res->push_back_inplace(val);
		while(!done()) {
			next();
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
	node_t *n1 = get_node(n1i);
	node_t *n2 = get_node(n2i);
	if(n1->type == NODE_NIL || n2->type == NODE_NIL) {
		return n1->type == NODE_NIL && n2->type == NODE_NIL;
	} else if(n1->is_seq() && n2->is_seq()) {
		// in this case we want to iterate over the sequences and compare
		// each element
		seq_iterator_t i1(env, n1i), i2(env, n2i);
		for(; i1 && i2; i1.next(), i2.next()) {
			if(!node_eq(env, i1.val, i2.val)) {
				return false;
			}
		}
		return true;
	} else if(n1->type == NODE_BOOL && n2->type == NODE_BOOL) {
		return n1->t_bool == n2->t_bool;
	} else if(n1->type == NODE_INT && n2->type == NODE_INT) {
		return n1->t_int == n2->t_int;
	} else if(n1->type == NODE_FLOAT || n2->type == NODE_FLOAT) {
		return n1->as_float() == n2->as_float();
	} else if((n1->flags&n2->flags&NODE_FLAG_STRING) == NODE_FLAG_STRING) {
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
		for(seq_iterator_t i(NULL, n); i; i.next()) {
			res = (res * 31) + jo_hash_value(i.val);
		}
		return res;
	} else if(n1->type == NODE_BOOL) {
		return n1->t_bool ? 1 : 0;
	} else if(n1->type == NODE_STRING || n1->type == NODE_SYMBOL) {
		return jo_hash_value(n1->t_string.c_str());
	} else if(n1->type == NODE_INT) {
		return n1->t_int;
	} else if(n1->type == NODE_FLOAT) {
		return jo_hash_value(n1->as_float());
	}
	return 0;
}


// native function to add any number of arguments
static node_idx_t native_add(env_ptr_t env, list_ptr_t args) { 
	int i = 0;
	double d = 0.0;
	for(list_t::iterator it = args->begin(); it; it++) {
		node_t *n = get_node(*it);
		if(n->type == NODE_INT) {
			i += n->t_int;
		} else if(n->type == NODE_FLOAT) {
			d += n->t_float;
		} else {
			d += n->as_float();
		}
	}
	return d == 0.0 ? new_node_int(i) : new_node_float(d+i);
}

// subtract any number of arguments from the first argument
static node_idx_t native_sub(env_ptr_t env, list_ptr_t args) {
	int i_sum = 0;
	double d_sum = 0.0;

	size_t size = args->size();
	if(size == 0) {
		return ZERO_NODE;
	}

	// Special case. 1 argument return the negative of that argument
	if(size == 1) {
		node_t *n = get_node(*args->begin());
		if(n->type == NODE_INT) {
			return new_node_int(-n->t_int);
		}
		return new_node_float(-n->as_float());
	}

	list_t::iterator i = args->begin();
	node_t *n = get_node(*i++);
	if(n->type == NODE_INT) {
		i_sum = n->t_int;
	} else {
		d_sum = n->as_float();
	}

	for(; i; i++) {
		n = get_node(*i);
		if(n->type == NODE_INT) {
			i_sum -= n->t_int;
		} else {
			d_sum -= n->as_float();
		}
	}
	return d_sum == 0.0 ? new_node_int(i_sum) : new_node_float(d_sum + i_sum);
}

static node_idx_t native_mul(env_ptr_t env, list_ptr_t args) {
	int i = 1;
	double d = 1.0;

	if(args->size() == 0) {
		return ZERO_NODE; // TODO: common enough should be a static constant
	}

	for(list_t::iterator it = args->begin(); it; it++) {
		node_t *n = get_node(*it);
		int type = n->type;
		if(type == NODE_INT) {
			i *= n->t_int;
		} else if(type == NODE_FLOAT) {
			d *= n->t_float;
		} else {
			d *= n->as_float();
		}
	}

	return d == 1.0 ? new_node_int(i) : new_node_float(d * i);
}

// divide any number of arguments from the first argument
static node_idx_t native_div(env_ptr_t env, list_ptr_t args) {
	int i_sum = 1;
	double d_sum = 1.0;
	bool is_int = true;

	size_t size = args->size();
	if(size == 0) {
		return NIL_NODE;
	}

	// special case of 1 argument, compute 1.0 / value
	if(size == 1) {
		node_t *n = get_node(*args->begin());
		return new_node_float(1.0 / n->as_float());
	}

	list_t::iterator i = args->begin();
	node_t *n = get_node(*i++);
	if(n->type == NODE_INT) {
		i_sum = n->t_int;
	} else {
		d_sum = n->as_float();
		is_int = false;
	}

	for(; i; i++) {
		n = get_node(*i);
		if(n->type == NODE_INT) {
			i_sum /= n->t_int;
			d_sum = i_sum;
		} else {
			d_sum /= n->as_float();
			is_int = false;
		}
	}

	return is_int ? new_node_int(i_sum) : new_node_float(d_sum);
}

// modulo the first argument by the second
static node_idx_t native_mod(env_ptr_t env, list_ptr_t args) {
	int i_sum = 0;
	double d_sum = 0.0;

	if(args->size() == 0) {
		return NIL_NODE;
	}

	list_t::iterator i = args->begin();
	node_t *n = get_node(*i++);
	if(n->type == NODE_INT) {
		i_sum = n->t_int % get_node(*i)->as_int();
		return new_node_int(i_sum);
	}
	d_sum = fmod(n->as_float(), get_node(*i)->as_float());
	return new_node_float(d_sum);
}

// Tests the equality between two or more objects
static node_idx_t native_eq(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return NIL_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = node_eq(env, n1, n2);
	for(; i && ret; i++) {
		ret = ret && node_eq(env, n1, *i);
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
		return NIL_NODE;
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
		return NIL_NODE;
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
		return NIL_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = node_lte(env, n1, n2);
	for(; i && ret; i++) {
		ret = ret && node_lte(env, n2, *i);
		n2 = *i;
	}
	return !ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_gte(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 2) {
		return NIL_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t n1 = *i++, n2 = *i++;
	bool ret = node_lt(env, n1, n2);
	for(; i && ret; i++) {
		ret = ret && node_lt(env, n2, *i);
		n2 = *i;
	}
	return !ret ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_if(env_ptr_t env, list_ptr_t args) {
	if(args->size() != 3) {
		return NIL_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t cond = eval_node(env, *i++);
	node_idx_t when_true = *i++, when_false = *i++;
	return eval_node(env, get_node(cond)->as_bool() ? when_true : when_false);
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
	node_idx_t ret = NIL_NODE;
	for(list_t::iterator i = args->begin(); i; i++) {
		ret = eval_node(env, *i);
	}
	return ret;
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
		for(seq_iterator_t it(env, coll); it && n; it.next(), n--) {
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

static node_idx_t native_fn(env_ptr_t env, list_ptr_t args) {
	node_idx_t reti = new_node(NODE_FUNC);
	node_t *ret = get_node(reti);
	ret->t_func.args = get_node(args->first_value())->t_list;
	ret->t_func.body = args->rest();
	ret->t_func.env = env;
	return reti;
}

/*
(defn functionname
   “optional documentation string”
   [arguments]
   (code block))
*/
static node_idx_t native_defn(env_ptr_t env, list_ptr_t args) {
	list_t::iterator i = args->begin();
	node_idx_t sym_node_idx = *i++;
	jo_string sym_node = get_node(sym_node_idx)->as_string();
	node_idx_t doc_string = *i++; // ignored for eval purposes if present
	node_idx_t arg_list;
	if(get_node_type(doc_string) != NODE_STRING) {
		arg_list = doc_string;
	} else {
		arg_list = *i++;
	}

	if(get_node_type(sym_node_idx) != NODE_SYMBOL) {
		fprintf(stderr, "defn: expected symbol");
		return NIL_NODE;
	}

	node_idx_t reti = new_node(NODE_FUNC);
	node_t *ret = get_node(reti);
	ret->t_func.args = get_node(arg_list)->t_list;
	ret->t_func.body = args->rest();
	ret->t_func.env = env;
	env->set(sym_node, reti);
	return NIL_NODE;
}

static node_idx_t native_is_nil(env_ptr_t env, list_ptr_t args) {
	return args->first_value() == NIL_NODE ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_inc(env_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->first_value());
	if(n1->type == NODE_INT) {
		return new_node_int(n1->t_int + 1);
	}
	return new_node_float(n1->as_float() + 1.0f);
}

static node_idx_t native_dec(env_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->first_value());
	if(n1->type == NODE_INT) {
		return new_node_int(n1->t_int - 1);
	}
	return new_node_float(n1->as_float() - 1.0f);
}

static node_idx_t native_rand_int(env_ptr_t env, list_ptr_t args) {
	return new_node_int(rand() % get_node(args->first_value())->as_int());
}

static node_idx_t native_rand_float(env_ptr_t env, list_ptr_t args) {
	return new_node_float(rand() / (float)RAND_MAX);
}

// This statement takes a set of test/expression pairs. 
// It evaluates each test one at a time. 
// If a test returns logical true, ‘cond’ evaluates and returns the value of the corresponding expression 
// and doesn't evaluate any of the other tests or expressions. ‘cond’ returns nil.
static node_idx_t native_cond(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	while(it) {
		node_idx_t test = eval_node(env, *it++), expr = *it++;
		node_t *n = get_node(test);
		if(n->as_bool()) {
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

// returns current time since program start in seconds
static node_idx_t native_time_now(env_ptr_t env, list_ptr_t args) {
	return new_node_float(jo_time() - time_program_start);
}

static node_idx_t native_dotimes(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t binding_idx = *it++;
	if(!get_node(binding_idx)->is_list()) {
		return NIL_NODE;
	}
	node_t *binding = get_node(binding_idx);
	list_ptr_t binding_list = binding->as_list();
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
   (doseq (n (0 1 2))
   (println n)))
(Example)

The ‘doseq’ statement is similar to the ‘for each’ statement which is found in many other programming languages. 
The doseq statement is basically used to iterate over a sequence.
*/
static node_idx_t native_doseq(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t binding_idx = *it++;
	if(!get_node(binding_idx)->is_list()) {
		return NIL_NODE;
	}
	node_t *binding = get_node(binding_idx);
	list_ptr_t binding_list = binding->as_list();
	if (binding_list->size() != 2) {
		return NIL_NODE;
	}
	node_idx_t name_idx = binding_list->first_value();
	node_idx_t value_idx = binding_list->nth(1);
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
	node_idx_t cond_idx = eval_node(env, *it++);
	node_t *cond = get_node(cond_idx);
	node_idx_t ret = NIL_NODE;
	if(cond->as_bool()) {
		for(; it; it++) {
			ret = eval_node(env, *it);
		}
	}
	return ret;
}

// (cons x seq)
// Returns a new seq where x is the first element and seq is the rest.
static node_idx_t native_cons(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t first_idx = *it++;
	node_idx_t second_idx = *it++;
	node_t *first = get_node(first_idx);
	node_t *second = get_node(second_idx);
	if(second->type == NODE_LIST) {
		list_ptr_t second_list = second->as_list();
		return new_node_list(second_list->cons(first_idx));
	}
	if(second->type == NODE_LAZY_LIST) {
		lazy_list_iterator_t lit(env, second_idx);
		list_ptr_t second_list = lit.all();
		return new_node_list(second_list->cons(first_idx));
	}
	list_ptr_t ret = new_list();
	ret->cons(second_idx);
	ret->cons(first_idx);
	return new_node_list(ret);
}

static node_idx_t native_conj(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t first_idx = *it++;
	node_idx_t second_idx = *it++;
	node_t *first = get_node(first_idx);
	node_t *second = get_node(second_idx);
	if(first->type == NODE_LIST) {
		list_ptr_t first_list = first->as_list();
		return new_node_list(first_list->cons(second_idx));
	}
	list_ptr_t ret = new_list();
	ret->cons(second_idx);
	ret->cons(first_idx);
	return new_node_list(ret);
}

static node_idx_t native_pop(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	if(!list->is_list()) {
		return NIL_NODE;
	}
	list_ptr_t list_list = list->as_list();
	if(list_list->size() == 0) {
		return NIL_NODE;
	}
	return new_node_list(list_list->pop());
}

static node_idx_t native_peek(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	if(!list->is_list()) {
		return NIL_NODE;
	}
	list_ptr_t list_list = list->as_list();
	if(list_list->size() == 0) {
		return NIL_NODE;
	}
	return list_list->first_value();
}

static node_idx_t native_constantly(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t reti = new_node(NODE_DELAY);
	node_t *ret = get_node(reti);
	ret->t_delay = args->first_value(); // always return this value
	return reti;
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
	return ZERO_NODE;
}

static node_idx_t native_is_delay(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return node->type == NODE_DELAY ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_is_empty(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		return list_list->size() == 0 ? TRUE_NODE : FALSE_NODE;
	}
	return FALSE_NODE;
}

static node_idx_t native_is_false(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return !node->as_bool() ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_is_true(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return node->as_bool() ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_first(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		if(list_list->size() == 0) {
			return NIL_NODE;
		}
		return list_list->first_value();
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
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		if(list_list->size() <= 1) {
			return NIL_NODE;
		}
		return list_list->nth(1);
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
	if(node->is_list()) {
		return node->as_list()->last_value();
	}
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, node_idx);
		for(; !lit.done(); lit.next()) {}
		return lit.val;
	}
	return NIL_NODE;
}

static node_idx_t native_drop(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	list_ptr_t list_list = list->as_list();
	node_idx_t n_idx = *it++;
	int n = get_node(n_idx)->as_int();
	if(list->is_list()) {
		return new_node_list(list_list->drop(n));
	}
	if(list->is_lazy_list()) {
		lazy_list_iterator_t lit(env, list_idx);
		lit.nth(n);
		if(lit.done()) {
			return NIL_NODE;
		}
		return new_node_lazy_list(lit.next_fn());
	}
	return NIL_NODE;
}

static node_idx_t native_nth(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	node_idx_t n_idx = *it++;
	int n = get_node(n_idx)->as_int();
	if(list->is_list()) {
		return list->as_list()->nth(n);
	}
	if(list->is_lazy_list()) {
		lazy_list_iterator_t lit(env, list_idx);
		return lit.nth(n);
	}
	return NIL_NODE;
}

// equivalent to (first (first x))
static node_idx_t native_ffirst(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	node_idx_t first_idx = NIL_NODE;
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		if(list_list->size() == 0) {
			return NIL_NODE;
		}
		first_idx = list_list->first_value();
	}
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, node_idx);
		first_idx = lit.val;
	}
	node_t *first = get_node(first_idx);
	if(first->is_list()) {
		list_ptr_t first_list = first->as_list();
		if(first_list->size() == 0) {
			return NIL_NODE;
		}
		return first_list->first_value();
	}
	if(first->is_lazy_list()) {
		lazy_list_iterator_t lit(env, first_idx);
		return lit.val;
	}
	return NIL_NODE;
}

static node_idx_t native_is_fn(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return node->type == NODE_FUNC ? TRUE_NODE : FALSE_NODE;
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
	if(node->is_lazy_list()) {
		lazy_list_iterator_t lit(env, node_idx);
		if(lit.done()) {
			return NIL_NODE;
		}
		return new_node_lazy_list(lit.next_fn());
	}
	return NIL_NODE;
}

// like next, but always returns a list
static node_idx_t native_rest(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		return new_node_list(list_list->rest());
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

static node_idx_t native_unless(env_ptr_t env, list_ptr_t args) {
	node_idx_t node_idx = eval_node(env, args->first_value());
	node_t *node = get_node(node_idx);
	if(!node->as_bool()) {
		return eval_node_list(env, args->rest());
	}
	return NIL_NODE;
}

// (let (a 1 b 2) (+ a b))
static node_idx_t native_let(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(!node->is_list()) {
		printf("let: expected list\n");
		return NIL_NODE;
	}
	list_ptr_t list_list = node->as_list();
	if(list_list->size() % 2 != 0) {
		printf("let: expected even number of elements\n");
		return NIL_NODE;
	}
	env_ptr_t env2 = new_env(env);
	for(list_t::iterator i = list_list->begin(); i;) {
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
				list_ptr_t arg_list = new_list();
				arg_list->push_back_inplace(f_idx);
				return eval_list(env, arg_list);
			}
			list_t::iterator it2 = list_list->begin();
			node_idx_t reti = *it2++;
			while(it2) {
				node_idx_t arg_idx = *it2++;
				list_ptr_t arg_list = new_list();
				arg_list->push_back_inplace(f_idx);
				arg_list->push_back_inplace(reti);
				arg_list->push_back_inplace(arg_idx);
				reti = eval_list(env, arg_list);
			}
			return reti;
		}
		if(coll->is_lazy_list()) {
			lazy_list_iterator_t lit(env, coll_idx);
			node_idx_t reti = lit.val;
			for(; !lit.done(); lit.next()) {
				node_idx_t arg_idx = lit.val;
				list_ptr_t arg_list = new_list();
				arg_list->push_back_inplace(f_idx);
				arg_list->push_back_inplace(reti);
				arg_list->push_back_inplace(arg_idx);
				reti = eval_list(env, arg_list);
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
				node_idx_t arg_idx = *it2++;
				list_ptr_t arg_list = new_list();
				arg_list->push_back_inplace(f_idx);
				arg_list->push_back_inplace(reti);
				arg_list->push_back_inplace(arg_idx);
				reti = eval_list(env, arg_list);
			}
			return reti;
		}
		if(coll_node->is_lazy_list()) {
			lazy_list_iterator_t lit(env, coll);
			for(; !lit.done(); lit.next()) {
				node_idx_t arg_idx = lit.val;
				list_ptr_t arg_list = new_list();
				arg_list->push_back_inplace(f_idx);
				arg_list->push_back_inplace(reti);
				arg_list->push_back_inplace(arg_idx);
				reti = eval_list(env, arg_list);
			}
			return reti;
		}
		warnf("reduce: expected list or lazy list\n");
		return NIL_NODE;
	}
	return NIL_NODE;
}

static node_idx_t native_take_last(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	int n = node->as_int();
	node_idx_t coll_idx = *it++;
	node_t *coll = get_node(coll_idx);
	if(coll->is_list()) {
		list_ptr_t list_list = coll->as_list();
		return new_node_list(list_list->subvec(list_list->size() - n, list_list->size()));
	}
	return NIL_NODE;
}

// eval each arg in turn, return if any eval to false
static node_idx_t native_and(env_ptr_t env, list_ptr_t args) {
	for(list_t::iterator it = args->begin(); it; it++) {
		node_idx_t node_idx = eval_node(env, *it);
		node_t *node = get_node(node_idx);
		if(!node->as_bool()) {
			return FALSE_NODE;
		}
	}
	return TRUE_NODE;
}

static node_idx_t native_or(env_ptr_t env, list_ptr_t args) {
	for(list_t::iterator it = args->begin(); it; it++) {
		node_idx_t node_idx = eval_node(env, *it);
		node_t *node = get_node(node_idx);
		if(node->as_bool()) {
			return TRUE_NODE;
		}
	}
	return FALSE_NODE;
}

static node_idx_t native_not(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = eval_node(env, *it++);
	node_t *node = get_node(node_idx);
	return !node->as_bool() ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_bit_and(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	int n = node->as_int();
	for(; it; it++) {
		node_idx_t node_idx = eval_node(env, *it);
		node_t *node = get_node(node_idx);
		n &= node->as_int();
	}
	return new_node_int(n);
}

static node_idx_t native_bit_or(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	int n = node->as_int();
	for(; it; it++) {
		node_idx_t node_idx = eval_node(env, *it);
		node_t *node = get_node(node_idx);
		n |= node->as_int();
	}
	return new_node_int(n);
}

static node_idx_t native_bit_xor(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	int n = node->as_int();
	for(; it; it++) {
		node_idx_t node_idx = eval_node(env, *it);
		node_t *node = get_node(node_idx);
		n ^= node->as_int();
	}
	return new_node_int(n);
}

static node_idx_t native_bit_not(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = eval_node(env, *it++);
	node_t *node = get_node(node_idx);
	return new_node_int(~node->as_int());
}

static node_idx_t native_reverse(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_t *node = get_node(*it++);
	// if its a string
	if(node->is_string()) {
		return new_node_string(node->as_string().reverse());
	}
	// if its a list
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		return new_node_list(list_list->reverse());
	}
	return NIL_NODE;
}


static node_idx_t native_eval(env_ptr_t env, list_ptr_t args) {
	return eval_node(env, args->first_value());
}

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
	list_ptr_t ret;
	if(get_node_type(to) == NODE_LIST) {
		ret = get_node(to)->t_list->clone();
	} else if(get_node_type(to) == NODE_LAZY_LIST) {
		ret = new_list();
		for(lazy_list_iterator_t lit(env, to); !lit.done(); lit.next()) {
			ret->push_back_inplace(lit.val);
		}
	}
	if(get_node_type(from) == NODE_LIST) {
		ret->conj_inplace(*get_node(from)->t_list);
	} else if(get_node_type(from) == NODE_LAZY_LIST) {
		for(lazy_list_iterator_t lit(env, from); !lit.done(); lit.next()) {
			ret->push_back_inplace(lit.val);
		}
	}
	return new_node_list(ret);
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
	node_t *val_node = get_node(val_idx);
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
    {
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
	}

	env->set("nil", NIL_NODE);
	env->set("zero", ZERO_NODE);
	env->set("false", FALSE_NODE);
	env->set("true", TRUE_NODE);
	env->set("quote", new_node_native_function("quote", &native_quote, true));
	env->set("let", new_node_native_function("let", &native_let, true));
	env->set("eval", new_node_native_function("eval", &native_eval, false));
	env->set("print", new_node_native_function("print", &native_print, false));
	env->set("println", new_node_native_function("println", &native_println, false));
	env->set("+", new_node_native_function("+", &native_add, false));
	env->set("-", new_node_native_function("-", &native_sub, false));
	env->set("*", new_node_native_function("*", &native_mul, false));
	env->set("/", new_node_native_function("/", &native_div, false));
	env->set("mod", new_node_native_function("mod", &native_mod, false));
	env->set("inc", new_node_native_function("inc", &native_inc, false));
	env->set("dec", new_node_native_function("dec", &native_dec, false));
	env->set("=", new_node_native_function("=", &native_eq, false));
	env->set("!=", new_node_native_function("!=", &native_neq, false));
	env->set("not=", new_node_native_function("not=", &native_neq, false));
	env->set("<", new_node_native_function("lt", &native_lt, false));
	env->set("<=", new_node_native_function("lte", &native_lte, false));
	env->set(">", new_node_native_function("gt", &native_gt, false));
	env->set(">=", new_node_native_function("gte", &native_gte, false));
	env->set("and", new_node_native_function("and", &native_and, true));
	env->set("or", new_node_native_function("or", &native_or, true));
	env->set("not", new_node_native_function("not", &native_not, true));
	env->set("bit-and", new_node_native_function("bit-and", &native_bit_and, true));
	env->set("bit-or", new_node_native_function("bit-or", &native_bit_or, true));
	env->set("bit-xor", new_node_native_function("bit-xor", &native_bit_xor, true));
	env->set("bit-not", new_node_native_function("bit-not", &native_bit_not, true));
	env->set("empty?", new_node_native_function("empty?", &native_is_empty, false));
	env->set("false?", new_node_native_function("false?", &native_is_false, false));
	env->set("true?", new_node_native_function("true?", &native_is_true, false));
	env->set("do", new_node_native_function("do", &native_do, false));
	env->set("doall", new_node_native_function("doall", &native_doall, true));
	env->set("cons", new_node_native_function("cons", &native_cons, false));
	env->set("conj", new_node_native_function("conj", &native_conj, false));
	env->set("into", new_node_native_function("info", &native_into, false));
	env->set("pop", new_node_native_function("pop", &native_pop, false));
	env->set("peek", new_node_native_function("peek", &native_peek, false));
	env->set("first", new_node_native_function("first", &native_first, false));
	env->set("second", new_node_native_function("second", &native_second, false));
	env->set("last", new_node_native_function("last", &native_last, false));
	env->set("drop", new_node_native_function("drop", &native_drop, false));
	env->set("nth", new_node_native_function("nth", &native_nth, false));
	env->set("ffirst", new_node_native_function("ffirst", &native_ffirst, false));
	env->set("next", new_node_native_function("next", &native_next, false));
	env->set("rest", new_node_native_function("rest", &native_rest, false));
	env->set("list", new_node_native_function("list", &native_list, false));
	env->set("hash-map", new_node_native_function("hash-map", &native_hash_map, false));
	env->set("take-last", new_node_native_function("take-last", &native_take_last, false));
	env->set("upper-case", new_node_native_function("upper-case", &native_upper_case, false));
	env->set("concat", new_node_native_function("concat", &native_concat, false));
	env->set("var", new_node_native_function("var", &native_var, false));
	env->set("def", new_node_native_function("def", &native_def, true));
	env->set("fn", new_node_native_function("fn", &native_fn, true));
	env->set("fn?", new_node_native_function("fn?", &native_is_fn, false));
	env->set("defn", new_node_native_function("defn", &native_defn, true));
	env->set("*ns*", new_node_var("nil", NIL_NODE));
	env->set("if", new_node_native_function("if", &native_if, true));
	env->set("unless", new_node_native_function("unless", &native_unless, true));
	env->set("when", new_node_native_function("when", &native_when, true));
	env->set("while", new_node_native_function("while", &native_while, true));
	env->set("cond", new_node_native_function("cond", &native_cond, true));
	env->set("case", new_node_native_function("case", &native_case, true));
	env->set("apply", new_node_native_function("apply", &native_apply, true));
	env->set("reduce", new_node_native_function("reduce", &native_reduce, true));
	env->set("delay", new_node_native_function("delay", &native_delay, true));
	env->set("delay?", new_node_native_function("is_delay", &native_is_delay, false));
	env->set("constantly", new_node_native_function("constantly", &native_constantly, false));
	env->set("count", new_node_native_function("count", &native_count, false));
	env->set("dotimes", new_node_native_function("dotimes", &native_dotimes, true));
	env->set("doseq", new_node_native_function("doseq", &native_doseq, true));
	env->set("nil?", new_node_native_function("nil?", native_is_nil, false));
	env->set("rand-int", new_node_native_function("rand-int", &native_rand_int, false));
	env->set("rand-float", new_node_native_function("rand-float", &native_rand_float, false));
	env->set("Time/now", new_node_native_function("Time/now", &native_time_now, false));
	env->set("time", new_node_native_function("time", &native_time, true));
	env->set("assoc", new_node_native_function("assoc", &native_assoc, false));
	env->set("get", new_node_native_function("get", &native_get, false));

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
	for(node_idx_t next = parse_next(env, &parse_state, 0); next != NIL_NODE; next = parse_next(env, &parse_state, 0)) {
		main_list->push_back_inplace(next);
	}
	fclose(fp);

	debugf("Evaluating...\n");

	node_idx_t res_idx = eval_node_list(env, main_list);
	print_node(res_idx, 0);

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

