#pragma once

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

void jo_lisp_math_init(list_ptr_t env) {
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
}
