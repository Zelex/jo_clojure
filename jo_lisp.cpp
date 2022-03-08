// TODO: arbitrary precision numbers... really? do I need to support this?
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "debugbreak.h"
#include "jo_stdcpp.h"

//#define debugf printf
#define debugf sizeof

enum {
	INV_NODE = -1,
	NIL_NODE = 0,

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

	NODE_FLAG_MACRO    = 1<<0,
	NODE_FLAG_INFINITE = 1<<1,
	NODE_FLAG_LAZY     = 1<<2,
};

typedef int node_idx_t;
typedef jo_string sym_t;
typedef jo_persistent_list<node_idx_t> list_t; // TODO: make node_t
typedef jo_shared_ptr<list_t> list_ptr_t;

typedef node_idx_t (*native_function_t)(list_ptr_t env, list_ptr_t args);

list_ptr_t new_list() { return list_ptr_t(new list_t()); }

struct node_t {
	int type;
	int flags;
	jo_string t_string;
	list_ptr_t t_list;
	struct {
		list_ptr_t args;
		list_ptr_t body;
		list_ptr_t env;
	} t_func;
	union {
		node_idx_t t_var; // link to the variable
		bool t_bool;
		// most implementations combine these as "number", but at the moment that sounds silly
		int t_int;
		double t_float;
		node_idx_t t_vector; // TODO: vectors of PODs or ??? should be hard coded types
		node_idx_t t_map;
		node_idx_t t_set;
		node_idx_t t_delay; // cached result
		node_idx_t t_lazy_fn;
		native_function_t t_native_function;
	};

	bool is_symbol() const { return type == NODE_SYMBOL; }
	bool is_list() const { return type == NODE_LIST; }
	bool is_lazy_list() const { return type == NODE_LAZY_LIST; }
	bool is_vector() const { return type == NODE_VECTOR; }
	bool is_string() const { return type == NODE_STRING; }
	bool is_func() const { return type == NODE_FUNC; }
	bool is_macro() const { return flags & NODE_FLAG_MACRO;}
	bool is_infinite() const { return flags & NODE_FLAG_INFINITE; }

	list_ptr_t &as_list() {
		return t_list;
	}

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
		case NODE_FUNC:
		case NODE_VAR:
		case NODE_SYMBOL:
		case NODE_STRING: return t_string;
		}
		return jo_string();
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

	// TODO: coerce types?
	bool is_eq(const node_t *other) const {
		if(type != other->type) return false;
		switch(type) {
		case NODE_BOOL:   return t_bool == other->t_bool;
		case NODE_INT:    return t_int == other->t_int;
		case NODE_FLOAT:  return t_float == other->t_float;
		case NODE_STRING: return t_string == other->t_string;
		case NODE_NATIVE_FUNCTION: return t_native_function == other->t_native_function;
		case NODE_FUNC:
		case NODE_VAR:
		case NODE_SYMBOL: return t_string == other->t_string;
		}
		return false;
	}

	bool is_neq(const node_t *other) const {
		return !is_eq(other);
	}

	bool is_lt(const node_t *other) const {
		if(type != other->type) return false;
		switch(type) {
		case NODE_BOOL:   return t_bool < other->t_bool;
		case NODE_INT:    return t_int < other->t_int;
		case NODE_FLOAT:  return t_float < other->t_float;
		case NODE_STRING: return t_string < other->t_string;
		case NODE_NATIVE_FUNCTION: return t_native_function < other->t_native_function;
		case NODE_FUNC:
		case NODE_VAR:
		case NODE_SYMBOL: return t_string < other->t_string;
		}
		return false;
	}

	bool is_lte(const node_t *other) const {
		if(type != other->type) return false;
		switch(type) {
		case NODE_BOOL:   return t_bool <= other->t_bool;
		case NODE_INT:    return t_int <= other->t_int;
		case NODE_FLOAT:  return t_float <= other->t_float;
		case NODE_STRING: return t_string <= other->t_string;
		case NODE_NATIVE_FUNCTION: return t_native_function <= other->t_native_function;
		case NODE_FUNC:
		case NODE_VAR:
		case NODE_SYMBOL: return t_string <= other->t_string;
		}
		return false;
	}

	bool is_gt(const node_t *other) const {
		return !is_lte(other);
	}

	bool is_gte(const node_t *other) const {
		return !is_lt(other);
	}
};

jo_vector<node_t> nodes;
jo_vector<node_idx_t> free_nodes; // available for allocation...

void print_node(node_idx_t node, int depth = 0);
void print_node_list(list_ptr_t nodes, int depth = 0);

inline node_t *get_node(node_idx_t idx) {
	return &nodes[idx];
}

inline int get_node_type(node_idx_t idx) {
	return get_node(idx)->type;
}

inline jo_string get_node_string(node_idx_t idx) {
	return get_node(idx)->as_string();
}

inline jo_string get_node_type_string(node_idx_t idx) {
	return get_node(idx)->type_as_string();
}

inline node_idx_t alloc_node() {
	if (free_nodes.size()) {
		node_idx_t idx = free_nodes.back();
		free_nodes.pop_back();
		return idx;
	}
	node_idx_t idx = nodes.size();
	nodes.push_back(node_t());
	return idx;
}

inline void free_node(node_idx_t idx) {
	free_nodes.push_back(idx);
}

// TODO: Should prefer to allocate nodes next to existing nodes which will be linked (for cache coherence)
inline node_idx_t new_node(const node_t *n) {
	if(free_nodes.size()) {
		int ni = free_nodes.pop_back();
		nodes[ni] = *n;
		return ni;
	}
	nodes.push_back(*n);
	return nodes.size()-1;
}

node_idx_t new_node(int type) {
	node_t n = {type};
	return new_node(&n);
}

node_idx_t new_node_list(list_ptr_t nodes) {
	node_idx_t idx = new_node(NODE_LIST);
	get_node(idx)->t_list = nodes;
	return idx;
}

node_idx_t new_node_lazy_list(node_idx_t lazy_fn) {
	node_idx_t idx = new_node(NODE_LAZY_LIST);
	get_node(idx)->t_lazy_fn = lazy_fn;
	get_node(idx)->flags |= NODE_FLAG_LAZY;
	return idx;
}

node_idx_t new_node_native_function(native_function_t f, bool is_macro) {
	node_t n = {NODE_NATIVE_FUNCTION};
	n.t_native_function = f;
	n.flags |= is_macro ? NODE_FLAG_MACRO : 0;
	return new_node(&n);
}

