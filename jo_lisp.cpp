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
	NODE_VECTOR,
	NODE_SET,
	NODE_MAP,
	NODE_NATIVE_FUNCTION,
	NODE_FUNC,
	NODE_VAR,
	NODE_DELAY,

	NODE_FLAG_MACRO = 1<<0,
};

typedef int node_idx_t;
typedef jo_string sym_t;
typedef jo_persistent_list<node_idx_t> list_t; // TODO: make node_t
typedef jo_shared_ptr<list_t> list_ptr_t;

typedef node_idx_t (*native_function_t)(list_ptr_t env, list_ptr_t args);

list_ptr_t new_list() {
	return list_ptr_t(new list_t());
}

struct node_t {
	int type;
	int stamp;
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
		native_function_t t_native_function;
	};

	bool is_list() const {
		return type == NODE_LIST;
	}

	list_ptr_t &as_list() {
		return t_list;
	}

	bool is_string() const {
		return type == NODE_STRING;
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

node_t *get_node(node_idx_t idx) {
	return &nodes[idx];
}

int get_node_type(node_idx_t idx) {
	return get_node(idx)->type;
}

jo_string get_node_string(node_idx_t idx) {
	return get_node(idx)->as_string();
}

jo_string get_node_type_string(node_idx_t idx) {
	return get_node(idx)->type_as_string();
}

/*
node_idx_t next_node(node_idx_t idx) {
	if(idx == NIL_NODE) {
		return NIL_NODE;
	}
	return nodes[idx].t_next;
}
*/

node_idx_t alloc_node() {
	if (free_nodes.size()) {
		node_idx_t idx = free_nodes.back();
		free_nodes.pop_back();
		return idx;
	}
	node_idx_t idx = nodes.size();
	nodes.push_back(node_t());
	return idx;
}

void free_node(node_idx_t idx) {
	free_nodes.push_back(idx);
}

// TODO: Should prefer to allocate nodes next to existing nodes which will be linked (for cache coherence)
node_idx_t new_node(const node_t *n) {
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

/*
node_idx_t link_node(node_idx_t from, node_idx_t to) {
	node_t *f = &nodes[from];
	f->t_next = to;
	return to;
}

void insert_node(node_idx_t from, node_idx_t to) {
	node_t *f = &nodes[from];
	node_t *t = &nodes[to];
	node_idx_t next = f->t_next;
	f->t_next = to;
	t->t_next = next;
}
*/

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

static int is_whitespace(int c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int is_num(int c) {
	return (c >= '0' && c <= '9');
}

static int is_alnum(int c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int is_separator(int c) {
	return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == ',';
}

enum token_type_t {
	TOK_EOF=0,
	TOK_STRING,
	TOK_SYMBOL,
	TOK_SEPARATOR,
};

struct token_t {
	token_type_t type;
	jo_string str;
};

token_t get_token(FILE *fp) {
	// skip leading whitepsace and comma
	do {
		int c = fgetc(fp);
		if(!is_whitespace(c) && c != ',') {
			ungetc(c, fp);
			break;
		}
	} while(true);

	token_t tok;

	int c = fgetc(fp);
	
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
			int C = fgetc(fp);
			if (C == EOF) {
				fprintf(stderr, "unterminated string\n");
				exit(__LINE__);
			}
			// escape next character
			if(C == '\\') {
				C = fgetc(fp);
				if(C == EOF) {
					fprintf(stderr, "unterminated string\n");
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
			int C = fgetc(fp);
			if(is_whitespace(C) || is_separator(C) || C == EOF) {
				ungetc(C, fp);
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
			int C = fgetc(fp);
			if (C == EOF) {
				fprintf(stderr, "unterminated comment\n");
				exit(__LINE__);
			}
			if(C == '\n') break;
		} while(true);
		return get_token(fp); // recurse
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
			ungetc(c, fp);
			break;
		}
		tok.str += (char)c;
		c = fgetc(fp);
	} while(true);
	
	debugf("token: %s\n", tok.str.c_str());

	return tok;
}

node_idx_t parse_next(FILE *fp, int stop_on_sep) {
	token_t tok = get_token(fp);
	debugf("parse_next %s, with %c\n", tok.str.c_str(), stop_on_sep);

	if(tok.type == TOK_EOF) {
		// end of list
		return NIL_NODE;
	}

	const char *tok_ptr = tok.str.c_str();
	const char *tok_ptr_end = tok_ptr + tok.str.size();
	int c = tok_ptr[0];
	int c2 = tok_ptr[1];

	if(c == 0 || c == stop_on_sep) {
		// end of list
		return NIL_NODE;
	}

	if(tok.type == TOK_SEPARATOR && c != stop_on_sep && (c == ')' || c == ']' || c == '}')) {
		fprintf(stderr, "unexpected separator '%c', was expecting '%c'\n", c, stop_on_sep);
		return NIL_NODE;
	}

	// 
	// literals
	//

	// container types
	//if(c == '#' && c2 == '"') return parse_regex(fp);
	// parse number...
	else if(is_num(c)) {
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
	} else if(tok.type == TOK_STRING) {
		debugf("string: %s\n", tok.str.c_str());
		return new_node_string(tok.str.c_str());
	} else if(tok.type == TOK_SYMBOL) {
		debugf("symbol: %s\n", tok.str.c_str());
		return new_node_symbol(tok.str.c_str());
	} 

	// parse list
	if(c == '(') {
		node_t n = {NODE_LIST};
		n.t_list = new_list();
		node_idx_t next = parse_next(fp, ')');
		while(next != NIL_NODE) {
			n.t_list->push_back_inplace(next);
			next = parse_next(fp, ')');
		}
		return new_node(&n);
	}

	// parse map

	// parse set

	return NIL_NODE;
}

node_idx_t eval_node(list_ptr_t env, node_idx_t root);

// eval a list of nodes
node_idx_t eval_list(list_ptr_t env, list_ptr_t list) {
	list_t::iterator it = list->begin();
	node_idx_t n1i = *it++;
	node_t *n1 = get_node(n1i);
	if(n1->type == NODE_SYMBOL) {

		node_idx_t sym_idx = env_get(env, n1->t_string);
		node_t *sym_node = get_node(sym_idx);

		// get the symbol's value
		if(sym_node->type == NODE_NATIVE_FUNCTION) {
			if(sym_node->flags & NODE_FLAG_MACRO) {
				list_ptr_t args1(list->rest());
				return sym_node->t_native_function(env, args1);
			}

			list_ptr_t args = new_list();
			for(; it; it++) {
				args->push_back_inplace(eval_node(env, *it));
			}

			// call the function
			return sym_node->t_native_function(env, args);
		} else if(sym_node->type == NODE_FUNC || sym_node->type == NODE_DELAY) {
			list_ptr_t &proto_args = sym_node->t_func.args;
			list_ptr_t &proto_body = sym_node->t_func.body;
			list_ptr_t &proto_env = sym_node->t_func.env;
			list_ptr_t fn_env = proto_env->conj(*env);
			list_ptr_t args1(list->rest());

			if(sym_node->type == NODE_DELAY && sym_node->t_delay != INV_NODE) {
				return sym_node->t_delay;
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

			if(sym_node->type == NODE_DELAY) {
				sym_node->t_delay = last;
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
		printf("%*s<native function>\n", depth, "");
	} else if(n->type == NODE_FUNC) {
		printf("%*s(defn %s\n", depth, "", n->t_string.c_str());
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
	node_idx_t cond = eval_node(env, *i++);
	list_t::iterator j = i;
	node_idx_t ret = NIL_NODE;
	while(get_node(cond)->as_bool()) {
		for(; j; j++) {
			ret = eval_node(env, *j);
		}
		cond = eval_node(env, *i);
	}
	return ret;
}

node_idx_t native_quote(list_ptr_t env, list_ptr_t args) {
	node_t n = {NODE_LIST};
	n.t_list = args->rest();
	return new_node(&n);
}

node_idx_t native_list(list_ptr_t env, list_ptr_t args) {
	node_t n = {NODE_LIST};
	n.t_list = args->rest();
	return new_node(&n);
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
	node_t *sym_node = get_node(*i++);
	node_idx_t init = NIL_NODE;
	if(i) {
		node_idx_t doc_string = *i++; // ignored for eval purposes if present
		if(get_node_type(doc_string) != NODE_STRING) {
			init = doc_string;
		} else if(i) {
			init = *i++;
		}
	}

	if(sym_node->type != NODE_SYMBOL) {
		fprintf(stderr, "def: expected symbol");
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

node_idx_t native_math_abs(list_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->nth(0));
	if(n1->type == NODE_INT) {
		return new_node_int(abs(n1->t_int));
	} else {
		return new_node_float(fabs(n1->as_float()));
	}
}

node_idx_t native_math_sqrt(list_ptr_t env, list_ptr_t args) { return new_node_float(sqrt(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_cbrt(list_ptr_t env, list_ptr_t args) { return new_node_float(cbrt(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_ceil(list_ptr_t env, list_ptr_t args) { return new_node_int(ceil(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_floor(list_ptr_t env, list_ptr_t args) { return new_node_int(floor(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_exp(list_ptr_t env, list_ptr_t args) { return new_node_float(exp(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_exp2(list_ptr_t env, list_ptr_t args) { return new_node_float(exp2(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_hypot(list_ptr_t env, list_ptr_t args) { return new_node_float(hypot(get_node(args->nth(0))->as_float(), get_node(args->nth(1))->as_float())); }
node_idx_t native_math_log10(list_ptr_t env, list_ptr_t args) { return new_node_float(log10(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_log(list_ptr_t env, list_ptr_t args) { return new_node_float(log(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_log2(list_ptr_t env, list_ptr_t args) { return new_node_float(log2(get_node(args->nth(0))->as_float()));}
node_idx_t native_math_log1p(list_ptr_t env, list_ptr_t args) { return new_node_float(log1p(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_sin(list_ptr_t env, list_ptr_t args) { return new_node_float(sin(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_cos(list_ptr_t env, list_ptr_t args) { return new_node_float(cos(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_tan(list_ptr_t env, list_ptr_t args) { return new_node_float(tan(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_pow(list_ptr_t env, list_ptr_t args) { return new_node_float(pow(get_node(args->nth(0))->as_float(), get_node(args->nth(1))->as_float())); }
node_idx_t native_math_sinh(list_ptr_t env, list_ptr_t args) { return new_node_float(sinh(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_cosh(list_ptr_t env, list_ptr_t args) { return new_node_float(cosh(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_tanh(list_ptr_t env, list_ptr_t args) { return new_node_float(tanh(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_asin(list_ptr_t env, list_ptr_t args) { return new_node_float(asin(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_acos(list_ptr_t env, list_ptr_t args) { return new_node_float(acos(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_atan(list_ptr_t env, list_ptr_t args) { return new_node_float(atan(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_asinh(list_ptr_t env, list_ptr_t args) { return new_node_float(asinh(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_acosh(list_ptr_t env, list_ptr_t args) { return new_node_float(acosh(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_atanh(list_ptr_t env, list_ptr_t args) { return new_node_float(atanh(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_erf(list_ptr_t env, list_ptr_t args) { return new_node_float(erf(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_erfc(list_ptr_t env, list_ptr_t args) { return new_node_float(erfc(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_tgamma(list_ptr_t env, list_ptr_t args) { return new_node_float(tgamma(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_lgamma(list_ptr_t env, list_ptr_t args) { return new_node_float(lgamma(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_round(list_ptr_t env, list_ptr_t args) { return new_node_int(round(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_trunc(list_ptr_t env, list_ptr_t args) { return new_node_int(trunc(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_logb(list_ptr_t env, list_ptr_t args) { return new_node_float(logb(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_ilogb(list_ptr_t env, list_ptr_t args) { return new_node_int(ilogb(get_node(args->nth(0))->as_float())); }
node_idx_t native_math_expm1(list_ptr_t env, list_ptr_t args) { return new_node_float(expm1(get_node(args->nth(0))->as_float())); }

// Computes the minimum value of any number of arguments
node_idx_t native_math_min(list_ptr_t env, list_ptr_t args) {
	// If there are no arguments, return nil
	if(args->length == 0) {
		return NIL_NODE;
	}

	list_t::iterator it = args->begin();

	// Get the first argument
	node_idx_t min_node = *it++;
	node_t *n = get_node(min_node);

	if(n->type == NODE_INT) {
		bool is_int = true;
		int min_int = n->t_int;
		float min_float = min_int;
		for(node_idx_t next = *it++; it; next = *it++) {
			n = get_node(next);
			if(is_int && n->type == NODE_INT) {
				if(n->t_int < min_int) {
					min_int = n->t_int;
					min_float = min_int;
					min_node = next;
				}
			} else {
				is_int = false;
				if(n->as_float() < min_float) {
					min_float = n->as_float();
					min_node = next;
				}
			}
		}
		if(is_int) {
			return new_node_int(min_int);
		}
		return new_node_float(min_int);
	}

	float min_float = n->as_float();
	for(node_idx_t next = *it++; it; next = *it++) {
		n = get_node(next);
		if(n->as_float() < min_float) {
			min_float = n->as_float();
			min_node = next;
		}
	}
	return min_node;
}

node_idx_t native_math_max(list_ptr_t env, list_ptr_t args) {
	// If there are no arguments, return nil
	if(args->length == 0) {
		return NIL_NODE;
	}
	
	list_t::iterator it = args->begin();
	
	// Get the first argument
	node_idx_t max_node = *it++;
	node_t *n = get_node(max_node);

	if(n->type == NODE_INT) {
		bool is_int = true;
		int max_int = n->t_int;
		float max_float = max_int;
		for(node_idx_t next = *it++; it; next = *it++) {
			n = get_node(next);
			if(is_int && n->type == NODE_INT) {
				if(n->t_int > max_int) {
					max_int = n->t_int;
					max_float = max_int;
					max_node = next;
				}
			} else {
				is_int = false;
				if(n->as_float() > max_float) {
					max_float = n->as_float();
					max_node = next;
				}
			}
		}
		if(is_int) {
			return new_node_int(max_int);
		}
		return new_node_float(max_int);
	}

	float max_float = n->as_float();
	for(node_idx_t next = *it++; it; next = *it++) {
		n = get_node(next);
		if(n->as_float() > max_float) {
			max_float = n->as_float();
			max_node = next;
		}
	}
	return max_node;
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

node_idx_t native_math_clamp(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t n1i = *it++;
	node_t *n1 = get_node(n1i);
	node_idx_t n2i = *it++;
	node_t *n2 = get_node(n2i);
	node_idx_t n3i = *it++;
	node_t *n3 = get_node(n3i);
	if(n1->type == NODE_INT && n2->type == NODE_INT && n3->type == NODE_INT) {
		int val = n1->t_int;
		int min = n2->t_int;
		int max = n3->t_int;
		val = val < min ? min : val > max ? max : val;
		return new_node_int(val);
	}
	float val = n1->as_float();
	float min = n2->as_float();
	float max = n3->as_float();
	val = val < min ? min : val > max ? max : val;
	return new_node_float(val);
}

// concat all arguments as_string
node_idx_t native_str(list_ptr_t env, list_ptr_t args) {
	jo_string str;
	for(list_t::iterator it = args->begin(); it; it++) {
		node_t *n = get_node(*it);
		str += n->as_string();
	}
	return new_node_string(str);
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

node_idx_t native_apply(list_ptr_t env, list_ptr_t args) {
	node_idx_t func_idx = eval_node(env, args->nth(0));
	node_t *func = get_node(func_idx);
	list_ptr_t func_args = new_list();
	for(list_t::iterator it = args->begin(); it; it++) {
		node_idx_t arg_idx = *it;
		if(arg_idx != func_idx) {
			func_args->cons(arg_idx);
		}
	}
	return eval_node(env, func_args);
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
	node_idx_t value_idx = binding_list->nth(1);
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

node_idx_t native_is_even(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return new_node_bool((node->as_int() & 1) == 0);
}

node_idx_t native_is_odd(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return new_node_bool((node->as_int() & 1) == 1);
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
	return NIL_NODE;
}

// equivalent to (first (first x))
node_idx_t native_ffirst(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	if(node->is_list()) {
		list_ptr_t list_list = node->as_list();
		if(list_list->size() == 0) {
			return NIL_NODE;
		}
		node_idx_t first_idx = list_list->nth(0);
		node_t *first = get_node(first_idx);
		if(first->is_list()) {
			list_ptr_t first_list = first->as_list();
			if(first_list->size() == 0) {
				return NIL_NODE;
			}
			return first_list->nth(0);
		}
		return NIL_NODE;
	}
	return NIL_NODE;
}

node_idx_t native_is_fn(list_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return new_node_bool(node->type == NODE_FUNC);
}

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
	return NIL_NODE;
}

// execute a shell command
node_idx_t native_sh(list_ptr_t env, list_ptr_t args) {
	jo_string str;
	for(list_t::iterator it = args->begin(); it; it++) {
		node_t *n = get_node(*it);
		str += " ";
		str += n->as_string();
	}
	int ret = system(str.c_str());
	return new_node_int(ret);
}

// same as (first (next args))
//node_idx_t native_fnext(list_ptr_t env, list_ptr_t args) {
	// TODO
//}

int main(int argc, char **argv) {
	if(argc <= 1) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		return 1;
	}

	FILE *fp = fopen(argv[1], "r");
	if(!fp) {
		return 0;
	}

	// new_node_func(new_node_list(new_node_symbol("print")), new_node_list(new_node_symbol("quote"))))

	list_ptr_t env = new_list();
	env->push_back_inplace(new_node_var("nil", new_node(NODE_NIL)));
	env->push_back_inplace(new_node_var("t", new_node_bool(true)));
	env->push_back_inplace(new_node_var("f", new_node_bool(false)));
	env->push_back_inplace(new_node_var("str", new_node_native_function(&native_str, false)));
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
	env->push_back_inplace(new_node_var("<", new_node_native_function(&native_lt, false)));
	env->push_back_inplace(new_node_var("<=", new_node_native_function(&native_lte, false)));
	env->push_back_inplace(new_node_var(">", new_node_native_function(&native_gt, false)));
	env->push_back_inplace(new_node_var(">=", new_node_native_function(&native_gte, false)));
	env->push_back_inplace(new_node_var("even?", new_node_native_function(&native_is_even, false)));
	env->push_back_inplace(new_node_var("odd?", new_node_native_function(&native_is_odd, false)));
	env->push_back_inplace(new_node_var("empty?", new_node_native_function(&native_is_empty, false)));
	env->push_back_inplace(new_node_var("false?", new_node_native_function(&native_is_false, false)));
	env->push_back_inplace(new_node_var("true?", new_node_native_function(&native_is_true, false)));
	env->push_back_inplace(new_node_var("do", new_node_native_function(&native_do, false)));
	env->push_back_inplace(new_node_var("cons", new_node_native_function(&native_cons, false)));
	env->push_back_inplace(new_node_var("conj", new_node_native_function(&native_conj, false)));
	env->push_back_inplace(new_node_var("pop", new_node_native_function(&native_pop, false)));
	env->push_back_inplace(new_node_var("peek", new_node_native_function(&native_peek, false)));
	env->push_back_inplace(new_node_var("first", new_node_native_function(&native_first, false)));
	env->push_back_inplace(new_node_var("ffirst", new_node_native_function(&native_ffirst, false)));
	env->push_back_inplace(new_node_var("next", new_node_native_function(&native_next, false)));
	env->push_back_inplace(new_node_var("rest", new_node_native_function(&native_rest, false)));
	env->push_back_inplace(new_node_var("quote", new_node_native_function(&native_quote, true)));
	env->push_back_inplace(new_node_var("list", new_node_native_function(&native_list, false)));
	env->push_back_inplace(new_node_var("var", new_node_native_function(&native_var, false)));
	env->push_back_inplace(new_node_var("def", new_node_native_function(&native_def, false)));
	env->push_back_inplace(new_node_var("fn", new_node_native_function(&native_fn, true)));
	env->push_back_inplace(new_node_var("fn?", new_node_native_function(&native_is_fn, false)));
	env->push_back_inplace(new_node_var("defn", new_node_native_function(&native_defn, true)));
	env->push_back_inplace(new_node_var("*ns*", new_node_var("nil", NIL_NODE)));
	env->push_back_inplace(new_node_var("if", new_node_native_function(&native_if, true)));
	env->push_back_inplace(new_node_var("when", new_node_native_function(&native_when, true)));
	env->push_back_inplace(new_node_var("while", new_node_native_function(&native_while, true)));
	env->push_back_inplace(new_node_var("quote", new_node_native_function(&native_quote, true)));
	env->push_back_inplace(new_node_var("cond", new_node_native_function(&native_cond, true)));
	env->push_back_inplace(new_node_var("case", new_node_native_function(&native_case, true)));
	env->push_back_inplace(new_node_var("apply", new_node_native_function(&native_apply, true)));
	env->push_back_inplace(new_node_var("delay", new_node_native_function(&native_delay, true)));
	env->push_back_inplace(new_node_var("delay?", new_node_native_function(&native_is_delay, false)));
	env->push_back_inplace(new_node_var("constantly", new_node_native_function(&native_constantly, false)));
	env->push_back_inplace(new_node_var("count", new_node_native_function(&native_count, false)));
	env->push_back_inplace(new_node_var("sh", new_node_native_function(&native_sh, false)));
	//env->push_back_inplace(new_node_var("repeat", new_node_native_function(&native_repeat, true)));
	env->push_back_inplace(new_node_var("dotimes", new_node_native_function(&native_dotimes, true)));
	env->push_back_inplace(new_node_var("nil?", new_node_native_function(&native_is_nil, false)));
	env->push_back_inplace(new_node_var("Math/abs", new_node_native_function(&native_math_abs, false)));
	env->push_back_inplace(new_node_var("Math/sqrt", new_node_native_function(&native_math_sqrt, false)));
	env->push_back_inplace(new_node_var("Math/cbrt", new_node_native_function(&native_math_cbrt, false)));
	env->push_back_inplace(new_node_var("Math/sin", new_node_native_function(&native_math_sin, false)));
	env->push_back_inplace(new_node_var("Math/cos", new_node_native_function(&native_math_cos, false)));
	env->push_back_inplace(new_node_var("Math/tan", new_node_native_function(&native_math_tan, false)));
	env->push_back_inplace(new_node_var("Math/asin", new_node_native_function(&native_math_asin, false)));
	env->push_back_inplace(new_node_var("Math/acos", new_node_native_function(&native_math_acos, false)));
	env->push_back_inplace(new_node_var("Math/atan", new_node_native_function(&native_math_atan, false)));
	env->push_back_inplace(new_node_var("Math/sinh", new_node_native_function(&native_math_sinh, false)));
	env->push_back_inplace(new_node_var("Math/cosh", new_node_native_function(&native_math_cosh, false)));
	env->push_back_inplace(new_node_var("Math/tanh", new_node_native_function(&native_math_tanh, false)));
	env->push_back_inplace(new_node_var("Math/asinh", new_node_native_function(&native_math_asinh, false)));
	env->push_back_inplace(new_node_var("Math/acosh", new_node_native_function(&native_math_acosh, false)));
	env->push_back_inplace(new_node_var("Math/atanh", new_node_native_function(&native_math_atanh, false)));
	env->push_back_inplace(new_node_var("Math/exp", new_node_native_function(&native_math_exp, false)));
	env->push_back_inplace(new_node_var("Math/log", new_node_native_function(&native_math_log, false)));
	env->push_back_inplace(new_node_var("Math/log10", new_node_native_function(&native_math_log10, false)));
	env->push_back_inplace(new_node_var("Math/log2", new_node_native_function(&native_math_log2, false)));
	env->push_back_inplace(new_node_var("Math/log1p", new_node_native_function(&native_math_log1p, false)));
	env->push_back_inplace(new_node_var("Math/expm1", new_node_native_function(&native_math_expm1, false)));
	env->push_back_inplace(new_node_var("Math/pow", new_node_native_function(&native_math_pow, false)));
	env->push_back_inplace(new_node_var("Math/hypot", new_node_native_function(&native_math_hypot, false)));
	env->push_back_inplace(new_node_var("Math/erf", new_node_native_function(&native_math_erf, false)));
	env->push_back_inplace(new_node_var("Math/erfc", new_node_native_function(&native_math_erfc, false)));
	env->push_back_inplace(new_node_var("Math/tgamma", new_node_native_function(&native_math_tgamma, false)));
	env->push_back_inplace(new_node_var("Math/lgamma", new_node_native_function(&native_math_lgamma, false)));
	env->push_back_inplace(new_node_var("Math/ceil", new_node_native_function(&native_math_ceil, false)));
	env->push_back_inplace(new_node_var("Math/floor", new_node_native_function(&native_math_floor, false)));
	env->push_back_inplace(new_node_var("Math/round", new_node_native_function(&native_math_round, false)));
	env->push_back_inplace(new_node_var("Math/trunc", new_node_native_function(&native_math_trunc, false)));
	env->push_back_inplace(new_node_var("Math/min", new_node_native_function(&native_math_min, false)));
	env->push_back_inplace(new_node_var("Math/max", new_node_native_function(&native_math_max, false)));
	env->push_back_inplace(new_node_var("Math/clamp", new_node_native_function(&native_math_clamp, false)));
	env->push_back_inplace(new_node_var("Math/PI", new_node_float(JO_M_PI)));
	env->push_back_inplace(new_node_var("Math/E", new_node_float(JO_M_E)));
	env->push_back_inplace(new_node_var("Math/LN2", new_node_float(JO_M_LN2)));
	env->push_back_inplace(new_node_var("Math/LN10", new_node_float(JO_M_LN10)));
	env->push_back_inplace(new_node_var("Math/LOG2E", new_node_float(JO_M_LOG2E)));
	env->push_back_inplace(new_node_var("Math/LOG10E", new_node_float(JO_M_LOG10E)));
	env->push_back_inplace(new_node_var("Math/SQRT2", new_node_float(JO_M_SQRT2)));
	env->push_back_inplace(new_node_var("Math/SQRT1_2", new_node_float(JO_M_SQRT1_2)));
	env->push_back_inplace(new_node_var("Math/NaN", new_node_float(NAN)));
	env->push_back_inplace(new_node_var("Math/Infinity", new_node_float(INFINITY)));
	env->push_back_inplace(new_node_var("Math/NegativeInfinity", new_node_float(-INFINITY)));
	//new_node_var("Math/isNaN", new_node_native_function(&native_math_isnan, false)));
	//new_node_var("Math/isFinite", new_node_native_function(&native_math_isfinite, false)));
	//new_node_var("Math/isInteger", new_node_native_function(&native_math_isinteger, false)));
	//new_node_var("Math/isSafeInteger", new_node_native_function(&native_math_issafeinteger, false)));
	env->push_back_inplace(new_node_var("rand-int", new_node_native_function(&native_rand_int, false)));
	env->push_back_inplace(new_node_var("rand-float", new_node_native_function(&native_rand_float, false)));
	env->push_back_inplace(new_node_var("Time/now", new_node_native_function(&native_time_now, false)));
	





	/*
	env.set("def", new_node_native_function(&native_def));
	env.set("lambda", new_node_native_function(&native_lambda));
	env.set("call", new_node_native_function(&native_call));
	env.set("int", new_node_native_function(&native_int));
	env.set("float", new_node_native_function(&native_float));
	env.set("string", new_node_native_function(&native_string));
	env.set("list", new_node_native_function(&native_list));
	env.set("car", new_node_native_function(&native_car));
	env.set("cdr", new_node_native_function(&native_cdr));
	env.set("cons", new_node_native_function(&native_cons));
	env.set("nil?", new_node_native_function(&native_is_nil));
	env.set("int?", new_node_native_function(&native_is_int));
	env.set("float?", new_node_native_function(&native_is_float));
	env.set("string?", new_node_native_function(&native_is_string));
	env.set("list?", new_node_native_function(&native_is_list));
	env.set("car?", new_node_native_function(&native_is_car));
	env.set("cdr?", new_node_native_function(&native_is_cdr));
	env.set("cons?", new_node_native_function(&native_is_cons));
	env.set("native?", new_node_native_function(&native_is_native));
	env.set("function?", new_node_native_function(&native_is_function));
	env.set("native-function?", new_node_native_function(&native_is_native_function));
	*/

	//print_node_list(env, 0);
	
	//printf("Parsing...\n");

	// parse the base list
	list_ptr_t main_list = new_list();
	for(node_idx_t next = parse_next(fp, 0); next != NIL_NODE; next = parse_next(fp, next)) {
		main_list->push_back_inplace(next);
	}

	//print_node_list(main_list, 0);

	node_idx_t res_idx = eval_node_list(env, main_list);
	print_node(res_idx, 0);

	//print_node(root_idx, 0);

	printf("nodes.size() = %zu\n", nodes.size());
	printf("free_nodes.size() = %zu\n", free_nodes.size());

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
	//jo_float f("1.23456789");
	//jo_string f_str = f.to_string();
	//printf("f = %s\n", f_str.c_str());



	fclose(fp);
}

