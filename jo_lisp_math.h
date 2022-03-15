#pragma once

// TODO:
// o dot product
// o cross product
// o matrix type
// o tensor type

static node_idx_t native_math_abs(env_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->nth(0));
	if(n1->type == NODE_INT) {
		return new_node_int(abs(n1->t_int));
	} else {
		return new_node_float(fabs(n1->as_float()));
	}
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

void jo_lisp_math_init(env_ptr_t env) {
	env->set_inplace("even?", new_node_native_function(&native_is_even, false));
	env->set_inplace("odd?", new_node_native_function(&native_is_odd, false));
	env->set_inplace("pos?", new_node_native_function(&native_is_pos, false));
	env->set_inplace("neg?", new_node_native_function(&native_is_neg, false));
	env->set_inplace("Math/abs", new_node_native_function(&native_math_abs, false));
	env->set_inplace("Math/sqrt", new_node_native_function(&native_math_sqrt, false));
	env->set_inplace("Math/cbrt", new_node_native_function(&native_math_cbrt, false));
	env->set_inplace("Math/sin", new_node_native_function(&native_math_sin, false));
	env->set_inplace("Math/cos", new_node_native_function(&native_math_cos, false));
	env->set_inplace("Math/tan", new_node_native_function(&native_math_tan, false));
	env->set_inplace("Math/asin", new_node_native_function(&native_math_asin, false));
	env->set_inplace("Math/acos", new_node_native_function(&native_math_acos, false));
	env->set_inplace("Math/atan", new_node_native_function(&native_math_atan, false));
	env->set_inplace("Math/sinh", new_node_native_function(&native_math_sinh, false));
	env->set_inplace("Math/cosh", new_node_native_function(&native_math_cosh, false));
	env->set_inplace("Math/tanh", new_node_native_function(&native_math_tanh, false));
	env->set_inplace("Math/asinh", new_node_native_function(&native_math_asinh, false));
	env->set_inplace("Math/acosh", new_node_native_function(&native_math_acosh, false));
	env->set_inplace("Math/atanh", new_node_native_function(&native_math_atanh, false));
	env->set_inplace("Math/exp", new_node_native_function(&native_math_exp, false));
	env->set_inplace("Math/log", new_node_native_function(&native_math_log, false));
	env->set_inplace("Math/log10", new_node_native_function(&native_math_log10, false));
	env->set_inplace("Math/log2", new_node_native_function(&native_math_log2, false));
	env->set_inplace("Math/log1p", new_node_native_function(&native_math_log1p, false));
	env->set_inplace("Math/expm1", new_node_native_function(&native_math_expm1, false));
	env->set_inplace("Math/pow", new_node_native_function(&native_math_pow, false));
	env->set_inplace("Math/hypot", new_node_native_function(&native_math_hypot, false));
	env->set_inplace("Math/erf", new_node_native_function(&native_math_erf, false));
	env->set_inplace("Math/erfc", new_node_native_function(&native_math_erfc, false));
	env->set_inplace("Math/tgamma", new_node_native_function(&native_math_tgamma, false));
	env->set_inplace("Math/lgamma", new_node_native_function(&native_math_lgamma, false));
	env->set_inplace("Math/ceil", new_node_native_function(&native_math_ceil, false));
	env->set_inplace("Math/floor", new_node_native_function(&native_math_floor, false));
	env->set_inplace("Math/round", new_node_native_function(&native_math_round, false));
	env->set_inplace("Math/trunc", new_node_native_function(&native_math_trunc, false));
	env->set_inplace("Math/min", new_node_native_function(&native_math_min, false));
	env->set_inplace("Math/max", new_node_native_function(&native_math_max, false));
	env->set_inplace("Math/clamp", new_node_native_function(&native_math_clamp, false));
	env->set_inplace("Math/PI", new_node_float(JO_M_PI));
	env->set_inplace("Math/E", new_node_float(JO_M_E));
	env->set_inplace("Math/LN2", new_node_float(JO_M_LN2));
	env->set_inplace("Math/LN10", new_node_float(JO_M_LN10));
	env->set_inplace("Math/LOG2E", new_node_float(JO_M_LOG2E));
	env->set_inplace("Math/LOG10E", new_node_float(JO_M_LOG10E));
	env->set_inplace("Math/SQRT2", new_node_float(JO_M_SQRT2));
	env->set_inplace("Math/SQRT1_2", new_node_float(JO_M_SQRT1_2));
	env->set_inplace("Math/NaN", new_node_float(NAN));
	env->set_inplace("Math/Infinity", new_node_float(INFINITY));
	env->set_inplace("Math/NegativeInfinity", new_node_float(-INFINITY));
	//new_node_var("Math/isNaN", new_node_native_function(&native_math_isnan, false)));
	//new_node_var("Math/isFinite", new_node_native_function(&native_math_isfinite, false)));
	//new_node_var("Math/isInteger", new_node_native_function(&native_math_isinteger, false)));
	//new_node_var("Math/isSafeInteger", new_node_native_function(&native_math_issafeinteger, false)));
}
