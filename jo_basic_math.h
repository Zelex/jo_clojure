#pragma once

#include <float.h>

// TODO:
// o dot product
// o cross product
// o matrix type
// o tensor type
// o check for divide by 0!

#if defined __has_builtin
#define JO_HAS_BUILTIN(n) __has_builtin(n)
#else
#define JO_HAS_BUILTIN(n) 0
#endif

#if JO_HAS_BUILTIN(__builtin_sqrt) && JO_HAS_BUILTIN(__builtin_log)
#define jo_math_sqrt(v) (__builtin_sqrt(v))
#define jo_math_log(v) (__builtin_log(v))
#else
#include <math.h>
#define jo_math_sqrt(v) (sqrt(v))
#define jo_math_log(v) (log(v))
#endif

template<typename T> constexpr T jo_math_max(T a, T b) { return a > b ? a : b; }
template <typename T> constexpr T jo_math_abs(T a) { return a > 0 ? a : -a; }
template <typename T> constexpr T jo_math_sqr(T a) { return a*a; }
template<typename T> constexpr T jo_math_sign(T a, T b) { return b >= 0.0 ? jo_math_abs(a) : -jo_math_abs(a); }

template <typename T> constexpr T jo_math_pythag(T a, T b) {
    T absa = jo_math_abs(a), absb = jo_math_abs(b);
    T tmp1 = absa / absb;
    T tmp2 = absb / absa;
    return (absa > absb ? absa * jo_math_sqrt(1.0 + tmp2 * tmp2) : (absb == 0.0 ? 0.0 : absb * jo_math_sqrt(1.0 + tmp1 * tmp1)));
}

// Singular Value Decomposition (economy)
// Computes a = USV where a is a m x n matrix.
// A is overwritten with U
// U is a m x n matrix
// S is a n x n diagonal matrix (matrix_t is always sparse)
// V is a n x n matrix
// returns false on error (no convergence)
inline bool jo_math_svd(matrix_ptr_t A, matrix_ptr_t S, matrix_ptr_t V) {
	int m = A->height;
	int n = A->width;
    int l = 0, nm = 0;
    double anorm = 0, scale = 0, s = 0, g = 0, tmp = 0;
    double *rv1 = (double *)malloc(n * sizeof(*rv1));
    for (int i = 0; i < n; ++i) {
        l = i + 2;
        rv1[i] = scale * g;
        scale = g = s = 0;
        if (i < m) {
            for (int k = i; k < m; ++k) {
				scale += jo_math_abs(get_node_float(A->get(i, k)));
            }
            if (scale != 0) {
                for (int k = i; k < m; ++k) {
					tmp = get_node_float(A->get(i, k)) / scale;
					A->set(i, k, new_node_float(tmp));
                    s += tmp * tmp;
                }
				double f = get_node_float(A->get(i, i));
                g = f >= 0 ? -jo_math_sqrt(s) : jo_math_sqrt(s);
                double h = f * g - s;
				A->set(i, i, new_node_float(f - g));
                for (int j = l - 1; j < n; ++j) {
                    s = 0;
                    for (int k = i; k < m; ++k) {
						s += get_node_float(A->get(i, k)) * get_node_float(A->get(j, k));
                    }
                    f = s / h;
                    for (int k = i; k < m; ++k) {
						A->set(j, k, new_node_float(get_node_float(A->get(j, k)) + f * get_node_float(A->get(i, k))));
                    }
                }
                for (int k = i; k < m; ++k) {
					A->set(i, k, new_node_float(get_node_float(A->get(i, k)) * scale));
                }
            }
        }
		S->set(i, i, new_node_float(scale * g));
        s = 0;
        g = 0;
        scale = 0;
        if (i + 1 <= m && i + 1 != n) {
            for (int k = l - 1; k < n; ++k) {
				scale += jo_math_abs(get_node_float(A->get(k, i)));
            }
            if (scale != 0) {
                for (int k = l - 1; k < n; ++k) {
					tmp = get_node_float(A->get(k, i)) / scale;
					A->set(k, i, new_node_float(tmp));
					s += tmp * tmp;
                }
				double f = get_node_float(A->get(l - 1, i));
                g = f >= 0 ? -jo_math_sqrt(s) : jo_math_sqrt(s);
                double h = f * g - s;
				A->set(l - 1, i, new_node_float(f - g));
                for (int k = l - 1; k < n; ++k) {
					rv1[k] = get_node_float(A->get(k, i)) / h;
                }
                for (int j = l - 1; j < m; ++j) {
                    s = 0;
                    for (int k = l - 1; k < n; ++k) {
						s += get_node_float(A->get(k, j)) * get_node_float(A->get(k, i));
                    }
                    for (int k = l - 1; k < n; ++k) {
						A->set(k, j, new_node_float(get_node_float(A->get(k, j)) + s * rv1[k]));
                    }
                }
                for (int k = l - 1; k < n; ++k) {
					A->set(k, i, new_node_float(get_node_float(A->get(k, i)) * scale));
                }
            }
        }
		tmp = jo_math_abs(get_node_float(S->get(i, i))) + jo_math_abs(rv1[i]);
        anorm = anorm > tmp ? anorm : tmp;
    }
    for (int i = n - 1; i >= 0; i--) {
        if (i < n - 1) {
            if (g != 0) {
                for (int j = l; j < n; ++j) {
					V->set(i, j, new_node_float(get_node_float(A->get(j, i)) / get_node_float(A->get(l, i)) / g));
                }
                for (int j = l; j < n; ++j) {
                    double s = 0;
                    for (int k = l; k < n; ++k) {
						s += get_node_float(A->get(k, i)) * get_node_float(V->get(j, k));
                    }
                    for (int k = l; k < n; ++k) {
						V->set(j, k, new_node_float(get_node_float(V->get(j, k)) + s * get_node_float(V->get(i, k))));
                    }
                }
            }
            for (int j = l; j < n; ++j) {
				V->set(i, j, ZERO_NODE);
				V->set(j, i, ZERO_NODE);
            }
        }
		V->set(i, i, ONE_NODE);
        g = rv1[i];
        l = i;
    }
    for (int i = (m < n ? m : n) - 1; i >= 0; --i) {
        l = i + 1;
		g = get_node_float(S->get(i, i));
        for (int j = l; j < n; ++j) {
			A->set(j, i, ZERO_NODE);
        }
        if (g != 0) {
            g = 1 / g;
            for (int j = l; j < n; ++j) {
                double s = 0;
                for (int k = l; k < m; ++k) {
					s += get_node_float(A->get(i, k)) * get_node_float(A->get(j, k));
                }
				double f = s / get_node_float(A->get(i, i)) * g;
                for (int k = i; k < m; ++k) {
					A->set(j, k, new_node_float(get_node_float(A->get(j, k)) + f * get_node_float(A->get(i, k))));
                }
            }
            for (int j = i; j < m; ++j) {
				A->set(i, j, new_node_float(get_node_float(A->get(i, j)) * g));
            }
        } else {
            for (int j = i; j < m; ++j) {
				A->set(i, j, ZERO_NODE);
            }
        }
		A->set(i, i, new_node_float(get_node_float(A->get(i, i)) + 1));
    }
    for (int k = n - 1; k >= 0; --k) {
        int iter = 0;
        for (; iter < 120; ++iter) {
            bool flag = true;
            for (l = k; l >= 0; --l) {
                nm = l - 1;
                if (l == 0 || jo_math_abs(rv1[l]) <= DBL_EPSILON * anorm) {
                    flag = false;
                    break;
                }
                if (jo_math_abs(get_node_float(S->get(nm,nm))) <= DBL_EPSILON * anorm) {
                    break;
                }
            }
            if (flag) {
                double c = 0, s = 1;
                for (int i = l; i < k + 1; ++i) {
                    double f = s * rv1[i];
                    rv1[i] = c * rv1[i];
                    if (jo_math_abs(f) <= DBL_EPSILON * anorm) {
                        break;
                    }
                    g = get_node_float(S->get(i,i));
                    double h = jo_math_pythag(f, g);
                    S->set(i,i, new_node_float(h));
                    h = 1 / h;
                    c = g * h;
                    s = -f * h;
                    for (int j = 0; j < m; ++j) {
						double y = get_node_float(A->get(nm, j));
						double z = get_node_float(A->get(i, j));
						A->set(nm, j, new_node_float(y * c + z * s));
						A->set(i, j, new_node_float(z * c - y * s));
                    }
                }
            }
            double z = get_node_float(S->get(k,k));
            if (l == k) {
                if (z < 0) {
					S->set(k, k, new_node_float(-z));
                    for (int j = 0; j < n; ++j) {
						V->set(k, j, new_node_float(-get_node_float(V->get(k, j))));
                    }
                }
                break;
            }
			double x = get_node_float(S->get(l, l));
			double y = get_node_float(S->get(k - 1, k - 1));
            g = rv1[k - 1];
            double h = rv1[k];
            double f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2 * h * y);
            g = jo_math_pythag<double>(f, 1);
            f += f >= 0 ? jo_math_abs(g) : -jo_math_abs(g);
            f = ((x - z) * (x + z) + h * (y / f - h)) / x;
            double c = 1;
            double s = 1;
            for (int j = l; j <= k - 1; ++j) {
                int i = j + 1;
                g = rv1[i];
                y = get_node_float(S->get(i, i));
                h = s * g;
                g = c * g;
                rv1[j] = z = jo_math_pythag(f, h);
                c = f / z;
                s = h / z;
                f = x * c + g * s;
                g = g * c - x * s;
                h = y * s;
                y *= c;
                for (int jj = 0; jj < n; ++jj) {
					x = get_node_float(V->get(j, jj));
					z = get_node_float(V->get(i, jj));
					V->set(j, jj, new_node_float(x * c + z * s));
					V->set(i, jj, new_node_float(z * c - x * s));
                }
                z = jo_math_pythag(f, h);
				S->set(j, j, new_node_float(z));
                if (z) {
                    z = 1 / z;
                    c = f * z;
                    s = h * z;
                }
                f = c * g + s * y;
                x = c * y - s * g;
                for (int jj = 0; jj < m; ++jj) {
					y = get_node_float(A->get(j, jj));
					z = get_node_float(A->get(i, jj));
					A->set(j, jj, new_node_float(y * c + z * s));
					A->set(i, jj, new_node_float(z * c - y * s));
                }
            }
            rv1[l] = 0;
            rv1[k] = f;
			S->set(k, k, new_node_float(x));
		}
        if (iter == 120) {
            free(rv1);
            return false;
        }
    }
    free(rv1);
    return true;
}

