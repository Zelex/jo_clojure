#pragma once

// TODO:
// o dot product
// o cross product
// o matrix type
// o tensor type
// o check for divide by 0!

// native function to add any number of arguments
static node_idx_t native_add(env_ptr_t env, list_ptr_t args) { 
	long long i = 0;
	double d = 0.0;
	for(list_t::iterator it(args); it; it++) {
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

static node_idx_t native_add_int(env_ptr_t env, list_ptr_t args) { 
	long long i = 0;
	for(list_t::iterator it(args); it; it++) {
		node_t *n = get_node(*it);
		if(n->type == NODE_INT) {
			i += n->t_int;
		} else {
			i += n->as_int();
		}
	}
	return new_node_int(i);
}

// subtract any number of arguments from the first argument
static node_idx_t native_sub(env_ptr_t env, list_ptr_t args) {
	long long i_sum = 0;
	double d_sum = 0.0;

	size_t size = args->size();
	if(size == 0) return ZERO_NODE;

	// Special case. 1 argument return the negative of that argument
	if(size == 1) {
		node_t *n = get_node(args->first_value());
		if(n->type == NODE_INT) return new_node_int(-n->t_int);
		return new_node_float(-n->as_float());
	}

	list_t::iterator i(args);
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

static node_idx_t native_sub_int(env_ptr_t env, list_ptr_t args) {
	long long i_sum = 0;

	size_t size = args->size();
	if(size == 0) return ZERO_NODE;

	// Special case. 1 argument return the negative of that argument
	if(size == 1) {
		node_t *n = get_node(args->first_value());
		if(n->type == NODE_INT) {
			return new_node_int(-n->t_int);
		}
		return new_node_int(-n->as_int());
	}

	list_t::iterator i(args);
	node_t *n = get_node(*i++);
	if(n->type == NODE_INT) {
		i_sum = n->t_int;
	} else {
		i_sum = n->as_int();
	}

	for(; i; i++) {
		n = get_node(*i);
		if(n->type == NODE_INT) {
			i_sum -= n->t_int;
		} else {
			i_sum -= n->as_int();
		}
	}
	return new_node_int(i_sum);
}

static node_idx_t native_mul(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) return ONE_NODE; 

	long long i = 1;
	double d = 1.0;
	for(list_t::iterator it(args); it; it++) {
		node_t *n = get_node(*it);
		int type = n->type;
		if(type == NODE_INT) 		i *= n->t_int;
		else if(type == NODE_FLOAT) d *= n->t_float;
		else 						d *= n->as_float();
	}
	return d == 1.0 ? new_node_int(i) : new_node_float(d * i);
}

static node_idx_t native_fma(env_ptr_t env, list_ptr_t args) {
	if(args->size() < 3) return ZERO_NODE;
	list_t::iterator i(args);
	node_idx_t a = *i++, b = *i++, c = *i++;
	return node_fma(a, b, c);
}

static node_idx_t native_mul_int(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) return ONE_NODE; 

	long long i = 1;
	for(list_t::iterator it(args); it; it++) {
		node_t *n = get_node(*it);
		int type = n->type;
		if(type == NODE_INT) 	i *= n->t_int;
		else					i *= n->as_int();
	}
	return new_node_int(i);
}

// divide any number of arguments from the first argument
static node_idx_t native_div(env_ptr_t env, list_ptr_t args) {
	long long i_sum = 1;
	double d_sum = 1.0;
	bool is_int = true;

	size_t size = args->size();
	if(size == 0) return ONE_NODE;

	// special case of 1 argument, compute 1.0 / value
	if(size == 1) {
		node_t *n = get_node(args->first_value());
		return new_node_float(1.0 / n->as_float());
	}

	list_t::iterator i(args);
	node_t *n = get_node(*i++);
	if(n->type == NODE_INT) {
		i_sum = n->t_int;
	} else {
		d_sum = n->as_float();
		is_int = false;
	}

	for(; i; i++) {
		n = get_node(*i);
		if(is_int && n->type == NODE_INT) {
			i_sum /= n->t_int;
			d_sum = i_sum;
		} else {
			d_sum /= n->as_float();
			is_int = false;
		}
	}

	return is_int ? new_node_int(i_sum) : new_node_float(d_sum);
}

static node_idx_t native_div_int(env_ptr_t env, list_ptr_t args) {
	long long i_sum = 1;

	size_t size = args->size();
	if(size == 0) return ONE_NODE;

	// special case of 1 argument, compute 1.0 / value
	if(size == 1) {
		node_t *n = get_node(args->first_value());
		i_sum = n->as_int();
		return i_sum == 1 ? ONE_NODE : i_sum == -1 ? new_node_int(-1) : ZERO_NODE;
	}

	list_t::iterator i(args);
	node_t *n = get_node(*i++);
	if(n->type == NODE_INT) {
		i_sum = n->t_int;
	} else {
		i_sum = n->as_int();
	}

	for(; i; i++) {
		n = get_node(*i);
		if(n->type == NODE_INT) {
			i_sum /= n->t_int;
		} else {
			i_sum /= n->as_int();
		}
	}

	return new_node_int(i_sum);
}

static node_idx_t native_mod(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) return ZERO_NODE;
	node_t *n = get_node(args->first_value());
	if(n->type == NODE_INT) return new_node_int(n->t_int % get_node(args->second_value())->as_int());
	return new_node_float(fmod(n->as_float(), get_node(args->second_value())->as_float()));
}