node_idx_t new_node_bool(bool b) {
	node_t n = {NODE_BOOL};
	n.t_bool = b;
	return new_node(&n);
}

node_idx_t new_node_int(int i) {
	node_t n = {NODE_INT};
	n.t_int = i;
	return new_node(&n);
}

node_idx_t new_node_float(double f) {
	node_t n = {NODE_FLOAT};
	n.t_float = f;
	return new_node(&n);
}

node_idx_t new_node_string(const jo_string &s) {
	node_t n = {NODE_STRING};
	n.t_string = s;
	return new_node(&n);
}

node_idx_t new_node_symbol(const jo_string &s) {
	node_t n = {NODE_SYMBOL};
	n.t_string = s;
	return new_node(&n);
}

node_idx_t new_node_var(const jo_string &name, node_idx_t value) {
	node_t n = {NODE_VAR};
	n.t_string = name;
	n.t_var = value;
	return new_node(&n);
}

node_idx_t clone_node(node_idx_t idx) {
	node_t *n = &nodes[idx];
	node_idx_t new_idx = new_node(n->type);
	node_t *new_n = &nodes[new_idx];
	new_n->flags = n->flags;
	new_n->t_string = n->t_string;
	new_n->t_list = n->t_list;
	switch(n->type) {
	case NODE_BOOL:   new_n->t_bool = n->t_bool; break;
	case NODE_INT:    new_n->t_int = n->t_int; break;
	case NODE_FLOAT:  new_n->t_float = n->t_float; break;
	case NODE_FUNC:   new_n->t_func = n->t_func; break;
	case NODE_VAR:    new_n->t_var = n->t_var; break;
	case NODE_VECTOR: new_n->t_vector = n->t_vector; break;
	case NODE_SET:    new_n->t_set = n->t_set; break;
	case NODE_MAP:    new_n->t_map = n->t_map; break;
	case NODE_NATIVE_FUNCTION: new_n->t_native_function = n->t_native_function; break;
	}
	return new_idx;
}

node_idx_t env_find(list_ptr_t env, const jo_string &name) {
	for(list_t::iterator it = env->begin(); it; ++it) {
		node_idx_t idx = *it;
		if(get_node_string(idx) == name) {
			return idx;
		}
	}
	return NIL_NODE;
}

node_idx_t env_get(list_ptr_t env, const jo_string &name) {
	node_idx_t idx = env_find(env, name);
	if(idx == NIL_NODE) {
		return NIL_NODE;
	}
	return get_node(idx)->t_var;
}

bool env_has(list_ptr_t env, const jo_string &name) {
	return env_find(env, name) != NIL_NODE;
}

// TODO: should this be destructive? or persistent?
list_ptr_t env_set(list_ptr_t env, const jo_string &name, node_idx_t value) {
	node_idx_t idx = env_find(env, name);
	if(idx == NIL_NODE) {
		env->cons_inplace(new_node_var(name, value));
		return env;
	}
	get_node(idx)->t_var = value;
	return env;
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

token_t get_token(parse_state_t *state) {
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
		tok.type = TOK_STRING;
		// string literal of a symbol
		do {
			int C = state->getc();
			if(is_whitespace(C) || is_separator(C) || C == EOF) {
				state->ungetc(C);
				break;
			}
			tok.str += C;
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

node_idx_t parse_next(parse_state_t *state, int stop_on_sep) {
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
		return new_node_symbol(tok.str.c_str());
	} 

	// parse list
	if(c == '(') {
		debugf("list begin\n");
		node_t n = {NODE_LIST};
		n.t_list = new_list();
		node_idx_t next = parse_next(state, ')');
		while(next != NIL_NODE) {
			n.t_list->push_back_inplace(next);
			next = parse_next(state, ')');
		}
		debugf("list end\n");
		return new_node(&n);
	}

	// parse map

	// parse set

	return NIL_NODE;
}

node_idx_t eval_node(list_ptr_t env, node_idx_t root);
node_idx_t eval_node_list(list_ptr_t env, list_ptr_t list);

// eval a list of nodes
node_idx_t eval_list(list_ptr_t env, list_ptr_t list) {
	list_t::iterator it = list->begin();
	node_idx_t n1i = *it++;
	int n1_type = get_node_type(n1i);
	if(n1_type == NODE_LIST || n1_type == NODE_SYMBOL) {
		node_idx_t sym_idx;
		if(n1_type == NODE_LIST) {
			sym_idx = eval_list(env, get_node(n1i)->t_list);
		} else {
			sym_idx = env_get(env, get_node(n1i)->t_string);
		}
		int sym_type = get_node_type(sym_idx);
		auto sym_flags = get_node(sym_idx)->flags;

		// get the symbol's value
		if(sym_type == NODE_NATIVE_FUNCTION) {
			if(sym_flags & NODE_FLAG_MACRO) {
				list_ptr_t args1(list->rest());
				return get_node(sym_idx)->t_native_function(env, args1);
			}

			list_ptr_t args = new_list();
			for(; it; it++) {
				args->push_back_inplace(eval_node(env, *it));
			}

			// call the function
			return get_node(sym_idx)->t_native_function(env, args);
		} else if(sym_type == NODE_FUNC || sym_type == NODE_DELAY) {
			list_ptr_t &proto_args = get_node(sym_idx)->t_func.args;
			list_ptr_t &proto_body = get_node(sym_idx)->t_func.body;
			list_ptr_t &proto_env = get_node(sym_idx)->t_func.env;
			list_ptr_t fn_env = proto_env->conj(*env);
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
					node_t *arg_sym = get_node(*i);
					node_idx_t arg_value = eval_node(env, *i2);
					node_idx_t var = new_node_var(arg_sym->t_string, arg_value);
					fn_env = fn_env->cons(var);
				}
			}
			
			//print_node_list(fn_env, 0);
			
			// Evaluate all statements in the body list
			node_idx_t last = NIL_NODE;
			for(list_t::iterator i = proto_body->begin(); i; i++) {
				last = eval_node(fn_env, *i);
			}

			if(sym_type == NODE_DELAY) {
				get_node(sym_idx)->t_delay = last;
			}
			return last;
		}
	}

	return NIL_NODE;
}

node_idx_t eval_node(list_ptr_t env, node_idx_t root) {
	node_idx_t n1_idx, n2_idx;
	node_t *n, *n1, *n2;
	node_idx_t i, n1i, n2i;

	if(root == NIL_NODE) {
		return NIL_NODE;
	}

	n = get_node(root);
	if(n->type == NODE_LIST) {
		return eval_list(env, n->t_list);
	} else if(n->type == NODE_SYMBOL) {
		node_idx_t sym_idx = env_find(env, n->t_string);
		if(sym_idx == NIL_NODE) {
			return root;
		}
		return eval_node(env, get_node(sym_idx)->t_var);
	}
	return root;
}