// fma with destructuring
static node_idx_t node_fma(node_idx_t n1i, node_idx_t n2i, node_idx_t n3i) {
    if (n1i == INV_NODE || n2i == INV_NODE || n3i == INV_NODE) {
        return ZERO_NODE;
    }

    node_t *n1 = get_node(n1i), *n2 = get_node(n2i), *n3 = get_node(n3i);
    if (n1->type == NODE_FUTURE && (n1->flags & NODE_FLAG_AUTO_DEREF)) {
        n1i = n1->deref();
        n1 = get_node(n1i);
    }
    if (n2->type == NODE_FUTURE && (n2->flags & NODE_FLAG_AUTO_DEREF)) {
        n2i = n2->deref();
        n2 = get_node(n2i);
    }
    if (n3->type == NODE_FUTURE && (n3->flags & NODE_FLAG_AUTO_DEREF)) {
        n3i = n3->deref();
        n3 = get_node(n3i);
    }

    if (n1->is_list() && n2->is_list() && n3->is_list()) {
        list_ptr_t r = new_list();
        list_t::iterator it1(n1->t_list), it2(n2->t_list), it3(n3->t_list);
        for (; it1 && it2 && it3; it1++, it2++, it3++) {
            r->push_back_inplace(node_fma(n1->as_vector()->nth(*it1), n2->as_vector()->nth(*it2), n3->as_vector()->nth(*it3)));
        }
        for (; it1; it1++) r->push_back_inplace(n1->as_vector()->nth(*it1));
        for (; it2; it2++) r->push_back_inplace(n2->as_vector()->nth(*it2));
        for (; it3; it3++) r->push_back_inplace(n3->as_vector()->nth(*it3));
        return new_node_list(r);
    }
    if (n1->is_vector() && n2->is_vector() && n3->is_vector()) {
        vector_ptr_t r = new_vector();
        vector_ptr_t v1 = n1->as_vector(), v2 = n2->as_vector(), v3 = n3->as_vector();
        size_t s1 = v1->size(), s2 = v2->size(), s3 = v3->size();
        size_t min_s = jo_min(s1, jo_min(s2, s3));
        size_t i = 0;
        for (; i < min_s; i++) {
            r->push_back_inplace(node_fma(v1->nth(i), v2->nth(i), v3->nth(i)));
        }
        for (; i < s1; i++) r->push_back_inplace(v1->nth(i));
        for (; i < s2; i++) r->push_back_inplace(v2->nth(i));
        for (; i < s3; i++) r->push_back_inplace(v3->nth(i));
        return new_node_vector(r);
    }
    if (n1->is_hash_map() && n2->is_hash_map() && n3->is_hash_map()) {
        hash_map_ptr_t r = new_hash_map();
        hash_map_ptr_t m1 = n1->as_hash_map(), m2 = n2->as_hash_map(), m3 = n3->as_hash_map();
        for (hash_map_t::iterator it = m1->begin(); it; it++) {
            if (!m2->contains(it->first, node_eq)) {
                r->assoc_inplace(it->first, it->second, node_eq);
                continue;
            }
            if (!m3->contains(it->first, node_eq)) {
                r->assoc_inplace(it->first, it->second, node_eq);
                continue;
            }
            r->assoc_inplace(it->first, node_fma(it->second, m2->get(it->first, node_eq), m3->get(it->first, node_eq)), node_eq);
        }
        for (hash_map_t::iterator it = m2->begin(); it; it++) {
            if (r->contains(it->first, node_eq)) continue;
            r->assoc_inplace(it->first, it->second, node_eq);
        }
        for (hash_map_t::iterator it = m3->begin(); it; it++) {
            if (r->contains(it->first, node_eq)) continue;
            r->assoc_inplace(it->first, it->second, node_eq);
        }
        return new_node_hash_map(r);
    }
    return new_node_float(fmaf(n1->as_float(), n2->as_float(), n3->as_float()));
}

// native function to add any number of arguments
static node_idx_t native_add(env_ptr_t env, list_ptr_t args) {
    long long i = 0;
    double d = 0.0;
    for (list_t::iterator it(args); it; it++) {
        node_t *n = get_node(*it);
        if (n->type == NODE_INT) {
            i += n->t_int;
        } else if (n->type == NODE_FLOAT) {
            d += n->t_float;
        } else {
            d += n->as_float();
        }
    }
    return d == 0.0 ? new_node_int(i) : new_node_float(d + i);
}