static node_idx_t native_remainder(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) return ZERO_NODE;
	node_t *n = get_node(args->first_value());
	if(n->type == NODE_INT) return new_node_int(n->t_int % get_node(args->second_value())->as_int());
	return new_node_float(remainder(n->as_float(), get_node(args->second_value())->as_float()));
}

static node_idx_t native_remainder_int(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) return ZERO_NODE;
	node_t *n = get_node(args->first_value());
	if(n->type == NODE_INT) return new_node_int(n->t_int % get_node(args->second_value())->as_int());
	return new_node_int(n->as_int() % get_node(args->second_value())->as_int());
}

static node_idx_t native_math_abs(env_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->first_value());
	if(n1->type == NODE_INT) return new_node_int(abs(n1->t_int));
	return new_node_float(fabs(n1->as_float()));
}

static node_idx_t native_inc(env_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->first_value());
	if(n1->type == NODE_INT) return new_node_int(n1->t_int + 1);
	return new_node_float(n1->as_float() + 1.0f);
}

static node_idx_t native_inc_int(env_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->first_value());
	if(n1->type == NODE_INT) return new_node_int(n1->t_int + 1);
	return new_node_int(n1->as_int() + 1);
}

static node_idx_t native_dec(env_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->first_value());
	if(n1->type == NODE_INT) return new_node_int(n1->t_int - 1);
	return new_node_float(n1->as_float() - 1.0f);
}

static node_idx_t native_dec_int(env_ptr_t env, list_ptr_t args) {
	node_t *n1 = get_node(args->first_value());
	if(n1->type == NODE_INT) return new_node_int(n1->t_int - 1);
	return new_node_int(n1->as_int() - 1);
}