node_idx_t eval_node_list(list_ptr_t env, list_ptr_t list) {
	node_idx_t res = NIL_NODE;
	for(list_t::iterator it = list->begin(); it; it++) {
		res = eval_node(env, *it);
	}
	return res;
}

// Print the node heirarchy
void print_node(node_idx_t node, int depth) {
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
		printf("%*s\"%s\"\n", depth, "", n->t_string.c_str());
	} else if(n->type == NODE_SYMBOL) {
		printf("%*s%s\n", depth, "", n->t_string.c_str());
	} else if(n->type == NODE_INT) {
		printf("%*s%d\n", depth, "", n->t_int);
	} else if(n->type == NODE_FLOAT) {
		printf("%*s%f\n", depth, "", n->t_float);
	} else if(n->type == NODE_BOOL) {
		printf("%*s%s\n", depth, "", n->t_bool ? "true" : "false");
	} else if(n->type == NODE_NATIVE_FUNCTION) {
		printf("%*s<native>\n", depth, "");
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
		printf("%*s%s = ", depth, "", n->t_string.c_str());
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
}

void print_node_list(list_ptr_t nodes, int depth) {
	for(list_t::iterator i = nodes->begin(); i; i++) {
		print_node(*i, depth);
	}
}

// native function to add any number of arguments
node_idx_t native_add(list_ptr_t env, list_ptr_t args) { 
	int i_sum = 0;
	double d_sum = 0.0;
	bool is_int = true;

	for(list_t::iterator i = args->begin(); i; i++) {
		node_t *n = get_node(*i);
		if(n->type == NODE_INT) {
			i_sum += n->t_int;
		} else {
			d_sum += n->as_float();
			is_int = false;
		}
	}

	if(is_int) {
		return new_node_int(i_sum);
	}
	return new_node_float(d_sum + i_sum);
}

// subtract any number of arguments from the first argument
node_idx_t native_sub(list_ptr_t env, list_ptr_t args) {
	int i_sum = 0;
	double d_sum = 0.0;
	bool is_int = true;

	size_t size = args->size();
	if(size == 0) {
		return NIL_NODE;
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
		is_int = false;
	}

	for(; i; i++) {
		n = get_node(*i);
		if(n->type == NODE_INT) {
			i_sum -= n->t_int;
		} else {
			d_sum -= n->as_float();
			is_int = false;
		}
	}

	if(is_int) {
		return new_node_int(i_sum);
	}
	return new_node_float(d_sum + i_sum);
}

node_idx_t native_mul(list_ptr_t env, list_ptr_t args) {
	int i_sum = 1;
	double d_sum = 1.0;
	bool is_int = true;

	if(args->size() == 0) {
		return new_node_int(0); // TODO: common enough should be a static constant
	}

	for(list_t::iterator i = args->begin(); i; i++) {
		node_t *n = get_node(*i);
		if(n->type == NODE_INT) {
			i_sum *= n->t_int;
		} else {
			d_sum *= n->as_float();
			is_int = false;
		}
	}

	if(is_int) {
		return new_node_int(i_sum);
	}
	return new_node_float(d_sum * i_sum);
}

// divide any number of arguments from the first argument
node_idx_t native_div(list_ptr_t env, list_ptr_t args) {
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
		} else {
			d_sum /= n->as_float();
			is_int = false;
		}
	}

	if(is_int) {
		return new_node_int(i_sum);
	}
	return new_node_float(d_sum / i_sum);
}

// modulo the first argument by the second
node_idx_t native_mod(list_ptr_t env, list_ptr_t args) {
	int i_sum = 0;
	double d_sum = 0.0;
	bool is_int = true;

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

// compares the first argument with all other arguments 
node_idx_t native_eq(list_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) {
		return NIL_NODE;
	}

	list_t::iterator i = args->begin();
	node_t *n = get_node(*i++);
	for(; i; i++) {
		if(!n->is_eq(get_node(*i))) {
			return new_node_bool(false);
		}
	}
	return new_node_bool(true);
}

node_idx_t native_neq(list_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) {
		return NIL_NODE;
	}

	list_t::iterator i = args->begin();
	node_t *n = get_node(*i++);
	for(; i; i++) {
		if(!n->is_neq(get_node(*i))) {
			return new_node_bool(false);
		}
	}
	return new_node_bool(true);
}

node_idx_t native_lt(list_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) {
		return NIL_NODE;
	}

	list_t::iterator i = args->begin();
	node_t *n = get_node(*i++);
	for(; i; i++) {
		node_t *n2 = get_node(*i);
		if(!n->is_lt(n2)) {
			return new_node_bool(false);
		}
		n = n2;
	}
	return new_node_bool(true);
}

node_idx_t native_lte(list_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) {
		return NIL_NODE;
	}

	list_t::iterator i = args->begin();
	node_t *n = get_node(*i++);
	for(; i; i++) {
		node_t *n2 = get_node(*i);
		if(!n->is_lte(n2)) {
			return new_node_bool(false);
		}
		n = n2;
	}
	return new_node_bool(true);
}

node_idx_t native_gt(list_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) {
		return NIL_NODE;
	}

	list_t::iterator i = args->begin();
	node_t *n = get_node(*i++);
	for(; i; i++) {
		node_t *n2 = get_node(*i);
		if(!n->is_gt(n2)) {
			return new_node_bool(false);
		}
		n = n2;
	}
	return new_node_bool(true);
}

node_idx_t native_gte(list_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) {
		return NIL_NODE;
	}

	list_t::iterator i = args->begin();
	node_t *n = get_node(*i++);
	for(; i; i++) {
		node_t *n2 = get_node(*i);
		if(!n->is_gte(n2)) {
			return new_node_bool(false);
		}
		n = n2;
	}
	return new_node_bool(true);
}

node_idx_t native_if(list_ptr_t env, list_ptr_t args) {
	if(args->size() != 3) {
		return NIL_NODE;
	}

	list_t::iterator i = args->begin();
	node_idx_t cond = eval_node(env, *i++);
	node_idx_t args2 = *i++;
	node_idx_t args3 = *i++;
	return eval_node(env, get_node(cond)->as_bool() ? args2 : args3);
}