static node_idx_t native_add_int(env_ptr_t env, list_ptr_t args) {
    long long i = 0;
    for (list_t::iterator it(args); it; it++) {
        node_t *n = get_node(*it);
        if (n->type == NODE_INT) {
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
    if (size == 0) return ZERO_NODE;

    // Special case. 1 argument return the negative of that argument
    if (size == 1) {
        node_t *n = get_node(args->first_value());
        if (n->type == NODE_INT) return new_node_int(-n->t_int);
        return new_node_float(-n->as_float());
    }

    list_t::iterator i(args);
    node_t *n = get_node(*i++);
    if (n->type == NODE_INT) {
        i_sum = n->t_int;
    } else {
        d_sum = n->as_float();
    }

    for (; i; i++) {
        n = get_node(*i);
        if (n->type == NODE_INT) {
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
    if (size == 0) return ZERO_NODE;

    // Special case. 1 argument return the negative of that argument
    if (size == 1) {
        node_t *n = get_node(args->first_value());
        if (n->type == NODE_INT) {
            return new_node_int(-n->t_int);
        }
        return new_node_int(-n->as_int());
    }

    list_t::iterator i(args);
    node_t *n = get_node(*i++);
    if (n->type == NODE_INT) {
        i_sum = n->t_int;
    } else {
        i_sum = n->as_int();
    }

    for (; i; i++) {
        n = get_node(*i);
        if (n->type == NODE_INT) {
            i_sum -= n->t_int;
        } else {
            i_sum -= n->as_int();
        }
    }
    return new_node_int(i_sum);
}

static node_idx_t native_mul(env_ptr_t env, list_ptr_t args) {
    if (args->size() == 0) return ONE_NODE;

    long long i = 1;
    double d = 1.0;
    for (list_t::iterator it(args); it; it++) {
        node_t *n = get_node(*it);
        int type = n->type;
        if (type == NODE_INT)
            i *= n->t_int;
        else if (type == NODE_FLOAT)
            d *= n->t_float;
        else
            d *= n->as_float();
    }
    return d == 1.0 ? new_node_int(i) : new_node_float(d * i);
}

static node_idx_t native_fma(env_ptr_t env, list_ptr_t args) {
    if (args->size() < 3) return ZERO_NODE;
    list_t::iterator i(args);
    node_idx_t a = *i++, b = *i++, c = *i++;
    return node_fma(a, b, c);
}

static node_idx_t native_mul_int(env_ptr_t env, list_ptr_t args) {
    if (args->size() == 0) return ONE_NODE;

    long long i = 1;
    for (list_t::iterator it(args); it; it++) {
        node_t *n = get_node(*it);
        int type = n->type;
        if (type == NODE_INT)
            i *= n->t_int;
        else
            i *= n->as_int();
    }
    return new_node_int(i);
}

// divide any number of arguments from the first argument
static node_idx_t native_div(env_ptr_t env, list_ptr_t args) {
    long long i_sum = 1;
    double d_sum = 1.0;
    bool is_int = true;

    size_t size = args->size();
    if (size == 0) return ONE_NODE;

    // special case of 1 argument, compute 1.0 / value
    if (size == 1) {
        node_t *n = get_node(args->first_value());
        return new_node_float(1.0 / n->as_float());
    }

    list_t::iterator i(args);
    node_t *n = get_node(*i++);
    if (n->type == NODE_INT) {
        i_sum = n->t_int;
        d_sum = i_sum;
    } else {
        d_sum = n->as_float();
        is_int = false;
    }

    for (; i; i++) {
        n = get_node(*i);
        if (is_int && n->type == NODE_INT) {
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
    if (size == 0) return ONE_NODE;

    // special case of 1 argument, compute 1.0 / value
    if (size == 1) {
        node_t *n = get_node(args->first_value());
        i_sum = n->as_int();
        return i_sum == 1 ? ONE_NODE : i_sum == -1 ? new_node_int(-1) : ZERO_NODE;
    }

    list_t::iterator i(args);
    node_t *n = get_node(*i++);
    if (n->type == NODE_INT) {
        i_sum = n->t_int;
    } else {
        i_sum = n->as_int();
    }

    for (; i; i++) {
        n = get_node(*i);
        if (n->type == NODE_INT) {
            i_sum /= n->t_int;
        } else {
            i_sum /= n->as_int();
        }
    }

    return new_node_int(i_sum);
}

static node_idx_t native_mod(env_ptr_t env, list_ptr_t args) {
    if (args->size() == 0) return ZERO_NODE;
    node_t *n = get_node(args->first_value());
    if (n->type == NODE_INT) return new_node_int(n->t_int % get_node(args->second_value())->as_int());
    return new_node_float(fmod(n->as_float(), get_node(args->second_value())->as_float()));
}

static node_idx_t native_remainder(env_ptr_t env, list_ptr_t args) {
    if (args->size() == 0) return ZERO_NODE;
    node_t *n = get_node(args->first_value());
    if (n->type == NODE_INT) return new_node_int(n->t_int % get_node(args->second_value())->as_int());
    return new_node_float(remainder(n->as_float(), get_node(args->second_value())->as_float()));
}

static node_idx_t native_remainder_int(env_ptr_t env, list_ptr_t args) {
    if (args->size() == 0) return ZERO_NODE;
    node_t *n = get_node(args->first_value());
    if (n->type == NODE_INT) return new_node_int(n->t_int % get_node(args->second_value())->as_int());
    return new_node_int(n->as_int() % get_node(args->second_value())->as_int());
}

static node_idx_t native_math_abs(env_ptr_t env, list_ptr_t args) {
    node_t *n1 = get_node(args->first_value());
    if (n1->type == NODE_INT) return new_node_int(abs(n1->t_int));
    return new_node_float(fabs(n1->as_float()));
}

static node_idx_t native_inc(env_ptr_t env, list_ptr_t args) {
    node_t *n1 = get_node(args->first_value());
    if (n1->type == NODE_INT) return new_node_int(n1->t_int + 1);
    return new_node_float(n1->as_float() + 1.0f);
}

static node_idx_t native_inc_int(env_ptr_t env, list_ptr_t args) {
    node_t *n1 = get_node(args->first_value());
    if (n1->type == NODE_INT) return new_node_int(n1->t_int + 1);
    return new_node_int(n1->as_int() + 1);
}

static node_idx_t native_dec(env_ptr_t env, list_ptr_t args) {
    node_t *n1 = get_node(args->first_value());
    if (n1->type == NODE_INT) return new_node_int(n1->t_int - 1);
    return new_node_float(n1->as_float() - 1.0f);
}

static node_idx_t native_dec_int(env_ptr_t env, list_ptr_t args) {
    node_t *n1 = get_node(args->first_value());
    if (n1->type == NODE_INT) return new_node_int(n1->t_int - 1);
    return new_node_int(n1->as_int() - 1);
}

static node_idx_t native_rand_int(env_ptr_t env, list_ptr_t args) {
    return new_node_int(jo_pcg32(&jo_rnd_state) % get_node_int(args->first_value()));
}
static node_idx_t native_rand(env_ptr_t env, list_ptr_t args) {
    double bias = 0.0;
    double scale = 1.0 / (double)UINT32_MAX;
    if (args->size() == 1) {
        scale = get_node_float(args->first_value()) / (double)UINT32_MAX;
    } else if (args->size() == 2) {
        // compute scale/bias for random value between first and second number
        double A = get_node_float(args->first_value());
        double B = get_node_float(args->second_value());
        bias = A;
        scale = (B - A) / (double)UINT32_MAX;
    }
    return new_node_float(bias + jo_pcg32(&jo_rnd_state) * scale);
}
static node_idx_t native_math_sqrt(env_ptr_t env, list_ptr_t args) {
    return new_node_float(sqrt(get_node_float(args->first_value())));
}
static node_idx_t native_math_cbrt(env_ptr_t env, list_ptr_t args) {
    return new_node_float(cbrt(get_node_float(args->first_value())));
}
static node_idx_t native_math_ceil(env_ptr_t env, list_ptr_t args) {
    return new_node_int(ceil(get_node_float(args->first_value())));
}
static node_idx_t native_math_floor(env_ptr_t env, list_ptr_t args) {
    return new_node_int(floor(get_node_float(args->first_value())));
}
static node_idx_t native_math_exp(env_ptr_t env, list_ptr_t args) {
    return new_node_float(exp(get_node_float(args->first_value())));
}
static node_idx_t native_math_exp2(env_ptr_t env, list_ptr_t args) {
    return new_node_float(exp2(get_node_float(args->first_value())));
}
static node_idx_t native_math_hypot(env_ptr_t env, list_ptr_t args) {
    return new_node_float(hypot(get_node_float(args->first_value()), get_node_float(args->second_value())));
}
static node_idx_t native_math_log10(env_ptr_t env, list_ptr_t args) {
    return new_node_float(log10(get_node_float(args->first_value())));
}
static node_idx_t native_math_log(env_ptr_t env, list_ptr_t args) {
    return new_node_float(log(get_node_float(args->first_value())));
}
static node_idx_t native_math_log2(env_ptr_t env, list_ptr_t args) {
    return new_node_float(log2(get_node_float(args->first_value())));
}
static node_idx_t native_math_log1p(env_ptr_t env, list_ptr_t args) {
    return new_node_float(log1p(get_node_float(args->first_value())));
}
static node_idx_t native_math_sin(env_ptr_t env, list_ptr_t args) {
    return new_node_float(sin(get_node_float(args->first_value())));
}
static node_idx_t native_math_cos(env_ptr_t env, list_ptr_t args) {
    return new_node_float(cos(get_node_float(args->first_value())));
}
static node_idx_t native_math_tan(env_ptr_t env, list_ptr_t args) {
    return new_node_float(tan(get_node_float(args->first_value())));
}
static node_idx_t native_math_pow(env_ptr_t env, list_ptr_t args) {
    return new_node_float(pow(get_node_float(args->first_value()), get_node_float(args->second_value())));
}
static node_idx_t native_math_sinh(env_ptr_t env, list_ptr_t args) {
    return new_node_float(sinh(get_node_float(args->first_value())));
}
static node_idx_t native_math_cosh(env_ptr_t env, list_ptr_t args) {
    return new_node_float(cosh(get_node_float(args->first_value())));
}
static node_idx_t native_math_tanh(env_ptr_t env, list_ptr_t args) {
    return new_node_float(tanh(get_node_float(args->first_value())));
}
static node_idx_t native_math_asin(env_ptr_t env, list_ptr_t args) {
    return new_node_float(asin(get_node_float(args->first_value())));
}
static node_idx_t native_math_acos(env_ptr_t env, list_ptr_t args) {
    return new_node_float(acos(get_node_float(args->first_value())));
}
static node_idx_t native_math_atan(env_ptr_t env, list_ptr_t args) {
    return new_node_float(atan(get_node_float(args->first_value())));
}
static node_idx_t native_math_atan2(env_ptr_t env, list_ptr_t args) {
    return new_node_float(atan2(get_node_float(args->first_value()), get_node_float(args->second_value())));
}
static node_idx_t native_math_asinh(env_ptr_t env, list_ptr_t args) {
    return new_node_float(asinh(get_node_float(args->first_value())));
}
static node_idx_t native_math_acosh(env_ptr_t env, list_ptr_t args) {
    return new_node_float(acosh(get_node_float(args->first_value())));
}
static node_idx_t native_math_atanh(env_ptr_t env, list_ptr_t args) {
    return new_node_float(atanh(get_node_float(args->first_value())));
}
static node_idx_t native_math_erf(env_ptr_t env, list_ptr_t args) {
    return new_node_float(erf(get_node_float(args->first_value())));
}
static node_idx_t native_math_erfc(env_ptr_t env, list_ptr_t args) {
    return new_node_float(erfc(get_node_float(args->first_value())));
}
static node_idx_t native_math_tgamma(env_ptr_t env, list_ptr_t args) {
    return new_node_float(tgamma(get_node_float(args->first_value())));
}
static node_idx_t native_math_lgamma(env_ptr_t env, list_ptr_t args) {
    return new_node_float(lgamma(get_node_float(args->first_value())));
}
static node_idx_t native_math_round(env_ptr_t env, list_ptr_t args) {
    return new_node_int(round(get_node_float(args->first_value())));
}
static node_idx_t native_math_trunc(env_ptr_t env, list_ptr_t args) {
    return new_node_int(trunc(get_node_float(args->first_value())));
}
static node_idx_t native_math_logb(env_ptr_t env, list_ptr_t args) {
    return new_node_float(logb(get_node_float(args->first_value())));
}
static node_idx_t native_math_ilogb(env_ptr_t env, list_ptr_t args) {
    return new_node_int(ilogb(get_node_float(args->first_value())));
}
static node_idx_t native_math_expm1(env_ptr_t env, list_ptr_t args) {
    return new_node_float(expm1(get_node_float(args->first_value())));
}
static node_idx_t native_is_even(env_ptr_t env, list_ptr_t args) {
    return new_node_bool((get_node_int(args->first_value()) & 1) == 0);
}
static node_idx_t native_is_odd(env_ptr_t env, list_ptr_t args) {
    return new_node_bool((get_node_int(args->first_value()) & 1) == 1);
}
static node_idx_t native_is_pos(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(get_node_float(args->first_value()) > 0);
}
static node_idx_t native_is_neg(env_ptr_t env, list_ptr_t args) {
    return new_node_bool(get_node_float(args->first_value()) < 0);
}
static node_idx_t native_is_pos_int(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    return new_node_bool(n->is_int() && n->as_int() > 0);
}
static node_idx_t native_is_neg_int(env_ptr_t env, list_ptr_t args) {
    node_t *n = get_node(args->first_value());
    return new_node_bool(n->is_int() && n->as_int() < 0);
}
static node_idx_t native_bit_not(env_ptr_t env, list_ptr_t args) {
    return new_node_int(~get_node_int(args->first_value()));
}
static node_idx_t native_bit_shift_left(env_ptr_t env, list_ptr_t args) {
    return new_node_int(get_node_int(args->first_value()) << get_node_int(args->second_value()));
}
static node_idx_t native_bit_shift_right(env_ptr_t env, list_ptr_t args) {
    return new_node_int(get_node_int(args->first_value()) >> get_node_int(args->second_value()));
}
static node_idx_t native_unsigned_bit_shift_right(env_ptr_t env, list_ptr_t args) {
    return new_node_int((unsigned)get_node_int(args->first_value()) >> get_node_int(args->second_value()));
}
static node_idx_t native_bit_extract(env_ptr_t env, list_ptr_t args) {
    return new_node_int((get_node_int(args->first_value()) >> get_node_int(args->second_value())) & ((1 << get_node_int(args->third_value())) - 1));
}
static node_idx_t native_bit_clear(env_ptr_t env, list_ptr_t args) {
    return new_node_int(get_node_int(args->first_value()) & ~(1 << get_node_int(args->second_value())));
}
static node_idx_t native_bit_flip(env_ptr_t env, list_ptr_t args) {
    return new_node_int(get_node_int(args->first_value()) ^ (1 << get_node_int(args->second_value())));
}
static node_idx_t native_bit_set(env_ptr_t env, list_ptr_t args) {
    return new_node_int(get_node_int(args->first_value()) | (1 << get_node_int(args->second_value())));
}
static node_idx_t native_bit_test(env_ptr_t env, list_ptr_t args) {
    return new_node_bool((get_node_int(args->first_value()) & (1 << get_node_int(args->second_value()))) != 0);
}
static node_idx_t native_math_to_degrees(env_ptr_t env, list_ptr_t args) {
    return new_node_float(get_node_float(args->first_value()) * 180.0f / JO_M_PI);
}
static node_idx_t native_math_to_radians(env_ptr_t env, list_ptr_t args) {
    return new_node_float(get_node_float(args->first_value()) * JO_M_PI / 180.0f);
}

// Computes the minimum value of any number of arguments
static node_idx_t native_math_min(env_ptr_t env, list_ptr_t args) {
    if (args->size() == 0) return ZERO_NODE;

    list_t::iterator it(args);
    node_idx_t min_node = *it++;
    if (args->size() == 1) return min_node;

    node_t *n = get_node(min_node);
    if (n->type == NODE_INT) {
        bool is_int = true;
        long long min_int = n->t_int;
        float min_float = min_int;
        node_idx_t next = *it++;
        while (true) {
            n = get_node(next);
            if (is_int && n->type == NODE_INT) {
                if (n->t_int < min_int) {
                    min_int = n->t_int;
                    min_float = min_int;
                    min_node = next;
                }
            } else {
                is_int = false;
                if (n->as_float() < min_float) {
                    min_float = n->as_float();
                    min_node = next;
                }
            }
            if (!it) {
                break;
            }
            next = *it++;
        }
        return min_node;
    }

    float min_float = n->as_float();
    for (node_idx_t next = *it++; it; next = *it++) {
        n = get_node(next);
        if (n->as_float() < min_float) {
            min_float = n->as_float();
            min_node = next;
        }
    }
    return min_node;
}

static node_idx_t native_math_max(env_ptr_t env, list_ptr_t args) {
    if (args->size() == 0) return ZERO_NODE;

    list_t::iterator it(args);

    // Get the first argument
    node_idx_t max_node = *it++;
    if (args->size() == 1) return max_node;

    node_t *n = get_node(max_node);
    if (n->type == NODE_INT) {
        bool is_int = true;
        long long max_int = n->t_int;
        float max_float = max_int;
        node_idx_t next = *it++;
        while (true) {
            n = get_node(next);
            if (is_int && n->type == NODE_INT) {
                if (n->t_int > max_int) {
                    max_int = n->t_int;
                    max_float = max_int;
                    max_node = next;
                }
            } else {
                is_int = false;
                if (n->as_float() > max_float) {
                    max_float = n->as_float();
                    max_node = next;
                }
            }
            if (!it) {
                break;
            }
            next = *it++;
        }
        return max_node;
    }

    float max_float = n->as_float();
    for (node_idx_t next = *it++; it; next = *it++) {
        n = get_node(next);
        if (n->as_float() > max_float) {
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
    if (n1->type == NODE_INT && n2->type == NODE_INT && n3->type == NODE_INT) {
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
    for (; it; it++) {
        n &= get_node_int(*it);
    }
    return new_node_int(n);
}

static node_idx_t native_bit_and_not(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    long long n = get_node_int(*it++);
    for (; it; it++) {
        n &= ~get_node_int(*it);
    }
    return new_node_int(n);
}

static node_idx_t native_bit_or(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    long long n = get_node_int(*it++);
    for (; it; it++) {
        n |= get_node_int(*it);
    }
    return new_node_int(n);
}

static node_idx_t native_bit_xor(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    long long n = get_node_int(*it++);
    for (; it; it++) {
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

static node_idx_t native_float(env_ptr_t env, list_ptr_t args) {
    return new_node_float(get_node_float(args->first_value()));
}
static node_idx_t native_double(env_ptr_t env, list_ptr_t args) {
    return new_node_float(get_node_float(args->first_value()));
}
static node_idx_t native_int(env_ptr_t env, list_ptr_t args) {
    return new_node_int(get_node(args->first_value())->as_int());
}

static node_idx_t native_is_float(env_ptr_t env, list_ptr_t args) {
    return get_node_type(args->first_value()) == NODE_FLOAT ? TRUE_NODE : FALSE_NODE;
}
static node_idx_t native_is_double(env_ptr_t env, list_ptr_t args) {
    return get_node_type(args->first_value()) == NODE_FLOAT ? TRUE_NODE : FALSE_NODE;
}
static node_idx_t native_integer_q(env_ptr_t env, list_ptr_t args) {
    return get_node_type(args->first_value()) == NODE_INT ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_math_quantize(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    double x = get_node_float(*it++);
    double step = get_node_float(*it++);
    return new_node_float(roundf(x / step) * step);
}

// Calculates the average of all the values in the list.
static node_idx_t native_math_average(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    double sum = 0;
    for (; it; it++) {
        sum += get_node_float(*it);
    }
    return new_node_float(sum / args->size());
}

static node_idx_t native_math_srand(env_ptr_t env, list_ptr_t args) {
    jo_rnd_state = jo_pcg32_init(get_node_int(args->first_value()));
    return NIL_NODE;
}

static node_idx_t native_math_reflect(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t I_idx = *it++; // incident vector
    node_idx_t N_idx = *it++; // normal vector
    node_t *I = get_node(I_idx);
    node_t *N = get_node(N_idx);
    if (!I->is_vector() || !N->is_vector()) {
        return NIL_NODE;
    }
    vector_ptr_t I_vec = I->as_vector();
    vector_ptr_t N_vec = N->as_vector();
    size_t min_dim = I_vec->size() < N_vec->size() ? I_vec->size() : N_vec->size();
    double dot = 0;
    for (size_t i = 0; i < min_dim; i++) {
        dot += get_node_float(I_vec->nth(i)) * get_node_float(N_vec->nth(i));
    }
    dot *= 2;
    vector_ptr_t res = new_vector();
    for (size_t i = 0; i < min_dim; i++) {
        res->push_back_inplace(new_node_float(get_node_float(I_vec->nth(i)) - dot * get_node_float(N_vec->nth(i))));
    }
    return new_node_vector(res);
}

static node_idx_t native_math_normalize(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t v_idx = *it++;
    node_t *v = get_node(v_idx);
    if (!v->is_vector()) {
        return NIL_NODE;
    }
    vector_ptr_t vec = v->as_vector();
    double len = 0;
    for (size_t i = 0; i < vec->size(); i++) {
        len += get_node_float(vec->nth(i)) * get_node_float(vec->nth(i));
    }
    len = len ? jo_math_sqrt(len) : 1;
    vector_ptr_t res = new_vector();
    for (size_t i = 0; i < vec->size(); i++) {
        res->push_back_inplace(new_node_float(get_node_float(vec->nth(i)) / len));
    }
    return new_node_vector(res);
}

static node_idx_t native_math_refract(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t I_idx = *it++;   // incident vector
    node_idx_t N_idx = *it++;   // normal vector
    node_idx_t eta_idx = *it++; // index of refraction
    node_t *I = get_node(I_idx);
    node_t *N = get_node(N_idx);
    node_t *eta = get_node(eta_idx);
    if (!I->is_vector() || !N->is_vector()) {
        return NIL_NODE;
    }
    vector_ptr_t I_vec = I->as_vector();
    vector_ptr_t N_vec = N->as_vector();
    size_t min_dim = I_vec->size() < N_vec->size() ? I_vec->size() : N_vec->size();
    double dot = 0;
    for (size_t i = 0; i < min_dim; i++) {
        dot += get_node_float(I_vec->nth(i)) * get_node_float(N_vec->nth(i));
    }
    double eta_val = eta->as_float();
    double k = 1 - eta_val * eta_val * (1 - dot * dot);
    if (k < 0) {
        return NIL_NODE;
    }
    vector_ptr_t res = new_vector();
    for (size_t i = 0; i < min_dim; i++) {
        res->push_back_inplace(new_node_float(eta_val * get_node_float(I_vec->nth(i)) - (eta_val * dot + jo_math_sqrt(k)) * get_node_float(N_vec->nth(i))));
    }
    return new_node_vector(res);
}

static node_idx_t native_math_dot(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t v1_idx = *it++;
    node_idx_t v2_idx = *it++;
    node_t *v1 = get_node(v1_idx);
    node_t *v2 = get_node(v2_idx);
    if (!v1->is_vector() || !v2->is_vector()) {
        return NIL_NODE;
    }
    vector_ptr_t v1_vec = v1->as_vector();
    vector_ptr_t v2_vec = v2->as_vector();
    size_t min_dim = jo_min(v1_vec->size(), v2_vec->size());
    double dot = 0;
    for (size_t i = 0; i < min_dim; i++) {
        dot += get_node_float(v1_vec->nth(i)) * get_node_float(v2_vec->nth(i));
    }
    return new_node_float(dot);
}

// create a vector of the diagonal elements of A
static node_idx_t native_math_diag(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A = get_node(A_idx);
    if (!A->is_matrix()) {
        warnf("diag: argument must be a matrix\n");
        return NIL_NODE;
    }
    matrix_ptr_t mat = A->as_matrix();
    size_t min_dim = mat->width < mat->height ? mat->width : mat->height;
    vector_ptr_t res = new_vector();
    for (size_t i = 0; i < min_dim; i++) {
        res->push_back_inplace(mat->get(i, i));
    }
    return new_node_vector(res);
}

// create a matrix with the vector v as the diagonal elements
static node_idx_t native_math_matrix_diag(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t v_idx = *it++;
    node_t *v = get_node(v_idx);
    if (!v->is_vector()) {
        warnf("%s: argument must be a vector\n", __func__);
        return NIL_NODE;
    }
    vector_ptr_t v_vec = v->as_vector();
    size_t min_dim = v_vec->size();
    matrix_ptr_t res = new_matrix(min_dim, min_dim);
    for (size_t i = 0; i < min_dim; i++) {
        res->set(i, i, v_vec->nth(i));
    }
    return new_node_matrix(res);
}

static node_idx_t native_math_matrix_sub(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_idx_t B_idx = *it++;
    node_t *A = get_node(A_idx);
    node_t *B = get_node(B_idx);
    if (!A->is_matrix() || !B->is_matrix()) {
        warnf("matrix_sub: arguments must be matrices\n");
        return NIL_NODE;
    }
    matrix_ptr_t A_mat = A->as_matrix();
    matrix_ptr_t B_mat = B->as_matrix();
    if (A_mat->width != B_mat->width || A_mat->height != B_mat->height) {
        warnf("matrix_sub: matrices have different dimensions\n");
        return NIL_NODE;
    }
    matrix_ptr_t res = new_matrix(A_mat->width, A_mat->height);
    for (size_t j = 0; j < A_mat->height; j++) {
        for (size_t i = 0; i < A_mat->width; i++) {
            res->set(i, j, new_node_float(get_node_float(A_mat->get(i, j)) - get_node_float(B_mat->get(i, j))));
        }
    }
    return new_node_matrix(res);
}

static node_idx_t native_math_matrix_mul(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_idx_t B_idx = *it++;
    node_t *A = get_node(A_idx);
    node_t *B = get_node(B_idx);
    if (!A->is_matrix() || !B->is_matrix()) {
        warnf("warning: matrix multiplication with non-matrix operands\n");
        return NIL_NODE;
    }
    matrix_ptr_t A_mat = A->as_matrix();
    matrix_ptr_t B_mat = B->as_matrix();
    if (A_mat->width != B_mat->height) {
        warnf("matrix_mul: incompatible matrix dimensions: %zu x %zu and %zu x %zu\n", A_mat->width, A_mat->height, B_mat->width, B_mat->height);
        return NIL_NODE;
    }
    matrix_ptr_t res = new_matrix(B_mat->width, A_mat->height);
    for (size_t j = 0; j < A_mat->height; j++) {
        for (size_t i = 0; i < B_mat->width; i++) {
            double dot = 0;
            for (size_t k = 0; k < A_mat->width; k++) {
                dot += get_node_float(A_mat->get(k, j)) * get_node_float(B_mat->get(i, k));
            }
            res->set(i, j, new_node_float(dot));
        }
    }
    return new_node_matrix(res);
}

static node_idx_t native_math_vector_sub(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_idx_t B_idx = *it++;
    node_t *A = get_node(A_idx);
    node_t *B = get_node(B_idx);
    if (!A->is_vector() || !B->is_vector()) {
        warnf("native_math_vector_sub: not a vector\n");
        return NIL_NODE;
    }
    vector_ptr_t A_vec = A->as_vector();
    vector_ptr_t B_vec = B->as_vector();
    if (A_vec->size() != B_vec->size()) {
        warnf("vector_sub: vector sizes do not match\n");
        return NIL_NODE;
    }
    vector_ptr_t res = new_vector();
    for (size_t i = 0; i < A_vec->size(); i++) {
        res->push_back_inplace(new_node_float(get_node_float(A_vec->nth(i)) - get_node_float(B_vec->nth(i))));
    }
    return new_node_vector(res);
}

static node_idx_t native_math_vector_div(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_idx_t B_idx = *it++;
    node_t *A = get_node(A_idx);
    node_t *B = get_node(B_idx);
    if (!A->is_vector() || !B->is_vector()) {
        warnf("native_math_vector_div: not a vector\n");
        return NIL_NODE;
    }
    vector_ptr_t A_vec = A->as_vector();
    vector_ptr_t B_vec = B->as_vector();
    if (A_vec->size() != B_vec->size()) {
        warnf("vector_div: vector sizes do not match\n");
        return NIL_NODE;
    }
    vector_ptr_t res = new_vector();
    for (size_t i = 0; i < A_vec->size(); i++) {
        res->push_back_inplace(new_node_float(get_node_float(A_vec->nth(i)) / get_node_float(B_vec->nth(i))));
    }
    return new_node_vector(res);
}

static node_idx_t native_math_matrix_div(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_idx_t B_idx = *it++;
    node_t *A = get_node(A_idx);
    node_t *B = get_node(B_idx);
    if (!A->is_matrix() || !B->is_matrix()) {
        warnf("native_math_matrix_div: not a matrix. arg types are %s and %s\n", A->type_name(), B->type_name());
        return NIL_NODE;
    }
    matrix_ptr_t A_mat = A->as_matrix();
    matrix_ptr_t B_mat = B->as_matrix();
    if (A_mat->width != B_mat->width || A_mat->height != B_mat->height) {
        warnf("native_math_matrix_div: matrix dimensions do not match\n");
        return NIL_NODE;
    }
    matrix_ptr_t res = new_matrix(A_mat->width, A_mat->height);
    for (size_t i = 0; i < A_mat->width; i++) {
        for (size_t j = 0; j < A_mat->height; j++) {
            res->set(i, j, new_node_float(get_node_float(A_mat->get(i, j)) / get_node_float(B_mat->get(i, j))));
        }
    }
    return new_node_matrix(res);
}

// pseudo-inverse via SVD (economy)
static node_idx_t native_math_matrix_svd(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A_node = get_node(A_idx);
    if (!A_node->is_matrix()) {
        warnf("native_math_matrix_svd: not a matrix. arg type is %s\n", A_node->type_name());
    }
    matrix_ptr_t A = A_node->as_matrix();
    int m = A->height;
    int n = A->width;

    matrix_ptr_t U = A->clone();
    matrix_ptr_t S = new_matrix(n, n);
    matrix_ptr_t V = new_matrix(n, n);

    // SVD
	jo_math_svd(U, S, V);

    return new_node_list(list_va(new_node_matrix(U), new_node_matrix(S), new_node_matrix(V)));
}

// pseudo-inverse via SVD (economy)
static node_idx_t native_math_matrix_pinv(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A_node = get_node(A_idx);
    if (!A_node->is_matrix()) {
        warnf("native_math_matrix_pinv: not a matrix. arg type is %s\n", A_node->type_name());
    }
    matrix_ptr_t A = A_node->as_matrix();
    int m = A->height;
    int n = A->width;

    matrix_ptr_t U = A->clone();
    matrix_ptr_t S = new_matrix(n, n);
    matrix_ptr_t V = new_matrix(n, n);

    // SVD
	jo_math_svd(U, S, V);

    matrix_ptr_t out = new_matrix(m, n);

	for (int k = 0; k < m; ++k) {
		for (int l = 0; l < n; ++l) {
			double tmp = 0;
			for (int j = 0; j < n; ++j) {
				double s = get_node_float(S->get(j,j)) < 0.0001 ? 0 : 1 / get_node_float(S->get(j,j));
				tmp += get_node_float(V->get(j,l)) * s * get_node_float(U->get(j,k));
			}
			out->set(k, l, new_node_float(tmp));
		}
	}

	return new_node_matrix(out);
}

// An m by n matrix of uniformly distributed random numbers
static node_idx_t native_math_matrix_rand(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t m_idx = *it++;
    node_idx_t n_idx = *it++;
	int m = get_node_int(m_idx);
	int n = get_node_int(n_idx);
	matrix_ptr_t out = new_matrix(m, n);
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < n; j++) {
			out->set(i, j, new_node_float(jo_pcg32(&jo_rnd_state) / (double)UINT32_MAX));
		}
	}
	return new_node_matrix(out);
}

static node_idx_t native_math_matrix_set_row(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A_node = get_node(A_idx);
    if (!A_node->is_matrix()) {
        warnf("native_math_matrix_set_row: not a matrix. arg type is %s\n", A_node->type_name());
        return A_idx;
    }
    matrix_ptr_t A = A_node->as_matrix();

    int row_num = get_node_int(*it++);

    node_idx_t row_idx = *it++;
    node_t *row_node = get_node(row_idx);
    
    matrix_ptr_t ret = A->clone();
    if (row_node->is_vector()) { // fast path
        vector_ptr_t row = row_node->as_vector();
        if(row->size() != ret->width) {
            warnf("native_math_matrix_set_row: vector size != matrix width\n");
            return A_idx;
        }
        for(int x = 0; x < ret->width; ++x) {
            node_idx_t val = row->nth(x);
            ret->set(x, row_num, val);
            assert(ret->get(x, row_num) == val);
        }
    } else if (row_node->is_list()) { // fast path
        list_ptr_t row = row_node->as_list();
        int x = 0;
        for(list_t::iterator it(row); it && x < ret->width; ++it, ++x) {
            ret->set(x, row_num, *it);
        }
    } else if(row_node->is_seq()) {
        list_ptr_t row = row_node->as_list();
        int x = 0;
        for(seq_iterator_t it(row_idx); it && x < ret->width; it.next(), ++x) {
            ret->set(x, row_num, it.val);
        }
    }
    return new_node_matrix(ret);
}

static node_idx_t native_math_matrix_set_col(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A_node = get_node(A_idx);
    if (!A_node->is_matrix()) {
        warnf("native_math_matrix_set_col: not a matrix. arg type is %s\n", A_node->type_name());
    }
    matrix_ptr_t A = A_node->as_matrix();

    int col_num = get_node_int(*it++);

    node_idx_t col_idx = *it++;
    node_t *col_node = get_node(col_idx);
    
    matrix_ptr_t ret = A->clone();
    if (col_node->is_vector()) { // fast path
        vector_ptr_t col = col_node->as_vector();
        if(col->size() != ret->height) {
            warnf("native_math_matrix_set_col: vector size != matrix height\n");
            return NIL_NODE;
        }
        for(int y = 0; y < ret->height; ++y) {
            ret->set(col_num, y, col->nth(y));
        }
    } else if (col_node->is_list()) { // fast path
        list_ptr_t col = col_node->as_list();
        int y = 0;
        for(list_t::iterator it(col); it && y < ret->height; ++it, ++y) {
            ret->set(col_num, y, *it);
        }
    } else if(col_node->is_seq()) {
        list_ptr_t col = col_node->as_list();
        int y = 0;
        for(seq_iterator_t it(col_idx); it && y < ret->height; it.next(), ++y) {
            ret->set(col_num, y, it.val);
        }
    }
    return new_node_matrix(ret);
}

static node_idx_t native_math_matrix_cholesky(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A_node = get_node(A_idx);
    if (!A_node->is_matrix()) {
        warnf("native_math_matrix_cholesky: not a matrix. arg type is %s\n", A_node->type_name());
        return NIL_NODE;
    }
    matrix_ptr_t A = A_node->as_matrix();
    int n = A->width;
    if(n != A->height) {
        warnf("native_math_matrix_cholesky: matrix is not square\n");
        return NIL_NODE;
    }
    matrix_ptr_t L = A->clone();
    for(int i = 0; i < n; ++i) {
        for(int j = i; j < n; ++j) {
            double sum = get_node_float(L->get(j, i));
            for(int k = i-1; k >= 0; --k) {
                sum -= get_node_float(L->get(k, i)) * get_node_float(L->get(k, j));
            }
            if(i == j) {
                if(sum <= 0) {
                    warnf("native_math_matrix_cholesky: matrix is not positive definite symmetric\n");
                    return NIL_NODE;
                }
                L->set(i, j, new_node_float(jo_math_sqrt(sum)));
            } else {
                L->set(i,j, new_node_float(sum / get_node_float(L->get(i, i))));
            }
        }
    }
    for(int i = 0; i < n; ++i) {
        for(int j = 0; j < i; ++j) {
            L->set(i, j, ZERO_NODE);
        }
    }
    return new_node_matrix(L);
}

static node_idx_t native_math_matrix_solve_cholesky(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t L_idx = *it++;
    node_t *L_node = get_node(L_idx);
    if (!L_node->is_matrix()) {
        warnf("native_math_matrix_cholesky_solve: L is not a matrix. arg type is %s\n", L_node->type_name());
        return NIL_NODE;
    }
    matrix_ptr_t L = L_node->as_matrix();
    int n = L->width;
    if(n != L->height) {
        warnf("native_math_matrix_cholesky_solve: L matrix is not square\n");
        return NIL_NODE;
    }
    node_idx_t b_idx = *it++;
    node_t *b_node = get_node(b_idx);
    if (!b_node->is_matrix()) {
        warnf("native_math_matrix_cholesky_solve: b is not a matrix. arg type is %s\n", b_node->type_name());
        return NIL_NODE;
    }
    matrix_ptr_t b = b_node->as_matrix();
    if(b->width != n || b->height != 1) {
        warnf("native_math_matrix_cholesky_solve: b matrix is %i by 1\n", n);
        return NIL_NODE;
    }

    matrix_ptr_t x = new_matrix(n, 1);

    // Solve Ly=b
    for(int i = 0; i < n; ++i) {
        double sum = get_node_float(b->get(i, 1));
        for(int k = i-1; k >= 0; --k) {
            sum -= get_node_float(L->get(k,i))*get_node_float(x->get(k,1));
        }
        x->set(i, 1, new_node_float(sum / get_node_float(L->get(i,i))));
    }

    // Solve L^Tx=y
    for(int i = n-1; i >= 0; --i) {
        double sum = get_node_float(x->get(i,1));
        for(int k = i+1; k < n; ++k) {
            sum -= get_node_float(L->get(i,k))*get_node_float(x->get(k,1));
        }
        x->set(i, 1, new_node_float(sum / get_node_float(L->get(i,i))));
    }

    return new_node_matrix(x);
}

static node_idx_t native_math_matrix_reflect_upper(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A_node = get_node(A_idx);
    if (!A_node->is_matrix()) {
        warnf("native_math_matrix_cholesky: not a matrix. arg type is %s\n", A_node->type_name());
        return NIL_NODE;
    }
    matrix_ptr_t A = A_node->as_matrix();
    int n = A->width;
    if(n != A->height) {
        warnf("native_math_matrix_cholesky: matrix is not square\n");
        return NIL_NODE;
    }
    matrix_ptr_t O = A->clone();
    // Sets the lower triangle equal to the upper triangle
    for(int i = 0; i < n; ++i) {
        for(int j = 0; j < i; ++j) {
            O->set(i, j, O->get(j, i));
        }
    }
    return new_node_matrix(O);
}

// Add a number to the diagonal
static node_idx_t native_math_matrix_add_diag(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A_node = get_node(A_idx);
    if (!A_node->is_matrix()) {
        warnf("native_math_matrix_add_diag: not a matrix. arg type is %s\n", A_node->type_name());
        return NIL_NODE;
    }
    double add = get_node_float(*it++);
    matrix_ptr_t A = A_node->as_matrix();
    matrix_ptr_t O = A->clone();
    int mn = A->width < A->height ? A->width : A->height;
    for(int i = 0; i < mn; ++i) {
        O->set(i,i, new_node_float(get_node_float(O->get(i,i)) + add));
    }
    return new_node_matrix(O);
}

// Max number on the diagonal
static node_idx_t native_math_matrix_max_diag(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A_node = get_node(A_idx);
    if (!A_node->is_matrix()) {
        warnf("native_math_matrix_max_diag: not a matrix. arg type is %s\n", A_node->type_name());
        return NIL_NODE;
    }
    matrix_ptr_t A = A_node->as_matrix();
    int mn = A->width < A->height ? A->width : A->height;
    double max = -DBL_MAX;
    for(int i = 0; i < mn; ++i) {
        double v = get_node_float(A->get(i,i));
        max = v > max ? v : max;
    }
    return new_node_float(max);
}

// Add a number to the diagonal
// https://en.wikipedia.org/wiki/Tikhonov_regularization
static node_idx_t native_math_matrix_regularize(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A_node = get_node(A_idx);
    if (!A_node->is_matrix()) {
        warnf("native_math_matrix_regularize: not a matrix. arg type is %s\n", A_node->type_name());
        return NIL_NODE;
    }
    double eps = get_node_float(*it++);
    matrix_ptr_t A = A_node->as_matrix();
    matrix_ptr_t O = A->clone();
    int mn = A->width < A->height ? A->width : A->height;
    // compute max diag
    double max = -DBL_MAX;
    for(int i = 0; i < mn; ++i) {
        double v = get_node_float(A->get(i,i));
        max = v > max ? v : max;
    }
    double add = max * eps;
    for(int i = 0; i < mn; ++i) {
        O->set(i,i, new_node_float(get_node_float(O->get(i,i)) + add));
    }
    return new_node_matrix(O);
}

// Compute the QR decomposition of A
// A is a n x n matrix input
// QT is a n x n matrix output (transpose of Q)
// R is a n x n matrix output
// returns NIL_NODE if input was singular
static node_idx_t native_math_matrix_qr(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A_node = get_node(A_idx);
    if (!A_node->is_matrix()) {
        warnf("native_math_matrix_qr: not a matrix. arg type is %s\n", A_node->type_name());
        return NIL_NODE;
    }
    matrix_ptr_t A = A_node->as_matrix();
    int n = A->width;
    if(n != A->height) {
        warnf("native_math_matrix_cholesky: matrix is not square\n");
        return NIL_NODE;
    }
    matrix_ptr_t R = A->clone();

    double *cd = (double *)malloc(sizeof(double) * n * 2);
    double *c = cd;
    double *d = cd+n;
    bool singular = false;
    for(int k = 0; k < n-1; ++k) {
        double scale = 0;
        for(int i = k; i < n; ++i) {
            scale = jo_math_max(scale, jo_math_abs(get_node_float(R->get(k,i))));
        }
        if(scale == 0) {
            singular = true;
            c[k] = d[k] = 0;
        } else {
            for(int i = k; i < n; ++i) {
                R->set(k, i, new_node_float(get_node_float(R->get(k,i)) / scale));
            }
            double sum = 0;
            for(int i = k; i < n; ++i) {
                sum += jo_math_sqr(get_node_float(R->get(k,i)));
            }
            double sigma = jo_math_sign(jo_math_sqrt(sum), get_node_float(R->get(k,k)));
            double tmp = get_node_float(R->get(k,k)) + sigma;
            R->set(k, k, new_node_float(tmp));
            c[k] = sigma * tmp;
            d[k] = -scale * sigma;
            for(int j = k+1; j < n; ++j) {
                sum = 0;
                for(int i = k; i < n; ++i) {
                    sum += get_node_float(R->get(k,i)) * get_node_float(R->get(j,i));
                }
                double tau = sum / c[k];
                for(int i = k; i < n; ++i) {
                    R->set(j, i, new_node_float(get_node_float(R->get(j,i)) - tau * get_node_float(R->get(k,i))));
                }
            }
        }
    }
    d[n-1] = get_node_float(R->get(n-1,n-1));
    singular |= d[n-1] == 0;
    // Start QT off as an identity matrix
    matrix_ptr_t QT = new_matrix(n, n);
    for(int i = 0; i < n; ++i) {
        QT->set(i,i, ONE_NODE);
    }
    // Calc QT
    for(int k = 0; k < n-1; ++k) {
        if(c[k] == 0) {
            continue;
        }
        for(int j = 0; j < n; ++j) {
            double sum = 0;
            for(int i = k; i < n; ++i) {
                sum += get_node_float(R->get(k,i)) * get_node_float(QT->get(j,i));
            }
            sum /= c[k];
            for(int i = k; i < n; ++i) {
                QT->set(j, i, new_node_float(get_node_float(QT->get(j,i)) - sum * get_node_float(R->get(k,i))));
            }
        }
    }
    // Finish R
    for(int i = 0; i < n; ++i) {
        R->set(i,i, new_node_float(d[i]));
        for(int j = 0; j < i; ++j) {
            R->set(j,i, ZERO_NODE);
        }
    }
    free(cd);
    return singular ? NIL_NODE : new_node_list(list_va(new_node_matrix(QT), new_node_matrix(R)));
}

static node_idx_t native_math_matrix_solve_qr(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t QT_idx = *it++;
    node_t *QT_node = get_node(QT_idx);
    if (!QT_node->is_matrix()) {
        warnf("native_math_matrix_solve_qr: QT is not a matrix. arg type is %s\n", QT_node->type_name());
        return NIL_NODE;
    }
    matrix_ptr_t QT = QT_node->as_matrix();
    int n = QT->width;
    if(n != QT->height) {
        warnf("native_math_matrix_solve_qr: matrix is not square\n");
        return NIL_NODE;
    }

    node_idx_t R_idx = *it++;
    node_t *R_node = get_node(R_idx);
    if (!R_node->is_matrix()) {
        warnf("native_math_matrix_solve_qr: R is not a matrix. arg type is %s\n", R_node->type_name());
        return NIL_NODE;
    }
    matrix_ptr_t R = R_node->as_matrix();
    if(n != R->height || n != R->width) {
        warnf("native_math_matrix_solve_qr: R is not square\n");
        return NIL_NODE;
    }

    node_idx_t b_idx = *it++;
    node_t *b_node = get_node(b_idx);
    if (!b_node->is_matrix()) {
        warnf("native_math_matrix_solve_qr: b is not a matrix. arg type is %s\n", b_node->type_name());
        return NIL_NODE;
    }
    matrix_ptr_t b = b_node->as_matrix();
    if(n != b->height || 1 != b->width) {
        warnf("native_math_matrix_solve_qr: b is not the right size\n");
        return NIL_NODE;
    }

    matrix_ptr_t x = new_matrix(n, 1);

    // Calc x = QT*b
    for(int i = 0; i < n; ++i) {
        double sum = 0;
        for(int j = 0; j < n; ++j) {
            sum += get_node_float(QT->get(j,i)) * get_node_float(b->get(0,j));
        }
        x->set(0, i, new_node_float(sum));
    }
    // Solve R*x = QT*b
    for(int i = n-1; i >= 0; --i) {
        double sum = get_node_float(x->get(i,0));
        for(int j = i+1; j < n; ++j) {
            sum -= get_node_float(R->get(j,i)) * get_node_float(x->get(0,j));
        }
        x->set(0, i, new_node_float(sum / get_node_float(R->get(i,i))));
    }

    return new_node_matrix(x);
}

static node_idx_t native_math_matrix_transpose(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t A_idx = *it++;
    node_t *A_node = get_node(A_idx);
    if (!A_node->is_matrix()) {
        warnf("native_math_matrix_svd: not a matrix. arg type is %s\n", A_node->type_name());
    }
    matrix_ptr_t A = A_node->as_matrix();
    int m = A->height;
    int n = A->width;

    matrix_ptr_t V = new_matrix(m, n);

    for(int i = 0; i < m; ++i) {
        for(int j = 0; j < n; ++j) {
            V->set(j, i, A->get(i,j));
        }
    }

    return new_node_matrix(V);
}

static node_idx_t native_boolean(env_ptr_t env, list_ptr_t args) { 
    return get_node_bool(args->first_value()) ? TRUE_NODE : FALSE_NODE; 
}

static node_idx_t native_is_boolean(env_ptr_t env, list_ptr_t args) { 
    return get_node_type(args->first_value()) == NODE_BOOL ? TRUE_NODE : FALSE_NODE; 
}

// byte
static node_idx_t native_byte(env_ptr_t env, list_ptr_t args) {
    unsigned char b = get_node_int(args->first_value());
    return new_node_int(b);
}

static node_idx_t native_byte_q(env_ptr_t env, list_ptr_t args) {
    node_idx_t idx = args->first_value();
    if(get_node_type(idx) != NODE_INT) {
        return FALSE_NODE;
    }
    long long v = get_node_int(idx);
    return v >= 0 && v <= 255 ? TRUE_NODE : FALSE_NODE;
}

// char
static node_idx_t native_char(env_ptr_t env, list_ptr_t args) {
    signed char b = get_node_int(args->first_value());
    return new_node_int(b);
}

static node_idx_t native_char_q(env_ptr_t env, list_ptr_t args) {
    node_idx_t idx = args->first_value();
    if(get_node_type(idx) != NODE_INT) {
        return FALSE_NODE;
    }
    long long v = get_node_int(idx);
    return v >= -128 && v <= 127 ? TRUE_NODE : FALSE_NODE;
}

// short
static node_idx_t native_short(env_ptr_t env, list_ptr_t args) {
    signed short b = get_node_int(args->first_value());
    return new_node_int(b);
}

static node_idx_t native_short_q(env_ptr_t env, list_ptr_t args) {
    node_idx_t idx = args->first_value();
    if(get_node_type(idx) != NODE_INT) {
        return FALSE_NODE;
    }
    long long v = get_node_int(idx);
    return v >= -32768 && v <= 32767 ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_int_q(env_ptr_t env, list_ptr_t args) {
    node_idx_t idx = args->first_value();
    if(get_node_type(idx) != NODE_INT) {
        return FALSE_NODE;
    }
    long long v = get_node_int(idx);
    return v >= INT_MIN && v <= INT_MAX ? TRUE_NODE : FALSE_NODE;
}

// long
static node_idx_t native_long(env_ptr_t env, list_ptr_t args) {
    signed long long b = get_node_int(args->first_value());
    return new_node_int(b);
}

static node_idx_t native_long_q(env_ptr_t env, list_ptr_t args) {
    node_idx_t idx = args->first_value();
    if(get_node_type(idx) != NODE_INT) {
        return FALSE_NODE;
    }
    long long v = get_node_int(idx);
    return v >= LLONG_MIN && v <= LLONG_MAX ? TRUE_NODE : FALSE_NODE;
}

void jo_basic_math_init(env_ptr_t env) {
	env->set("boolean", new_node_native_function("boolean", &native_boolean, false, NODE_FLAG_PRERESOLVE));
	env->set("boolean?", new_node_native_function("boolean?", &native_is_boolean, false, NODE_FLAG_PRERESOLVE));
    env->set("byte", new_node_native_function("byte", &native_byte, false, NODE_FLAG_PRERESOLVE));
    env->set("byte?", new_node_native_function("byte?", &native_byte_q, false, NODE_FLAG_PRERESOLVE));
    env->set("char", new_node_native_function("char", &native_char, false, NODE_FLAG_PRERESOLVE));
    env->set("char?", new_node_native_function("char?", &native_char_q, false, NODE_FLAG_PRERESOLVE));
    env->set("short", new_node_native_function("short", &native_short, false, NODE_FLAG_PRERESOLVE));
    env->set("short?", new_node_native_function("short?", &native_short_q, false, NODE_FLAG_PRERESOLVE));
    env->set("int", new_node_native_function("int", &native_int, false, NODE_FLAG_PRERESOLVE));
    env->set("int?", new_node_native_function("int?", &native_int_q, false, NODE_FLAG_PRERESOLVE));
    env->set("integer?", new_node_native_function("integer?", &native_integer_q, false, NODE_FLAG_PRERESOLVE));
    env->set("long", new_node_native_function("long", &native_long, false, NODE_FLAG_PRERESOLVE));
    env->set("long?", new_node_native_function("long?", &native_long_q, false, NODE_FLAG_PRERESOLVE));
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
    env->set("Math/srand", new_node_native_function("Math/srand", &native_math_srand, false, NODE_FLAG_PRERESOLVE));
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
    env->set("Math/reflect", new_node_native_function("Math/reflect", &native_math_reflect, false, NODE_FLAG_PRERESOLVE));
    env->set("Math/refract", new_node_native_function("Math/refract", &native_math_refract, false, NODE_FLAG_PRERESOLVE));
    env->set("Math/normalize", new_node_native_function("Math/normalize", &native_math_normalize, false, NODE_FLAG_PRERESOLVE));
    env->set("Math/dot", new_node_native_function("Math/dot", &native_math_dot, false, NODE_FLAG_PRERESOLVE));
    env->set("diag-vector", new_node_native_function("diag-vector", &native_math_diag, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/diag", new_node_native_function("matrix/diag", &native_math_matrix_diag, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/sub", new_node_native_function("matrix/sub", &native_math_matrix_sub, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/mul", new_node_native_function("matrix/mul", &native_math_matrix_mul, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/div", new_node_native_function("matrix/div", &native_math_matrix_div, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/svd", new_node_native_function("matrix/svd", &native_math_matrix_svd, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/pinv", new_node_native_function("matrix/pinv", &native_math_matrix_pinv, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/rand", new_node_native_function("matrix/rand", &native_math_matrix_rand, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/set-row", new_node_native_function("matrix/set-row", &native_math_matrix_set_row, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/set-col", new_node_native_function("matrix/set-col", &native_math_matrix_set_col, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/cholesky", new_node_native_function("matrix/cholesky", &native_math_matrix_cholesky, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/solve-cholesky", new_node_native_function("matrix/solve-cholesky", &native_math_matrix_solve_cholesky, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/reflect-upper", new_node_native_function("matrix/reflect-upper", &native_math_matrix_reflect_upper, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/add-diag", new_node_native_function("matrix/add-diag", &native_math_matrix_add_diag, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/max-diag", new_node_native_function("matrix/max-diag", &native_math_matrix_max_diag, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/regularize", new_node_native_function("matrix/regularize", &native_math_matrix_regularize, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/qr", new_node_native_function("matrix/qr", &native_math_matrix_qr, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/solve-qr", new_node_native_function("matrix/solve-qr", &native_math_matrix_solve_qr, false, NODE_FLAG_PRERESOLVE));
    env->set("matrix/transpose", new_node_native_function("matrix/transpose", &native_math_matrix_transpose, false, NODE_FLAG_PRERESOLVE));
    env->set("vector/sub", new_node_native_function("vector/sub", &native_math_vector_sub, false, NODE_FLAG_PRERESOLVE));
    env->set("vector/div", new_node_native_function("vector/div", &native_math_vector_div, false, NODE_FLAG_PRERESOLVE));
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
    // new_node_var("Math/isNaN", new_node_native_function(&native_math_isnan, false, NODE_FLAG_PRERESOLVE)));
    // new_node_var("Math/isFinite", new_node_native_function(&native_math_isfinite, false, NODE_FLAG_PRERESOLVE)));
    // new_node_var("Math/isInteger", new_node_native_function(&native_math_isinteger, false, NODE_FLAG_PRERESOLVE)));
    // new_node_var("Math/isSafeInteger", new_node_native_function(&native_math_issafeinteger, false, NODE_FLAG_PRERESOLVE)));

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