static node_idx_t native_rand_int(env_ptr_t env, list_ptr_t args) { return new_node_int(rand() % get_node_int(args->first_value())); }
static node_idx_t native_rand(env_ptr_t env, list_ptr_t args) { 
	double bias = 0.0;
	double scale = 1.0 / RAND_MAX;
	if(args->size() == 1) {
		scale = get_node_float(args->first_value()) / RAND_MAX;
	} else if(args->size() == 2) {
		// compute scale/bias for random value between first and second number
		double A = get_node_float(args->first_value());
		double B = get_node_float(args->second_value());
		bias = A;
		scale = (B - A) / RAND_MAX;
	}
	return new_node_float(bias + rand() * scale); 
}
static node_idx_t native_math_sqrt(env_ptr_t env, list_ptr_t args) { return new_node_float(sqrt(get_node_float(args->first_value()))); }
static node_idx_t native_math_cbrt(env_ptr_t env, list_ptr_t args) { return new_node_float(cbrt(get_node_float(args->first_value()))); }
static node_idx_t native_math_ceil(env_ptr_t env, list_ptr_t args) { return new_node_int(ceil(get_node_float(args->first_value()))); }
static node_idx_t native_math_floor(env_ptr_t env, list_ptr_t args) { return new_node_int(floor(get_node_float(args->first_value()))); }
static node_idx_t native_math_exp(env_ptr_t env, list_ptr_t args) { return new_node_float(exp(get_node_float(args->first_value()))); }
static node_idx_t native_math_exp2(env_ptr_t env, list_ptr_t args) { return new_node_float(exp2(get_node_float(args->first_value()))); }
static node_idx_t native_math_hypot(env_ptr_t env, list_ptr_t args) { return new_node_float(hypot(get_node_float(args->first_value()), get_node_float(args->second_value()))); }
static node_idx_t native_math_log10(env_ptr_t env, list_ptr_t args) { return new_node_float(log10(get_node_float(args->first_value()))); }
static node_idx_t native_math_log(env_ptr_t env, list_ptr_t args) { return new_node_float(log(get_node_float(args->first_value()))); }
static node_idx_t native_math_log2(env_ptr_t env, list_ptr_t args) { return new_node_float(log2(get_node_float(args->first_value())));}
static node_idx_t native_math_log1p(env_ptr_t env, list_ptr_t args) { return new_node_float(log1p(get_node_float(args->first_value()))); }
static node_idx_t native_math_sin(env_ptr_t env, list_ptr_t args) { return new_node_float(sin(get_node_float(args->first_value()))); }
static node_idx_t native_math_cos(env_ptr_t env, list_ptr_t args) { return new_node_float(cos(get_node_float(args->first_value()))); }
static node_idx_t native_math_tan(env_ptr_t env, list_ptr_t args) { return new_node_float(tan(get_node_float(args->first_value()))); }
static node_idx_t native_math_pow(env_ptr_t env, list_ptr_t args) { return new_node_float(pow(get_node_float(args->first_value()), get_node_float(args->second_value()))); }
static node_idx_t native_math_sinh(env_ptr_t env, list_ptr_t args) { return new_node_float(sinh(get_node_float(args->first_value()))); }
static node_idx_t native_math_cosh(env_ptr_t env, list_ptr_t args) { return new_node_float(cosh(get_node_float(args->first_value()))); }
static node_idx_t native_math_tanh(env_ptr_t env, list_ptr_t args) { return new_node_float(tanh(get_node_float(args->first_value()))); }
static node_idx_t native_math_asin(env_ptr_t env, list_ptr_t args) { return new_node_float(asin(get_node_float(args->first_value()))); }
static node_idx_t native_math_acos(env_ptr_t env, list_ptr_t args) { return new_node_float(acos(get_node_float(args->first_value()))); }
static node_idx_t native_math_atan(env_ptr_t env, list_ptr_t args) { return new_node_float(atan(get_node_float(args->first_value()))); }
static node_idx_t native_math_atan2(env_ptr_t env, list_ptr_t args) { return new_node_float(atan2(get_node_float(args->first_value()), get_node_float(args->second_value()))); }
static node_idx_t native_math_asinh(env_ptr_t env, list_ptr_t args) { return new_node_float(asinh(get_node_float(args->first_value()))); }
static node_idx_t native_math_acosh(env_ptr_t env, list_ptr_t args) { return new_node_float(acosh(get_node_float(args->first_value()))); }
static node_idx_t native_math_atanh(env_ptr_t env, list_ptr_t args) { return new_node_float(atanh(get_node_float(args->first_value()))); }
static node_idx_t native_math_erf(env_ptr_t env, list_ptr_t args) { return new_node_float(erf(get_node_float(args->first_value()))); }
static node_idx_t native_math_erfc(env_ptr_t env, list_ptr_t args) { return new_node_float(erfc(get_node_float(args->first_value()))); }
static node_idx_t native_math_tgamma(env_ptr_t env, list_ptr_t args) { return new_node_float(tgamma(get_node_float(args->first_value()))); }
static node_idx_t native_math_lgamma(env_ptr_t env, list_ptr_t args) { return new_node_float(lgamma(get_node_float(args->first_value()))); }
static node_idx_t native_math_round(env_ptr_t env, list_ptr_t args) { return new_node_int(round(get_node_float(args->first_value()))); }
static node_idx_t native_math_trunc(env_ptr_t env, list_ptr_t args) { return new_node_int(trunc(get_node_float(args->first_value()))); }
static node_idx_t native_math_logb(env_ptr_t env, list_ptr_t args) { return new_node_float(logb(get_node_float(args->first_value()))); }
static node_idx_t native_math_ilogb(env_ptr_t env, list_ptr_t args) { return new_node_int(ilogb(get_node_float(args->first_value()))); }
static node_idx_t native_math_expm1(env_ptr_t env, list_ptr_t args) { return new_node_float(expm1(get_node_float(args->first_value()))); }
static node_idx_t native_is_even(env_ptr_t env, list_ptr_t args) { return new_node_bool((get_node_int(args->first_value()) & 1) == 0); }
static node_idx_t native_is_odd(env_ptr_t env, list_ptr_t args) { return new_node_bool((get_node_int(args->first_value()) & 1) == 1); }
static node_idx_t native_is_pos(env_ptr_t env, list_ptr_t args) { return new_node_bool(get_node_float(args->first_value()) > 0); }
static node_idx_t native_is_neg(env_ptr_t env, list_ptr_t args) { return new_node_bool(get_node_float(args->first_value()) < 0); }
static node_idx_t native_is_pos_int(env_ptr_t env, list_ptr_t args) { node_t *n = get_node(args->first_value()); return new_node_bool(n->is_int() && n->as_int() > 0); }
static node_idx_t native_is_neg_int(env_ptr_t env, list_ptr_t args) { node_t *n = get_node(args->first_value()); return new_node_bool(n->is_int() && n->as_int() < 0); }
static node_idx_t native_bit_not(env_ptr_t env, list_ptr_t args) { return new_node_int(~get_node_int(args->first_value())); }
static node_idx_t native_bit_shift_left(env_ptr_t env, list_ptr_t args) { return new_node_int(get_node_int(args->first_value()) << get_node_int(args->second_value())); }
static node_idx_t native_bit_shift_right(env_ptr_t env, list_ptr_t args) { return new_node_int(get_node_int(args->first_value()) >> get_node_int(args->second_value())); }
static node_idx_t native_unsigned_bit_shift_right(env_ptr_t env, list_ptr_t args) { return new_node_int((unsigned)get_node_int(args->first_value()) >> get_node_int(args->second_value())); }
static node_idx_t native_bit_extract(env_ptr_t env, list_ptr_t args) { return new_node_int((get_node_int(args->first_value()) >> get_node_int(args->second_value())) & ((1 << get_node_int(args->third_value())) - 1)); }
static node_idx_t native_bit_clear(env_ptr_t env, list_ptr_t args) { return new_node_int(get_node_int(args->first_value()) & ~(1 << get_node_int(args->second_value()))); }
static node_idx_t native_bit_flip(env_ptr_t env, list_ptr_t args) { return new_node_int(get_node_int(args->first_value()) ^ (1 << get_node_int(args->second_value()))); }
static node_idx_t native_bit_set(env_ptr_t env, list_ptr_t args) { return new_node_int(get_node_int(args->first_value()) | (1 << get_node_int(args->second_value()))); }
static node_idx_t native_bit_test(env_ptr_t env, list_ptr_t args) { return new_node_bool((get_node_int(args->first_value()) & (1 << get_node_int(args->second_value()))) != 0); }
static node_idx_t native_math_to_degrees(env_ptr_t env, list_ptr_t args) { return new_node_float(get_node_float(args->first_value()) * 180.0f / JO_M_PI); }
static node_idx_t native_math_to_radians(env_ptr_t env, list_ptr_t args) { return new_node_float(get_node_float(args->first_value()) * JO_M_PI / 180.0f); }