node_idx_t native_print(list_ptr_t env, list_ptr_t args) {
	list_t::iterator i = args->begin();
	for(; i; i++) {
		printf("%s", get_node(*i)->as_string().c_str());
	}
	return NIL_NODE;
}

node_idx_t native_println(list_ptr_t env, list_ptr_t args) {
	native_print(env, args);
	printf("\n");
	return NIL_NODE;	
}

node_idx_t native_do(list_ptr_t env, list_ptr_t args) {
	node_idx_t ret = NIL_NODE;
	for(list_t::iterator i = args->begin(); i; i++) {
		ret = eval_node(env, *i);
	}
	return ret;
}

node_idx_t native_while(list_ptr_t env, list_ptr_t args) {
	list_t::iterator i = args->begin();
	node_idx_t cond_idx = *i++;
	node_idx_t cond = eval_node(env, cond_idx);
	node_idx_t ret = NIL_NODE;
	while(get_node(cond)->as_bool()) {
		list_t::iterator j = i;
		for(; j; j++) {
			ret = eval_node(env, *j);
		}
		cond = eval_node(env, cond_idx);
	}
	return ret;
}

node_idx_t native_quote(list_ptr_t env, list_ptr_t args) {
	return new_node_list(args);
}

node_idx_t native_list(list_ptr_t env, list_ptr_t args) {
	return new_node_list(args);
}

/*
Usage: (var symbol)
The symbol must resolve to a var, and the Var object
itself (not its value) is returned. The reader macro #'x
expands to (var x).
*/
node_idx_t native_var(list_ptr_t env, list_ptr_t args) {
	list_t::iterator i = args->begin();
	node_t *n = get_node(*i++);
	if(env_has(env, n->as_string())) {
		return env_find(env, n->as_string());
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

node_idx_t native_def(list_ptr_t env, list_ptr_t args) {
	if(args->size() < 1 || args->size() > 3) {
		return NIL_NODE;
	}
	list_t::iterator i = args->begin();
	node_idx_t sym_node_idx = *i++;
	node_t *sym_node = get_node(sym_node_idx);
	node_idx_t init = NIL_NODE;
	if(i) {
		node_idx_t doc_string = *i++; // ignored for eval purposes if present
		if(args->size() < 3 || get_node_type(doc_string) != NODE_STRING) {
			init = doc_string;
		} else if(i) {
			init = *i++;
		}
	}

	if(sym_node->type != NODE_SYMBOL) {
		fprintf(stderr, "def: expected symbol\n");
		print_node(sym_node_idx);
		return NIL_NODE;
	}

	env_set(env, sym_node->as_string(), eval_node(env, init));
	return NIL_NODE;
}

node_idx_t native_fn(list_ptr_t env, list_ptr_t args) {
	node_idx_t reti = new_node(NODE_FUNC);
	node_t *ret = get_node(reti);
	ret->t_func.args = get_node(args->nth(0))->t_list;
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
node_idx_t native_defn(list_ptr_t env, list_ptr_t args) {
	list_t::iterator i = args->begin();
	node_t *sym_node = get_node(*i++);
	node_idx_t doc_string = *i++; // ignored for eval purposes if present
	node_idx_t arg_list;
	if(get_node_type(doc_string) != NODE_STRING) {
		arg_list = doc_string;
	} else {
		arg_list = *i++;
	}

	if(sym_node->type != NODE_SYMBOL) {
		fprintf(stderr, "defn: expected symbol");
		return NIL_NODE;
	}

	node_idx_t reti = new_node(NODE_FUNC);
	node_t *ret = get_node(reti);
	ret->t_func.args = get_node(arg_list)->t_list;
	ret->t_func.body = args->rest();
	ret->t_func.env = env;
	env_set(env, sym_node->as_string(), reti);
	return NIL_NODE;
}

node_idx_t native_is_nil(list_ptr_t env, list_ptr_t args) {
	return new_node_bool(args->nth(0) == NIL_NODE);
}

node_idx_t native_inc(list_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->nth(0));
	if(n1->type == NODE_INT) {
		return new_node_int(n1->t_int + 1);
	}
	return new_node_float(n1->as_float() + 1.0f);
}

node_idx_t native_dec(list_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->nth(0));
	if(n1->type == NODE_INT) {
		return new_node_int(n1->t_int - 1);
	}
	return new_node_float(n1->as_float() - 1.0f);
}

node_idx_t native_rand_int(list_ptr_t env, list_ptr_t args) {
	return new_node_int(rand() % get_node(args->nth(0))->as_int());
}

node_idx_t native_rand_float(list_ptr_t env, list_ptr_t args) {
	return new_node_float(rand() / (float)RAND_MAX);
}

// This statement takes a set of test/expression pairs. 
// It evaluates each test one at a time. 
// If a test returns logical true, ‘cond’ evaluates and returns the value of the corresponding expression 
// and doesn't evaluate any of the other tests or expressions. ‘cond’ returns nil.
node_idx_t native_cond(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	while(it) {
		node_idx_t test = *it++;
		node_idx_t expr = *it++;
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
node_idx_t native_case(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t cond_idx = eval_node(env, *it++);
	node_t *cond = get_node(cond_idx);
	node_idx_t next = *it++;
	while(it) {
		node_idx_t body_idx = *it++;
		node_idx_t value_idx = eval_node(env, next);
		node_t *value = get_node(value_idx);
		if(value->is_eq(cond)) {
			return eval_node(env, body_idx);
		}
		next = *it++;
	}
	return eval_node(env, next);
}

// wall time in seconds since program start
float time_now() {
	return (float)clock() / CLOCKS_PER_SEC; // TODO: not correct when multi-threaded!
}

// returns current time since program start in seconds
node_idx_t native_time_now(list_ptr_t env, list_ptr_t args) {
	return new_node_float(time_now());
}

node_idx_t native_dotimes(list_ptr_t env, list_ptr_t args) {
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
	node_idx_t name_idx = binding_list->nth(0);
	node_idx_t value_idx = eval_node(env, binding_list->nth(1));
	node_t *name = get_node(name_idx);
	node_t *value = get_node(value_idx);
	int times = value->as_int();
	for(int i = 0; i < times; ++i) {
		list_ptr_t new_env = env->cons(new_node_var(name->as_string(), new_node_int(i)));
		for(list_t::iterator it2 = it; it2; it2++) { 
			eval_node(new_env, *it2);
		}
	}
	return NIL_NODE;
}

/*
(defn Example []
   (doseq (n (0 1 2))
   (println n)))
(Example)

The ‘doseq’ statement is similar to the ‘for each’ statement which is found in many other programming languages. 
The doseq statement is basically used to iterate over a sequence.
*/
node_idx_t native_doseq(list_ptr_t env, list_ptr_t args) {
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
	node_idx_t name_idx = binding_list->nth(0);
	node_idx_t value_idx = binding_list->nth(1);
	node_t *name = get_node(name_idx);
	node_t *value = get_node(value_idx);
	list_ptr_t value_list = value->as_list();
	node_idx_t ret = NIL_NODE;
	for(list_t::iterator it2 = value_list->begin(); it2; it2++) {
		node_idx_t item_idx = *it2;
		list_ptr_t new_env = env->cons(new_node_var(name->as_string(), item_idx));
		for(list_t::iterator it3 = it; it3; it3++) { 
			ret = eval_node(new_env, *it3);
		}
	}
	return ret;
}

node_idx_t native_delay(list_ptr_t env, list_ptr_t args) {
	node_idx_t reti = new_node(NODE_DELAY);
	node_t *ret = get_node(reti);
	//ret->t_func.args  // no args for delays...
	ret->t_func.body = args;
	ret->t_func.env = env;
	ret->t_delay = INV_NODE;
	return reti;
}

node_idx_t native_when(list_ptr_t env, list_ptr_t args) {
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
node_idx_t native_cons(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t first_idx = *it++;
	node_idx_t second_idx = *it++;
	node_t *first = get_node(first_idx);
	node_t *second = get_node(second_idx);
	if(second->type == NODE_LIST) {
		list_ptr_t second_list = second->as_list();
		return new_node_list(second_list->cons(first_idx));
	}
	list_ptr_t ret = new_list();
	ret->cons(second_idx);
	ret->cons(first_idx);
	return new_node_list(ret);
}

node_idx_t native_conj(list_ptr_t env, list_ptr_t args) {
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

node_idx_t native_pop(list_ptr_t env, list_ptr_t args) {
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

node_idx_t native_peek(list_ptr_t env, list_ptr_t args) {
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
	return list_list->nth(0);
}

node_idx_t native_constantly(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t reti = new_node(NODE_DELAY);
	node_t *ret = get_node(reti);
	ret->t_delay = args->nth(0); // always return this value
	return reti;
}

node_idx_t native_count(list_ptr_t env, list_ptr_t args) {
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
	return new_node_int(0);
}

node_idx_t native_is_delay(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return new_node_bool(node->type == NODE_DELAY);
}

node_idx_t native_is_empty(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		return new_node_bool(list_list->size() == 0);
	}
	return new_node_bool(false);
}

node_idx_t native_is_false(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return new_node_bool(!node->as_bool());
}

node_idx_t native_is_true(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return new_node_bool(node->as_bool());
}

node_idx_t native_first(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		if(list_list->size() == 0) {
			return NIL_NODE;
		}
		return list_list->nth(0);
	}
	if(node->is_lazy_list()) {
		// call the t_lazy_fn, and grab the first element of the return and return that.
		node_idx_t reti = eval_node(env, node->t_lazy_fn);
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			if(list_list->size() == 0) {
				return NIL_NODE;
			}
			return list_list->nth(0);
		}
		return NIL_NODE;
	}
	return NIL_NODE;
}

node_idx_t native_second(list_ptr_t env, list_ptr_t args) {
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
		// call the t_lazy_fn, and grab the first element of the return and return that.
		node_idx_t reti = eval_node(env, node->t_lazy_fn);
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			if(list_list->size() == 0) {
				return NIL_NODE;
			}
			node_idx_t val_node = eval_list(env, list_list->rest());
			if(get_node(val_node)->is_list()) {
				list_ptr_t val_list = get_node(val_node)->as_list();
				if(val_list->size() == 0) {
					return NIL_NODE;
				}
				return val_list->nth(0);
			}
		}
	}
	return NIL_NODE;
}

node_idx_t native_last(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		return node->as_list()->last_value();
	}
	if(node->is_lazy_list()) {
		// call the t_lazy_fn, and grab the first element of the return and return that.
		node_idx_t reti = eval_node(env, node->t_lazy_fn);
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			if(list_list->size() == 0) {
				return NIL_NODE;
			}
			do {
				node_idx_t last_val = list_list->nth(0);
				node_idx_t val_node_idx = eval_list(env, list_list->rest());
				node_t *val_node = get_node(val_node_idx);
				if(!val_node->is_list()) {
					return last_val;
				}
				list_list = val_node->as_list();
			} while(true);
		}
		return NIL_NODE;
	}
	return NIL_NODE;
}

