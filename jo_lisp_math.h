#pragma once

// TODO:
// o dot product
// o cross product
// o matrix type
// o tensor type

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
		return ONE_NODE; 
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


static node_idx_t native_math_abs(env_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->nth(0));
	if(n1->type == NODE_INT) {
		return new_node_int(abs(n1->t_int));
	} else {
		return new_node_float(fabs(n1->as_float()));
	}
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


static node_idx_t native_math_sqrt(env_ptr_t env, list_ptr_t args) { return new_node_float(sqrt(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_cbrt(env_ptr_t env, list_ptr_t args) { return new_node_float(cbrt(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_ceil(env_ptr_t env, list_ptr_t args) { return new_node_int(ceil(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_floor(env_ptr_t env, list_ptr_t args) { return new_node_int(floor(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_exp(env_ptr_t env, list_ptr_t args) { return new_node_float(exp(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_exp2(env_ptr_t env, list_ptr_t args) { return new_node_float(exp2(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_hypot(env_ptr_t env, list_ptr_t args) { return new_node_float(hypot(get_node(args->nth(0))->as_float(), get_node(args->nth(1))->as_float())); }
static node_idx_t native_math_log10(env_ptr_t env, list_ptr_t args) { return new_node_float(log10(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_log(env_ptr_t env, list_ptr_t args) { return new_node_float(log(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_log2(env_ptr_t env, list_ptr_t args) { return new_node_float(log2(get_node(args->nth(0))->as_float()));}
static node_idx_t native_math_log1p(env_ptr_t env, list_ptr_t args) { return new_node_float(log1p(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_sin(env_ptr_t env, list_ptr_t args) { return new_node_float(sin(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_cos(env_ptr_t env, list_ptr_t args) { return new_node_float(cos(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_tan(env_ptr_t env, list_ptr_t args) { return new_node_float(tan(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_pow(env_ptr_t env, list_ptr_t args) { return new_node_float(pow(get_node(args->nth(0))->as_float(), get_node(args->nth(1))->as_float())); }
static node_idx_t native_math_sinh(env_ptr_t env, list_ptr_t args) { return new_node_float(sinh(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_cosh(env_ptr_t env, list_ptr_t args) { return new_node_float(cosh(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_tanh(env_ptr_t env, list_ptr_t args) { return new_node_float(tanh(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_asin(env_ptr_t env, list_ptr_t args) { return new_node_float(asin(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_acos(env_ptr_t env, list_ptr_t args) { return new_node_float(acos(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_atan(env_ptr_t env, list_ptr_t args) { return new_node_float(atan(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_asinh(env_ptr_t env, list_ptr_t args) { return new_node_float(asinh(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_acosh(env_ptr_t env, list_ptr_t args) { return new_node_float(acosh(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_atanh(env_ptr_t env, list_ptr_t args) { return new_node_float(atanh(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_erf(env_ptr_t env, list_ptr_t args) { return new_node_float(erf(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_erfc(env_ptr_t env, list_ptr_t args) { return new_node_float(erfc(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_tgamma(env_ptr_t env, list_ptr_t args) { return new_node_float(tgamma(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_lgamma(env_ptr_t env, list_ptr_t args) { return new_node_float(lgamma(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_round(env_ptr_t env, list_ptr_t args) { return new_node_int(round(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_trunc(env_ptr_t env, list_ptr_t args) { return new_node_int(trunc(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_logb(env_ptr_t env, list_ptr_t args) { return new_node_float(logb(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_ilogb(env_ptr_t env, list_ptr_t args) { return new_node_int(ilogb(get_node(args->nth(0))->as_float())); }
static node_idx_t native_math_expm1(env_ptr_t env, list_ptr_t args) { return new_node_float(expm1(get_node(args->nth(0))->as_float())); }

// Computes the minimum value of any number of arguments
static node_idx_t native_math_min(env_ptr_t env, list_ptr_t args) {
	// If there are no arguments, return nil
	if(args->size() == 0) {
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

static node_idx_t native_math_max(env_ptr_t env, list_ptr_t args) {
	// If there are no arguments, return nil
	if(args->size() == 0) {
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
static node_idx_t native_math_clamp(env_ptr_t env, list_ptr_t args) {
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

static node_idx_t native_is_even(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return new_node_bool((node->as_int() & 1) == 0);
}

static node_idx_t native_is_odd(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	node_idx_t node_idx = *it++;
	node_t *node = get_node(node_idx);
	return new_node_bool((node->as_int() & 1) == 1);
}

static node_idx_t native_is_pos(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    return new_node_bool(node->as_float() > 0);
}

static node_idx_t native_is_neg(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it = args->begin();
    node_idx_t node_idx = *it++;
    node_t *node = get_node(node_idx);
    return new_node_bool(node->as_float() < 0);
}

static node_idx_t native_bit_and(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	int n = get_node_int(*it++);
	for(; it; it++) {
		n &= get_node_int(*it);
	}
	return new_node_int(n);
}

static node_idx_t native_bit_or(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	int n = get_node_int(*it++);
	for(; it; it++) {
		n |= get_node_int(*it);
	}
	return new_node_int(n);
}

static node_idx_t native_bit_xor(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	int n = get_node_int(*it++);
	for(; it; it++) {
		n ^= get_node_int(*it);
	}
	return new_node_int(n);
}

static node_idx_t native_bit_not(env_ptr_t env, list_ptr_t args) {
	return new_node_int(~get_node_int(args->first_value()));
}

static node_idx_t native_bit_shift_left(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	int n = get_node_int(*it++);
	int n2 = get_node_int(*it++);
	return new_node_int(n << n2);
}

static node_idx_t native_bit_shift_right(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	int n = get_node_int(*it++);
	int n2 = get_node_int(*it++);
	return new_node_int(n >> n2);
}

static node_idx_t native_bit_extract(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it = args->begin();
	int n = get_node_int(*it++);
	int n2 = get_node_int(*it++);
	int n3 = get_node_int(*it++);
	return new_node_int((n >> n2) & ((1 << n3) - 1));
}

// (bit-override dst src pos len)
// Override len bits in dst starting from pos using bits from src.
static node_idx_t native_bit_override(env_ptr_t env, list_ptr_t args) {	
	list_t::iterator it = args->begin();
	int dst = get_node_int(*it++);
	int src = get_node_int(*it++);
	int pos = get_node_int(*it++);
	int len = get_node_int(*it++);
	int mask = ((1 << len) - 1) << pos;
	return new_node_int((dst & ~mask) | (src & mask));
}

void jo_lisp_math_init(env_ptr_t env) {
	env->set("+", new_node_native_function("+", &native_add, false));
	env->set("-", new_node_native_function("-", &native_sub, false));
	env->set("*", new_node_native_function("*", &native_mul, false));
	env->set("/", new_node_native_function("/", &native_div, false));
	env->set("mod", new_node_native_function("mod", &native_mod, false));
	env->set("inc", new_node_native_function("inc", &native_inc, false));
	env->set("dec", new_node_native_function("dec", &native_dec, false));
	env->set("rand-int", new_node_native_function("rand-int", &native_rand_int, false));
	env->set("rand-float", new_node_native_function("rand-float", &native_rand_float, false));
	env->set("even?", new_node_native_function("even?", &native_is_even, false));
	env->set("odd?", new_node_native_function("odd?", &native_is_odd, false));
	env->set("pos?", new_node_native_function("pos?", &native_is_pos, false));
	env->set("neg?", new_node_native_function("neg?", &native_is_neg, false));
	env->set("bit-and", new_node_native_function("bit-and", &native_bit_and, false));
	env->set("bit-or", new_node_native_function("bit-or", &native_bit_or, false));
	env->set("bit-xor", new_node_native_function("bit-xor", &native_bit_xor, false));
	env->set("bit-not", new_node_native_function("bit-not", &native_bit_not, false));
	env->set("bit-shift-left", new_node_native_function("bit-shift-left", &native_bit_shift_left, false));
	env->set("bit-shift-right", new_node_native_function("bit-shift-right", &native_bit_shift_right, false));
	env->set("bit-extract", new_node_native_function("bit-extract", &native_bit_extract, false));
	env->set("bit-override", new_node_native_function("bit-override", &native_bit_override, false));
	env->set("Math/abs", new_node_native_function("Math/abs", &native_math_abs, false));
	env->set("Math/sqrt", new_node_native_function("Math/sqrt", &native_math_sqrt, false));
	env->set("Math/cbrt", new_node_native_function("Math/cbrt", &native_math_cbrt, false));
	env->set("Math/sin", new_node_native_function("Math/sin", &native_math_sin, false));
	env->set("Math/cos", new_node_native_function("Math/cos", &native_math_cos, false));
	env->set("Math/tan", new_node_native_function("Math/tan", &native_math_tan, false));
	env->set("Math/asin", new_node_native_function("Math/asin", &native_math_asin, false));
	env->set("Math/acos", new_node_native_function("Math/acos", &native_math_acos, false));
	env->set("Math/atan", new_node_native_function("Math/atan", &native_math_atan, false));
	env->set("Math/sinh", new_node_native_function("Math/sinh", &native_math_sinh, false));
	env->set("Math/cosh", new_node_native_function("Math/cosh", &native_math_cosh, false));
	env->set("Math/tanh", new_node_native_function("Math/tanh", &native_math_tanh, false));
	env->set("Math/asinh", new_node_native_function("Math/asinh", &native_math_asinh, false));
	env->set("Math/acosh", new_node_native_function("Math/acosh", &native_math_acosh, false));
	env->set("Math/atanh", new_node_native_function("Math/atanh", &native_math_atanh, false));
	env->set("Math/exp", new_node_native_function("Math/exp", &native_math_exp, false));
	env->set("Math/log", new_node_native_function("Math/log", &native_math_log, false));
	env->set("Math/log10", new_node_native_function("Math/log10", &native_math_log10, false));
	env->set("Math/log2", new_node_native_function("Math/log2", &native_math_log2, false));
	env->set("Math/log1p", new_node_native_function("Math/log1p", &native_math_log1p, false));
	env->set("Math/expm1", new_node_native_function("Math/expm1", &native_math_expm1, false));
	env->set("Math/pow", new_node_native_function("Math/pow", &native_math_pow, false));
	env->set("Math/hypot", new_node_native_function("Math/hypot", &native_math_hypot, false));
	env->set("Math/erf", new_node_native_function("Math/erf", &native_math_erf, false));
	env->set("Math/erfc", new_node_native_function("Math/erfc", &native_math_erfc, false));
	env->set("Math/tgamma", new_node_native_function("Math/tgamma", &native_math_tgamma, false));
	env->set("Math/lgamma", new_node_native_function("Math/lgamma", &native_math_lgamma, false));
	env->set("Math/ceil", new_node_native_function("Math/ceil", &native_math_ceil, false));
	env->set("Math/floor", new_node_native_function("Math/floor", &native_math_floor, false));
	env->set("Math/round", new_node_native_function("Math/round", &native_math_round, false));
	env->set("Math/trunc", new_node_native_function("Math/trunc", &native_math_trunc, false));
	env->set("Math/min", new_node_native_function("Math/min", &native_math_min, false));
	env->set("Math/max", new_node_native_function("Math/max", &native_math_max, false));
	env->set("Math/clamp", new_node_native_function("Math/clamp", &native_math_clamp, false));
	env->set("Math/PI", new_node_float(JO_M_PI));
	env->set("Math/E", new_node_float(JO_M_E));
	env->set("Math/LN2", new_node_float(JO_M_LN2));
	env->set("Math/LN10", new_node_float(JO_M_LN10));
	env->set("Math/LOG2E", new_node_float(JO_M_LOG2E));
	env->set("Math/LOG10E", new_node_float(JO_M_LOG10E));
	env->set("Math/SQRT2", new_node_float(JO_M_SQRT2));
	env->set("Math/SQRT1_2", new_node_float(JO_M_SQRT1_2));
	env->set("Math/NaN", new_node_float(NAN));
	env->set("Math/Infinity", new_node_float(INFINITY));
	env->set("Math/NegativeInfinity", new_node_float(-INFINITY));
	//new_node_var("Math/isNaN", new_node_native_function(&native_math_isnan, false)));
	//new_node_var("Math/isFinite", new_node_native_function(&native_math_isfinite, false)));
	//new_node_var("Math/isInteger", new_node_native_function(&native_math_isinteger, false)));
	//new_node_var("Math/isSafeInteger", new_node_native_function(&native_math_issafeinteger, false)));
}