// Computes the minimum value of any number of arguments
static node_idx_t native_math_min(env_ptr_t env, list_ptr_t args) {
	if(args->size() == 0) return ZERO_NODE;

	list_t::iterator it(args);
	node_idx_t min_node = *it++;
	if(args->size() == 1) return min_node;

	node_t *n = get_node(min_node);
	if(n->type == NODE_INT) {
		bool is_int = true;
		long long min_int = n->t_int;
		float min_float = min_int;
		node_idx_t next = *it++;
		while(true) {
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
			if(!it) {
				break;
			}
			next = *it++;
		}
		return min_node;
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
	if(args->size() == 0) return ZERO_NODE;
	
	list_t::iterator it(args);
	
	// Get the first argument
	node_idx_t max_node = *it++;
	if(args->size() == 1) return max_node;

	node_t *n = get_node(max_node);
	if(n->type == NODE_INT) {
		bool is_int = true;
		long long max_int = n->t_int;
		float max_float = max_int;
		node_idx_t next = *it++;
		while(true) {
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
			if(!it) {
				break;
			}
			next = *it++;
		}
		return max_node;
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

static node_idx_t native_math_clip(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	node_t *n1 = get_node(*it++);
	node_t *n2 = get_node(*it++);
	node_t *n3 = get_node(*it++);
	if(n1->type == NODE_INT && n2->type == NODE_INT && n3->type == NODE_INT) {
		long long val = n1->t_int;
		long long min = n2->t_int;
		long long max = n3->t_int;
		val = val < min ? min : val > max ? max : val;
		return new_node_int(val);
	}
	float val = n1->as_float();
	float min = n2->as_float();
	float max = n3->as_float();
	val = val < min ? min : val > max ? max : val;
	return new_node_float(val);
}

static node_idx_t native_bit_and(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	long long n = get_node_int(*it++);
	for(; it; it++) {
		n &= get_node_int(*it);
	}
	return new_node_int(n);
}

static node_idx_t native_bit_and_not(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	long long n = get_node_int(*it++);
	for(; it; it++) {
		n &= ~get_node_int(*it);
	}
	return new_node_int(n);
}

static node_idx_t native_bit_or(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	long long n = get_node_int(*it++);
	for(; it; it++) {
		n |= get_node_int(*it);
	}
	return new_node_int(n);
}

static node_idx_t native_bit_xor(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	long long n = get_node_int(*it++);
	for(; it; it++) {
		n ^= get_node_int(*it);
	}
	return new_node_int(n);
}

// (bit-override dst src pos len)
// Override len bits in dst starting from pos using bits from src.
static node_idx_t native_bit_override(env_ptr_t env, list_ptr_t args) {	
	list_t::iterator it(args);
	long long dst = get_node_int(*it++);
	long long src = get_node_int(*it++);
	long long pos = get_node_int(*it++);
	long long len = get_node_int(*it++);
	long long mask = ((1 << len) - 1) << pos;
	return new_node_int((dst & ~mask) | (src & mask));
}

static node_idx_t native_math_interp(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	float x = get_node_float(*it++);
	float x0 = get_node_float(*it++);
	float x1 = get_node_float(*it++);
	float y0 = get_node_float(*it++);
	float y1 = get_node_float(*it++);
	return new_node_float(y0 + (y1 - y0) * (x - x0) / (x1 - x0));
}

static node_idx_t native_float(env_ptr_t env, list_ptr_t args) { return new_node_float(get_node_float(args->first_value())); }
static node_idx_t native_double(env_ptr_t env, list_ptr_t args) { return new_node_float(get_node_float(args->first_value())); }
static node_idx_t native_int(env_ptr_t env, list_ptr_t args) { return new_node_int(get_node(args->first_value())->as_int()); }

static node_idx_t native_is_float(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_FLOAT ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_double(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_FLOAT ? TRUE_NODE : FALSE_NODE; }
static node_idx_t native_is_int(env_ptr_t env, list_ptr_t args) { return get_node_type(args->first_value()) == NODE_INT ? TRUE_NODE : FALSE_NODE; }

static node_idx_t native_math_quantize(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	float x = get_node_float(*it++);
	float step = get_node_float(*it++);
	return new_node_float(roundf(x / step) * step);
}

// Calculates the average of all the values in the list.
static node_idx_t native_math_average(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	float sum = 0;
	for(; it; it++) {
		sum += get_node_float(*it);
	}
	return new_node_float(sum / args->size());
}

void jo_clojure_math_init(env_ptr_t env) {
	env->set("int", new_node_native_function("int", &native_int, false, NODE_FLAG_PRERESOLVE));
	env->set("int?", new_node_native_function("int?", &native_is_int, false, NODE_FLAG_PRERESOLVE));
	env->set("integer?", new_node_native_function("integer?", &native_is_int, false, NODE_FLAG_PRERESOLVE));
	env->set("float", new_node_native_function("float", &native_float, false, NODE_FLAG_PRERESOLVE));
	env->set("float?", new_node_native_function("float?", &native_is_float, false, NODE_FLAG_PRERESOLVE));
	env->set("double", new_node_native_function("double", &native_double, false, NODE_FLAG_PRERESOLVE));
	env->set("double?", new_node_native_function("double?", &native_is_double, false, NODE_FLAG_PRERESOLVE));

	env->set("+", new_node_native_function("+", &native_add, false, NODE_FLAG_PRERESOLVE));
	env->set("-", new_node_native_function("-", &native_sub, false, NODE_FLAG_PRERESOLVE));
	env->set("*", new_node_native_function("*", &native_mul, false, NODE_FLAG_PRERESOLVE));
	env->set("*+", new_node_native_function("*+", &native_fma, false, NODE_FLAG_PRERESOLVE));
	env->set("/", new_node_native_function("/", &native_div, false, NODE_FLAG_PRERESOLVE));
	env->set("rem", new_node_native_function("rem", &native_remainder, false, NODE_FLAG_PRERESOLVE));
	env->set("mod", new_node_native_function("mod", &native_mod, false, NODE_FLAG_PRERESOLVE));
	env->set("inc", new_node_native_function("inc", &native_inc, false, NODE_FLAG_PRERESOLVE));
	env->set("dec", new_node_native_function("dec", &native_dec, false, NODE_FLAG_PRERESOLVE));
	env->set("rand-int", new_node_native_function("rand-int", &native_rand_int, false, NODE_FLAG_PRERESOLVE));
	env->set("rand", new_node_native_function("rand", &native_rand, false, NODE_FLAG_PRERESOLVE));
	env->set("even?", new_node_native_function("even?", &native_is_even, false, NODE_FLAG_PRERESOLVE));
	env->set("odd?", new_node_native_function("odd?", &native_is_odd, false, NODE_FLAG_PRERESOLVE));
	env->set("pos?", new_node_native_function("pos?", &native_is_pos, false, NODE_FLAG_PRERESOLVE));
	env->set("neg?", new_node_native_function("neg?", &native_is_neg, false, NODE_FLAG_PRERESOLVE));
	env->set("pos-int?", new_node_native_function("pos-int?", &native_is_pos_int, false, NODE_FLAG_PRERESOLVE));
	env->set("neg-int?", new_node_native_function("neg-int?", &native_is_neg_int, false, NODE_FLAG_PRERESOLVE));
	env->set("nat-int?", new_node_native_function("nat-int?", &native_is_pos_int, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-and", new_node_native_function("bit-and", &native_bit_and, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-and-not", new_node_native_function("bit-and-not", &native_bit_and_not, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-or", new_node_native_function("bit-or", &native_bit_or, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-xor", new_node_native_function("bit-xor", &native_bit_xor, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-not", new_node_native_function("bit-not", &native_bit_not, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-shift-left", new_node_native_function("bit-shift-left", &native_bit_shift_left, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-shift-right", new_node_native_function("bit-shift-right", &native_bit_shift_right, false, NODE_FLAG_PRERESOLVE));
	env->set("unsigned-bit-shift-right", new_node_native_function("unsigned-bit-shift-right", &native_unsigned_bit_shift_right, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-extract", new_node_native_function("bit-extract", &native_bit_extract, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-override", new_node_native_function("bit-override", &native_bit_override, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-clear", new_node_native_function("bit-clear", &native_bit_clear, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-flip", new_node_native_function("bit-flip", &native_bit_flip, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-set", new_node_native_function("bit-set", &native_bit_set, false, NODE_FLAG_PRERESOLVE));
	env->set("bit-test", new_node_native_function("bit-test", &native_bit_test, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/abs", new_node_native_function("Math/abs", &native_math_abs, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/sqrt", new_node_native_function("Math/sqrt", &native_math_sqrt, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/cbrt", new_node_native_function("Math/cbrt", &native_math_cbrt, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/sin", new_node_native_function("Math/sin", &native_math_sin, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/cos", new_node_native_function("Math/cos", &native_math_cos, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/tan", new_node_native_function("Math/tan", &native_math_tan, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/asin", new_node_native_function("Math/asin", &native_math_asin, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/acos", new_node_native_function("Math/acos", &native_math_acos, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/atan", new_node_native_function("Math/atan", &native_math_atan, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/atan2", new_node_native_function("Math/atan2", &native_math_atan2, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/sinh", new_node_native_function("Math/sinh", &native_math_sinh, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/cosh", new_node_native_function("Math/cosh", &native_math_cosh, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/tanh", new_node_native_function("Math/tanh", &native_math_tanh, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/asinh", new_node_native_function("Math/asinh", &native_math_asinh, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/acosh", new_node_native_function("Math/acosh", &native_math_acosh, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/atanh", new_node_native_function("Math/atanh", &native_math_atanh, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/exp", new_node_native_function("Math/exp", &native_math_exp, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/log", new_node_native_function("Math/log", &native_math_log, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/log10", new_node_native_function("Math/log10", &native_math_log10, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/log2", new_node_native_function("Math/log2", &native_math_log2, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/log1p", new_node_native_function("Math/log1p", &native_math_log1p, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/expm1", new_node_native_function("Math/expm1", &native_math_expm1, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/pow", new_node_native_function("Math/pow", &native_math_pow, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/hypot", new_node_native_function("Math/hypot", &native_math_hypot, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/erf", new_node_native_function("Math/erf", &native_math_erf, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/erfc", new_node_native_function("Math/erfc", &native_math_erfc, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/tgamma", new_node_native_function("Math/tgamma", &native_math_tgamma, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/lgamma", new_node_native_function("Math/lgamma", &native_math_lgamma, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/ceil", new_node_native_function("Math/ceil", &native_math_ceil, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/floor", new_node_native_function("Math/floor", &native_math_floor, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/round", new_node_native_function("Math/round", &native_math_round, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/trunc", new_node_native_function("Math/trunc", &native_math_trunc, false, NODE_FLAG_PRERESOLVE));
	env->set("min", new_node_native_function("min", &native_math_min, false, NODE_FLAG_PRERESOLVE));
	env->set("max", new_node_native_function("max", &native_math_max, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/clip", new_node_native_function("Math/clip", &native_math_clip, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/to-degrees", new_node_native_function("Math/to-degrees", &native_math_to_degrees, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/to-radians", new_node_native_function("Math/to-radians", &native_math_to_radians, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/interp", new_node_native_function("Math/interp", &native_math_interp, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/quantize", new_node_native_function("Math/quantize", &native_math_quantize, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/average", new_node_native_function("Math/average", &native_math_average, false, NODE_FLAG_PRERESOLVE));
	env->set("Math/PI", new_node_float(JO_M_PI, NODE_FLAG_PRERESOLVE));
	env->set("Math/E", new_node_float(JO_M_E, NODE_FLAG_PRERESOLVE));
	env->set("Math/LN2", new_node_float(JO_M_LN2, NODE_FLAG_PRERESOLVE));
	env->set("Math/LN10", new_node_float(JO_M_LN10, NODE_FLAG_PRERESOLVE));
	env->set("Math/LOG2E", new_node_float(JO_M_LOG2E, NODE_FLAG_PRERESOLVE));
	env->set("Math/LOG10E", new_node_float(JO_M_LOG10E, NODE_FLAG_PRERESOLVE));
	env->set("Math/SQRT2", new_node_float(JO_M_SQRT2, NODE_FLAG_PRERESOLVE));
	env->set("Math/SQRT1_2", new_node_float(JO_M_SQRT1_2, NODE_FLAG_PRERESOLVE));
	env->set("Math/NaN", new_node_float(NAN, NODE_FLAG_PRERESOLVE));
	env->set("Math/Infinity", new_node_float(INFINITY, NODE_FLAG_PRERESOLVE));
	env->set("Math/NegativeInfinity", new_node_float(-INFINITY, NODE_FLAG_PRERESOLVE));
	//new_node_var("Math/isNaN", new_node_native_function(&native_math_isnan, false, NODE_FLAG_PRERESOLVE)));
	//new_node_var("Math/isFinite", new_node_native_function(&native_math_isfinite, false, NODE_FLAG_PRERESOLVE)));
	//new_node_var("Math/isInteger", new_node_native_function(&native_math_isinteger, false, NODE_FLAG_PRERESOLVE)));
	//new_node_var("Math/isSafeInteger", new_node_native_function(&native_math_issafeinteger, false, NODE_FLAG_PRERESOLVE)));

	// These are the same in this clojure...
	env->set("unchecked-add", new_node_native_function("unchecked-add", &native_add, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-add-int", new_node_native_function("unchecked-add-int", &native_add_int, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-subtract", new_node_native_function("unchecked-subtract", &native_sub, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-subtract-int", new_node_native_function("unchecked-subtract-int", &native_sub_int, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-negate", new_node_native_function("unchecked-negate", &native_sub, false, NODE_FLAG_PRERESOLVE)); // TODO: This is the same, right?
	env->set("unchecked-negate-int", new_node_native_function("unchecked-negate-int", &native_sub_int, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-multiply", new_node_native_function("unchecked-multiply", &native_mul, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-multiply-int", new_node_native_function("unchecked-multiply-int", &native_mul_int, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-divide", new_node_native_function("unchecked-divide", &native_div, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-divide-int", new_node_native_function("unchecked-divide-int", &native_div_int, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-inc", new_node_native_function("unchecked-inc", &native_inc, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-inc-int", new_node_native_function("unchecked-inc-int", &native_inc_int, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-dec", new_node_native_function("unchecked-dec", &native_dec, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-dec-int", new_node_native_function("unchecked-dec-int", &native_dec_int, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-remainder", new_node_native_function("unchecked-remainder", &native_remainder, false, NODE_FLAG_PRERESOLVE));
	env->set("unchecked-remainder-int", new_node_native_function("unchecked-remainder-int", &native_remainder_int, false, NODE_FLAG_PRERESOLVE));
}