node_idx_t native_drop(list_ptr_t env, list_ptr_t args) {
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
		// call the t_lazy_fn, and grab the first element of the return and return that.
		node_idx_t reti = eval_node(env, list->t_lazy_fn);
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			if(list_list->size() == 0) {
				return NIL_NODE;
			}
			do {
				node_idx_t last_val = list_list->nth(0);
				node_idx_t val_node_idx = eval_list(env, list_list->rest());
				node_t *val_node = get_node(val_node_idx);
				if(!val_node->is_list()) {
					return NIL_NODE;
				}
				list_list = val_node->as_list();
			} while(--n);
			return new_node_lazy_list(new_node_list(list_list->rest()));
		}
		return NIL_NODE;
	}
	return NIL_NODE;
}

node_idx_t native_nth(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t list_idx = *it++;
	node_t *list = get_node(list_idx);
	list_ptr_t list_list = list->as_list();
	node_idx_t n_idx = *it++;
	int n = get_node(n_idx)->as_int();
	if(list->is_list()) {
		return list_list->nth(n);
	}
	if(list->is_lazy_list()) {
		// call the t_lazy_fn, and grab the first element of the return and return that.
		node_idx_t reti = eval_node(env, list->t_lazy_fn);
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			if(list_list->size() == 0) {
				return NIL_NODE;
			}
			node_idx_t last_val = NIL_NODE;
			do {
				last_val = list_list->nth(0);
				node_idx_t val_node_idx = eval_list(env, list_list->rest());
				node_t *val_node = get_node(val_node_idx);
				if(!val_node->is_list()) {
					return last_val;
				}
				list_list = val_node->as_list();
			} while(--n);
			return last_val;
		}
		return NIL_NODE;
	}
	return NIL_NODE;
}

// equivalent to (first (first x))
node_idx_t native_ffirst(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	node_idx_t first_idx = NIL_NODE;
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		if(list_list->size() == 0) {
			return NIL_NODE;
		}
		first_idx = list_list->nth(0);
	}
	if(node->is_lazy_list()) {
		// call the t_lazy_fn, and grab the first element of the return and return that.
		node_idx_t reti = eval_node(env, node->t_lazy_fn);
		node_t *ret = get_node(reti);
		if(!ret->is_list()) {
			return NIL_NODE;
		}
		list_ptr_t list_list = ret->as_list();
		if(list_list->size() == 0) {
			return NIL_NODE;
		}
		first_idx = list_list->nth(0);
	}
	node_t *first = get_node(first_idx);
	if(first->is_list()) {
		list_ptr_t first_list = first->as_list();
		if(first_list->size() == 0) {
			return NIL_NODE;
		}
		return first_list->nth(0);
	}
	if(first->is_lazy_list()) {
		// call the t_lazy_fn, and grab the first element of the return and return that.
		node_idx_t reti = eval_node(env, first->t_lazy_fn);
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			if(list_list->size() == 0) {
				return NIL_NODE;
			}
			return list_list->nth(0);
		}
	}
	return NIL_NODE;
}

node_idx_t native_is_fn(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return new_node_bool(node->type == NODE_FUNC);
}

// (next coll)
// Returns a seq of the items after the first. Calls seq on its
// argument.  If there are no more items, returns nil.
node_idx_t native_next(list_ptr_t env, list_ptr_t args) {
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
		node_idx_t reti = eval_node(env, node->t_lazy_fn);
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			if(list_list->size() == 0) {
				return NIL_NODE;
			}
			return new_node_lazy_list(new_node_list(list_list->rest()));
		}
	}
	return NIL_NODE;
}

// like next, but always returns a list
node_idx_t native_rest(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		return new_node_list(list_list->rest());
	}
	if(node->is_lazy_list()) {
		// TODO: is this correct behavior for reaching the end of the lazy seq?
		node_idx_t reti = eval_node(env, node->t_lazy_fn);
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			return new_node_lazy_list(new_node_list(list_list->rest()));
		}
	}
	return NIL_NODE;
}

node_idx_t native_unless(list_ptr_t env, list_ptr_t args) {
	node_idx_t node_idx = args->nth(0);
	node_t *node = get_node(node_idx);
	if(!node->as_bool()) {
		return eval_node_list(env, args->rest());
	}
	return NIL_NODE;
}

// (let (a 1 b 2) (+ a b))
node_idx_t native_let(list_ptr_t env, list_ptr_t args) {
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
	list_ptr_t new_env = env;
	for(list_t::iterator i = list_list->begin(); i;) {
		node_idx_t key_idx = *i++; // TODO: should this be eval'd?
		node_idx_t value_idx = eval_node(new_env, *i++);
		node_t *key = get_node(key_idx);
		new_env = new_env->cons(new_node_var(key->as_string(), value_idx));
	}
	return eval_node_list(new_env, args->rest());
}

// (take n coll)
// Returns a lazy sequence of the first n items in coll, or all items if there are fewer than n.
node_idx_t native_take(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	int n = node->as_int();
	node_idx_t coll_idx = *it++;
	node_t *coll = get_node(coll_idx);
	if(coll->is_list()) {
		list_ptr_t list_list = coll->as_list();
		return new_node_list(list_list->subvec(0, n));
	}
	if(coll->is_lazy_list()) {
		// executes a chain of n functions and returns a sequence of all the values returned
		node_idx_t reti = eval_node(env, coll->t_lazy_fn);
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			list_ptr_t return_list = new_list();
			do {
				return_list->push_back_inplace(list_list->nth(0));
				reti = eval_list(env, list_list->rest());
				ret = get_node(reti);
				if(ret->is_list()) {
					list_list = ret->as_list();
				}
			} while(return_list->size() < n && ret->is_list());
			return new_node_list(return_list);
		}
		return NIL_NODE;
	}
	return NIL_NODE;
}

node_idx_t native_apply(list_ptr_t env, list_ptr_t args) {
	// collect the arguments, if its a list add the whole list, then eval it
	list_ptr_t arg_list = new_list();
	for(list_t::iterator it = args->begin(); it; it++) {
		node_idx_t arg_idx = it == args->begin() ? *it : eval_node(env, *it);
		node_t *arg = get_node(arg_idx);
		if(arg->is_list()) {
			arg_list = arg_list->conj(*arg->as_list().ptr);
		} else if(arg->is_lazy_list()) {
			// executes a chain of n functions and returns a sequence of all the values returned
			node_idx_t reti = eval_node(env, arg->t_lazy_fn);
			node_t *ret = get_node(reti);
			if(ret->is_list()) {
				list_ptr_t list_list = ret->as_list();
				do {
					arg_list->push_back_inplace(list_list->nth(0));
					reti = eval_list(env, list_list->rest());
					ret = get_node(reti);
					if(ret->is_list()) {
						list_list = ret->as_list();
					}
				} while(ret->is_list());
			}
		} else {
			arg_list->push_back_inplace(arg_idx);
		}
	}
	return eval_list(env, arg_list);
}


node_idx_t native_take_last(list_ptr_t env, list_ptr_t args) {
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
node_idx_t native_and(list_ptr_t env, list_ptr_t args) {
	for(list_t::iterator it = args->begin(); it; it++) {
		node_idx_t node_idx = eval_node(env, *it);
		node_t *node = get_node(node_idx);
		if(!node->as_bool()) {
			return new_node_bool(false);
		}
	}
	return new_node_bool(true);
}

node_idx_t native_or(list_ptr_t env, list_ptr_t args) {
	for(list_t::iterator it = args->begin(); it; it++) {
		node_idx_t node_idx = eval_node(env, *it);
		node_t *node = get_node(node_idx);
		if(node->as_bool()) {
			return new_node_bool(true);
		}
	}
	return new_node_bool(false);
}

node_idx_t native_not(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = eval_node(env, *it++);
	node_t *node = get_node(node_idx);
	return new_node_bool(!node->as_bool());
}

node_idx_t native_bit_and(list_ptr_t env, list_ptr_t args) {
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

node_idx_t native_bit_or(list_ptr_t env, list_ptr_t args) {
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

node_idx_t native_bit_xor(list_ptr_t env, list_ptr_t args) {
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

node_idx_t native_bit_not(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = eval_node(env, *it++);
	node_t *node = get_node(node_idx);
	return new_node_int(~node->as_int());
}

// removes duplicate elements in a list
node_idx_t native_distinct(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		list_ptr_t ret = new_list();
		for(list_t::iterator it = list_list->begin(); it; it++) {
			node_idx_t value_idx = eval_node(env, *it);
			node_t *value = get_node(value_idx);
			if(!ret->contains(value_idx)) { // TODO:  check for equality, not contains.... right?
				ret->push_back_inplace(value_idx);
			}
		}
		return new_node_list(ret);
	}
	if(node->is_lazy_list()) {
		// executes a chain of n functions and returns a sequence of all the values returned
		node_idx_t reti = eval_node(env, node->t_lazy_fn);
		node_t *ret = get_node(reti);
		if(ret->is_list()) {
			list_ptr_t list_list = ret->as_list();
			list_ptr_t return_list = new_list();
			do {
				node_idx_t value_idx = list_list->nth(0);
				if(!return_list->contains(value_idx)) {
					return_list->push_back_inplace(value_idx);
				}
				reti = eval_list(env, list_list->rest());
				ret = get_node(reti);
				if(ret->is_list()) { // TODO:  check for equality, not contains.... right?
					list_list = ret->as_list();
				}
			} while(ret->is_list());
			return new_node_list(return_list);
		}
		return NIL_NODE;
	}
	return NIL_NODE;
}

node_idx_t native_reverse(list_ptr_t env, list_ptr_t args) {
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

// (range)(range end)(range start end)(range start end step)
// Returns a lazy seq of nums from start (inclusive) to end
// (exclusive), by step, where start defaults to 0, step to 1, and end to
// infinity. When step is equal to 0, returns an infinite sequence of
// start. When start is equal to end, returns empty list.
node_idx_t native_range(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	int end = args->size();
	int start = 0;
	int step = 1;
	if(end == 0) {
		end = INT_MAX; // "infinite" series
	}
	if(end == 1) {
		end = get_node(*it++)->as_int();
	}
	if(end == 2) {
		start = get_node(*it++)->as_int();
		end = get_node(*it++)->as_int();
	}
	if(end == 3) {
		start = get_node(*it++)->as_int();
		end = get_node(*it++)->as_int();
		step = get_node(*it++)->as_int();
	}
	// constructs a function which returns the next value in the range, and another function
	node_idx_t lazy_func_idx = new_node(NODE_LIST);
	node_t *lazy_func = get_node(lazy_func_idx);
	lazy_func->t_list = new_list();
	lazy_func->t_list->push_back_inplace(new_node_symbol("range-next"));
	lazy_func->t_list->push_back_inplace(new_node_int(start));
	lazy_func->t_list->push_back_inplace(new_node_int(step));
	lazy_func->t_list->push_back_inplace(new_node_int(end));
	return new_node_lazy_list(lazy_func_idx);
}

node_idx_t native_range_next(list_ptr_t env, list_ptr_t args) {
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

// (repeat x) (repeat n x)
// Returns a lazy (infinite!, or length n if supplied) sequence of xs.
node_idx_t native_repeat(list_ptr_t env, list_ptr_t args) {
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
	node_t *lazy_func = get_node(lazy_func_idx);
	lazy_func->t_list = new_list();
	lazy_func->t_list->push_back_inplace(new_node_symbol("repeat-next"));
	lazy_func->t_list->push_back_inplace(x);
	lazy_func->t_list->push_back_inplace(new_node_int(n));
	return new_node_lazy_list(lazy_func_idx);
}

node_idx_t native_repeat_next(list_ptr_t env, list_ptr_t args) {
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

// (concat)(concat x)(concat x y)(concat x y & zs)
// Returns a lazy seq representing the concatenation of the elements in the supplied colls.
node_idx_t native_concat(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t x = *it++;
	node_idx_t lazy_func_idx = new_node(NODE_LIST);
	node_t *lazy_func = get_node(lazy_func_idx);
	lazy_func->t_list = new_list();
	lazy_func->t_list->push_back_inplace(new_node_symbol("concat-next"));
	lazy_func->t_list->conj_inplace(*args.ptr);
	return new_node_lazy_list(lazy_func_idx);
}

node_idx_t native_concat_next(list_ptr_t env, list_ptr_t args) {
concat_next:
	if(args->size() == 0) {
		return NIL_NODE;
	}
	node_idx_t nidx = args->first_value();
	node_idx_t val = NIL_NODE;
	int ntype = get_node(nidx)->type;
	args = args->pop();
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
			val = list_list->nth(0);
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


#include "jo_lisp_math.h"
#include "jo_lisp_string.h"
#include "jo_lisp_system.h"

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

	// new_node_func(new_node_list(new_node_symbol("print")), new_node_list(new_node_symbol("quote"))))

	list_ptr_t env = new_list();
	env->push_back_inplace(new_node_var("nil", new_node(NODE_NIL)));
	env->push_back_inplace(new_node_var("t", new_node_bool(true)));
	env->push_back_inplace(new_node_var("f", new_node_bool(false)));
	env->push_back_inplace(new_node_var("let", new_node_native_function(&native_let, true)));
	env->push_back_inplace(new_node_var("print", new_node_native_function(&native_print, false)));
	env->push_back_inplace(new_node_var("println", new_node_native_function(&native_println, false)));
	env->push_back_inplace(new_node_var("+", new_node_native_function(&native_add, false)));
	env->push_back_inplace(new_node_var("-", new_node_native_function(&native_sub, false)));
	env->push_back_inplace(new_node_var("*", new_node_native_function(&native_mul, false)));
	env->push_back_inplace(new_node_var("/", new_node_native_function(&native_div, false)));
	env->push_back_inplace(new_node_var("mod", new_node_native_function(&native_mod, false)));
	env->push_back_inplace(new_node_var("inc", new_node_native_function(&native_inc, false)));
	env->push_back_inplace(new_node_var("dec", new_node_native_function(&native_dec, false)));
	env->push_back_inplace(new_node_var("=", new_node_native_function(&native_eq, false)));
	env->push_back_inplace(new_node_var("!=", new_node_native_function(&native_neq, false)));
	env->push_back_inplace(new_node_var("not=", new_node_native_function(&native_neq, false)));
	env->push_back_inplace(new_node_var("<", new_node_native_function(&native_lt, false)));
	env->push_back_inplace(new_node_var("<=", new_node_native_function(&native_lte, false)));
	env->push_back_inplace(new_node_var(">", new_node_native_function(&native_gt, false)));
	env->push_back_inplace(new_node_var(">=", new_node_native_function(&native_gte, false)));
	env->push_back_inplace(new_node_var("and", new_node_native_function(&native_and, true)));
	env->push_back_inplace(new_node_var("or", new_node_native_function(&native_or, true)));
	env->push_back_inplace(new_node_var("not", new_node_native_function(&native_not, true)));
	env->push_back_inplace(new_node_var("bit-and", new_node_native_function(&native_bit_and, true)));
	env->push_back_inplace(new_node_var("bit-or", new_node_native_function(&native_bit_or, true)));
	env->push_back_inplace(new_node_var("bit-xor", new_node_native_function(&native_bit_xor, true)));
	env->push_back_inplace(new_node_var("bit-not", new_node_native_function(&native_bit_not, true)));
	env->push_back_inplace(new_node_var("empty?", new_node_native_function(&native_is_empty, false)));
	env->push_back_inplace(new_node_var("false?", new_node_native_function(&native_is_false, false)));
	env->push_back_inplace(new_node_var("true?", new_node_native_function(&native_is_true, false)));
	env->push_back_inplace(new_node_var("do", new_node_native_function(&native_do, false)));
	env->push_back_inplace(new_node_var("cons", new_node_native_function(&native_cons, false)));
	env->push_back_inplace(new_node_var("conj", new_node_native_function(&native_conj, false)));
	env->push_back_inplace(new_node_var("pop", new_node_native_function(&native_pop, false)));
	env->push_back_inplace(new_node_var("peek", new_node_native_function(&native_peek, false)));
	env->push_back_inplace(new_node_var("first", new_node_native_function(&native_first, false)));
	env->push_back_inplace(new_node_var("second", new_node_native_function(&native_second, false)));
	env->push_back_inplace(new_node_var("last", new_node_native_function(&native_last, false)));
	env->push_back_inplace(new_node_var("drop", new_node_native_function(&native_drop, false)));
	env->push_back_inplace(new_node_var("nth", new_node_native_function(&native_nth, false)));
	env->push_back_inplace(new_node_var("ffirst", new_node_native_function(&native_ffirst, false)));
	env->push_back_inplace(new_node_var("next", new_node_native_function(&native_next, false)));
	env->push_back_inplace(new_node_var("rest", new_node_native_function(&native_rest, false)));
	env->push_back_inplace(new_node_var("quote", new_node_native_function(&native_quote, true)));
	env->push_back_inplace(new_node_var("list", new_node_native_function(&native_list, false)));
	env->push_back_inplace(new_node_var("take", new_node_native_function(&native_take, false)));
	env->push_back_inplace(new_node_var("take-last", new_node_native_function(&native_take_last, false)));
	env->push_back_inplace(new_node_var("distinct", new_node_native_function(&native_distinct, false)));
	env->push_back_inplace(new_node_var("reverse", new_node_native_function(&native_upper_case, false)));
	env->push_back_inplace(new_node_var("concat", new_node_native_function(&native_concat, false)));
	env->push_back_inplace(new_node_var("var", new_node_native_function(&native_var, false)));
	env->push_back_inplace(new_node_var("def", new_node_native_function(&native_def, true)));
	env->push_back_inplace(new_node_var("fn", new_node_native_function(&native_fn, true)));
	env->push_back_inplace(new_node_var("fn?", new_node_native_function(&native_is_fn, false)));
	env->push_back_inplace(new_node_var("defn", new_node_native_function(&native_defn, true)));
	env->push_back_inplace(new_node_var("*ns*", new_node_var("nil", NIL_NODE)));
	env->push_back_inplace(new_node_var("if", new_node_native_function(&native_if, true)));
	env->push_back_inplace(new_node_var("unless", new_node_native_function(&native_unless, true)));
	env->push_back_inplace(new_node_var("when", new_node_native_function(&native_when, true)));
	env->push_back_inplace(new_node_var("while", new_node_native_function(&native_while, true)));
	env->push_back_inplace(new_node_var("cond", new_node_native_function(&native_cond, true)));
	env->push_back_inplace(new_node_var("case", new_node_native_function(&native_case, true)));
	env->push_back_inplace(new_node_var("apply", new_node_native_function(&native_apply, true)));
	env->push_back_inplace(new_node_var("delay", new_node_native_function(&native_delay, true)));
	env->push_back_inplace(new_node_var("delay?", new_node_native_function(&native_is_delay, false)));
	env->push_back_inplace(new_node_var("constantly", new_node_native_function(&native_constantly, false)));
	env->push_back_inplace(new_node_var("count", new_node_native_function(&native_count, false)));
	//env->push_back_inplace(new_node_var("repeat", new_node_native_function(&native_repeat, true)));
	env->push_back_inplace(new_node_var("dotimes", new_node_native_function(&native_dotimes, true)));
	env->push_back_inplace(new_node_var("doseq", new_node_native_function(&native_doseq, true)));
	env->push_back_inplace(new_node_var("nil?", new_node_native_function(&native_is_nil, false)));
	env->push_back_inplace(new_node_var("rand-int", new_node_native_function(&native_rand_int, false)));
	env->push_back_inplace(new_node_var("rand-float", new_node_native_function(&native_rand_float, false)));
	env->push_back_inplace(new_node_var("Time/now", new_node_native_function(&native_time_now, false)));
	// lazy stuffs
	env->push_back_inplace(new_node_var("range", new_node_native_function(&native_range, false)));
	env->push_back_inplace(new_node_var("range-next", new_node_native_function(&native_range_next, false)));
	env->push_back_inplace(new_node_var("repeat", new_node_native_function(&native_repeat, true)));
	env->push_back_inplace(new_node_var("repeat-next", new_node_native_function(&native_repeat_next, true)));
	env->push_back_inplace(new_node_var("concat", new_node_native_function(&native_concat, true)));
	env->push_back_inplace(new_node_var("concat-next", new_node_native_function(&native_concat_next, true)));
	jo_lisp_math_init(env);
	jo_lisp_string_init(env);
	jo_lisp_system_init(env);
	

	//print_node_list(env, 0);

	FILE *fp = fopen(argv[1], "r");
	if(!fp) {
		return 0;
	}
	
	//printf("Parsing...\n");

	parse_state_t parse_state;
	parse_state.fp = fp;
	parse_state.line_num = 1;

	// parse the base list
	list_ptr_t main_list = new_list();
	for(node_idx_t next = parse_next(&parse_state, 0); next != NIL_NODE; next = parse_next(&parse_state, 0)) {
		main_list->push_back_inplace(next);
	}
	fclose(fp);

	//print_node_list(main_list, 0);

	node_idx_t res_idx = eval_node_list(env, main_list);
	print_node(res_idx, 0);

	//print_node(root_idx, 0);

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

