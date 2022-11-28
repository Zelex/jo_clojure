#ifndef JO_RESIZE_H
#define JO_RESIZE_H
#pragma once

#include <stdlib.h>
#include <math.h>
//#include <malloc.h>
#include <memory.h>
#include <type_traits>

#ifdef _OPENMP
#define JO_RESIZE_NUM_THREADS 8
#else
#define JO_RESIZE_NUM_THREADS 1
#endif

typedef float (filter_func_t)(float);

static float jo_resize_filter_box(float t) {
    return t > -0.5f && t <= 0.5f ? 1.f : 0.f;
}
#define jo_resize_filter_box_support (1)

static float jo_resize_filter_triangle(float t) {
    if(t < 0) t = -t;
    if(t < 1) return 1 - t;
    return 0;
}
#define jo_resize_filter_triangle_support (3)

static float jo_resize_filter_bell(float t) {
    if(t < 0) t = -t;
    if(t < 0.5f) return 0.75f - (t * t);
    if(t < 1.5f) {
        t = (t - 1.5f);
        return 0.5f * (t * t);
    }
    return 0;
}
#define jo_resize_filter_bell_support (3)

static float jo_resize_filter_bspline(float t) {
    if(t < 0) t = -t;
    if(t < 1) {
        float tt = t * t;
        return((0.5f * tt * t) - tt + (2.f / 3.f));
    } else if(t < 2) {
        t = 2 - t;
        return((1.f / 6.f) * (t * t * t));
    }
    return(0.0);
}
#define jo_resize_filter_bspline_support (4)

static float jo_resize_filter_sinc(float t) {
    t *= 3.1415926f;
    if(t != 0) return sinf(t) / t;
    return 1;
}

static float jo_resize_filter_lanczos3(float t) {
    if(t < 0) t = -t;
    if(t < 3) return jo_resize_filter_sinc(t) * jo_resize_filter_sinc(t/3);
    return 0;
}
#define jo_resize_filter_lanczos3_support (6)

static float jo_resize_filter_catmullrom(float x) {
    x = (float)fabs(x);
    if (x < 1.0f)
        return 1 - x*x*(2.5f - 1.5f*x);
    else if (x < 2.0f)
        return 2 - x*(4 + x*(0.5f*x - 2.5f));

    return (0.0f);
}
#define jo_resize_filter_catmullrom_support (4)

static float jo_resize_filter_mitchell(float t) {
    t = (float)fabs(t);
    if (t < 1.0f)
        return (16 + t*t*(21 * t - 36))/18;
    else if (t < 2.0f)
        return (32 + t*(-60 + t*(36 - 7*t)))/18;
    return (0.0f);
}
#define jo_resize_filter_mitchell_support (4)

static inline void* jo_resize_alloc(size_t size, size_t align) {
    // round up size to multiple of 256 bytes
    size = (size + 255) & -256;
    void *result = 0;
#ifdef _MSC_VER 
    result = _aligned_malloc(size, align);
#else 
    posix_memalign(&result, align, size);
#endif
    return result;
}

static inline void jo_resize_free(void *ptr) {
    if(ptr) {
#ifdef _MSC_VER 
        _aligned_free(ptr);
#else 
        free(ptr);
#endif
    }
}

struct jo_resize_contrib_t {
    unsigned short first;
    unsigned short n;
};

struct jo_resize_weights_t {
    float *weights;
    jo_resize_contrib_t *contrib;
    float width;
    int res;
};

template<const int ncomp, typename T>
static jo_resize_weights_t jo_resize_make_weights(int from_res, int to_res, filter_func_t filter, const int support) {
    jo_resize_weights_t ret;
    ret.res = to_res;
    int max_res = from_res > to_res ? from_res : to_res;
    float scale = (float)to_res / (float)from_res;
    float delta = (0.5f / scale) - 0.5f;
    ret.width = support / 2.f;
    ret.width = scale < 1 ? ret.width / scale : ret.width;

    int weights_step = (int)(ret.width * 2 + 1 + 1);
    ret.weights = (float *)jo_resize_alloc(weights_step*to_res*sizeof(float), 64);
    ret.contrib = (jo_resize_contrib_t*)jo_resize_alloc(sizeof(jo_resize_contrib_t)*to_res, 64);

    jo_resize_contrib_t * __restrict contrib = ret.contrib;

    float min_weight = 1.f / (1<<20);
    if(sizeof(T) == 1) min_weight = 1.f / (1<<10);
    if(sizeof(T) == 2) min_weight = 1.f / (1<<18);

    if(scale < 1.0f) {
        float * __restrict W = ret.weights;
        for(int i = 0; i < to_res; ++i) {
            float center = i / scale + delta;
            int left = (int)(center - ret.width + 1);
            int right = (int)(center + ret.width);
            int n = 0;
            int first = INT_MAX;
            float total_weight = 0;
            for(int j = left; j <= right; ++j) {
                if((unsigned)j<(unsigned)from_res) {
                    float w = center - j;
                    w = filter(w * scale) * scale;
                    first = j < first ? j : first;
                    if(j == first && (w < min_weight && w > -min_weight)) {
                        first = INT_MAX; // not first!
                    } else {
                        *W++ = w;
                        ++n;
                        total_weight += w;
                    }
                }
            }
            // trim off trailing low weights
            while(n > 1 && fabsf(W[-1]) < min_weight) { --n; --W; total_weight -= *W; }
            // normalize weights
            if(total_weight < (1 - min_weight)) {
                float weight_scale = 1.f / total_weight;
                for(int i = -n; i < 0; ++i) { W[i] *= weight_scale; }
            }
            contrib[i].n = n;
            contrib[i].first = first; 
        }
    } else {
        float * __restrict W = ret.weights;
        for(int i = 0; i < to_res; ++i) {
            float center = i / scale + delta;
            int left = (int)(center - ret.width + 1);
            int right = (int)(center + ret.width);
            int n = 0;
            int first = INT_MAX;
            float total_weight = 0;
            for(int j = (int)left; j <= right; ++j) {
                if((unsigned)j<(unsigned)from_res) {
                    float w = center - j;
                    w = filter(w);
                    first = j < first ? j : first;
                    if(j == first && (w < min_weight && w > -min_weight)) {
                        first = INT_MAX; // not first!
                    } else {
                        *W++ = w;
                        ++n;
                        total_weight += w;
                    }
                }
            }
            // trim off trailing low weights
            while(n > 1 && fabsf(W[-1]) < min_weight) { --n; --W; total_weight -= *W; }
            // normalize weights
            if(total_weight < (1 - min_weight)) {
                float weight_scale = 1.f / total_weight;
                for(int i = -n; i < 0; ++i) { W[i] *= weight_scale; }
            }
            contrib[i].n = n;
            contrib[i].first = first; 
        }
    }
    return ret;
}

static inline void jo_resize_free_weights(jo_resize_weights_t *w) {
    jo_resize_free(w->weights);
    jo_resize_free(w->contrib);
    memset(w, 0, sizeof(*w));
}

static inline jo_resize_weights_t jo_resize_combine_weights(const jo_resize_weights_t *w0, const jo_resize_weights_t *w1) {
    jo_resize_weights_t ret;
    ret.res = w1->res;

    // Note: this is a very conservative estimate of the max step .. in reality its lower than this.
    int iwidth0 = (int)(w0->width*2+1+1);
    int iwidth1 = (int)(w1->width*2+1+1);
    int weights_step = iwidth0*2 + iwidth1;
    ret.width = weights_step/2;

    ret.weights = (float *)jo_resize_alloc(weights_step*w1->res*sizeof(float), 64);
    ret.contrib = (jo_resize_contrib_t*)jo_resize_alloc(sizeof(jo_resize_contrib_t)*w1->res, 64);

    memset(ret.weights, 0, weights_step*w1->res*sizeof(float));

    float **W0_map = (float**)jo_resize_alloc(sizeof(*W0_map)*w0->res, 64);
    float *W0 = w0->weights;
    for(int i = 0; i < w0->res; ++i) {
        W0_map[i] = W0;
        W0 += w0->contrib[i].n;
    }

    float *W = ret.weights;
    float *W1 = w1->weights;
    int max_step = 0;
    for(int i = 0; i < w1->res; ++i) {
        jo_resize_contrib_t *c = ret.contrib + i;
        jo_resize_contrib_t *c1 = w1->contrib + i;
        int first = w0->contrib[c1->first].first; // first is always first of the first (... confusing I know ...)
        int last = w0->contrib[c1->first+c1->n-1].first + w0->contrib[c1->first+c1->n-1].n; // last is always last of the last
        int step = last - first;
        max_step = step > max_step ? step : max_step;
        float total_w = 0;
        for(int j = 0; j < c1->n; ++j) {
            float weight1 = *W1++;
            jo_resize_contrib_t *c0 = w0->contrib + c1->first + j;
            W0 = W0_map[c1->first + j];
            for(int k = 0; k < c0->n; ++k) {
                float weight = weight1 * W0[k];
                total_w += weight;
                W[c0->first - first + k] += weight;
            }
        }
        float w_scale = 1.f / total_w;
        for(int j = 0; j < step; ++j) {
            W[j] *= w_scale;
        }
        W += step;
        c->first = first;
        c->n = step;
    }

    // give it an accurate number since we actually know it now....
    ret.width = max_step/2;

    jo_resize_free(W0_map);
    return ret;
}

template<const int ncomp, typename T_in, typename T_out>
void jo_resize(T_in *from, int from_width, int from_height, int from_stride, T_out *to, int to_width, int to_height, int to_stride, const jo_resize_weights_t &wx, const jo_resize_weights_t &wy) {
    int from_max = from_width > from_height ? from_width : from_height;
    int to_max = to_width > to_height ? to_width : to_height;
    int all_max = from_max > to_max ? from_max : to_max;
    float * __restrict ftmp = (float *)jo_resize_alloc(all_max*ncomp*sizeof(*ftmp),64);

    int ring_size = wy.width * 2 + 1;
    float * __restrict ring_buf = (float *)jo_resize_alloc(to_width*ring_size*ncomp*sizeof(*ring_buf),64);

    const float * __restrict W_x = wx.weights;
    const float * __restrict W_y = wy.weights;
    const jo_resize_contrib_t * __restrict C_x = wx.contrib;
    const jo_resize_contrib_t * __restrict C_y = wy.contrib;

    const float min_val = std::numeric_limits<T_out>::lowest();
    const float max_val = std::numeric_limits<T_out>::max();

    T_out * __restrict out = to;
    int scanline_h = 0, scanline_v = 0;
    while(scanline_v < to_height) {
        if(scanline_h < from_height) {
            const T_in *F = (T_in*)((char*)from + scanline_h*from_stride);
            for(int i = 0; i < from_width*ncomp; ++i) {
                ftmp[i] = F[i];
            }
            float *H = ring_buf + (scanline_h%ring_size)*to_width*ncomp;
            const float * __restrict W = W_x;
            for(int i = 0; i < to_width; ++i) {
                int t = C_x[i].first * ncomp;
                int n = C_x[i].n;
                const float * __restrict D = ftmp + t;
                float w0,w1,w2,w3;
                float w = *W++;
                w0 = D[0] * w;
                if(ncomp > 1) w1 = D[1] * w;
                if(ncomp > 2) w2 = D[2] * w;
                if(ncomp > 3) w3 = D[3] * w;
                D += ncomp;
                for(int j = 1; j < n; ++j) {
                    w = *W++;
                    w0 += D[0] * w;
                    if(ncomp > 1) w1 += D[1] * w;
                    if(ncomp > 2) w2 += D[2] * w;
                    if(ncomp > 3) w3 += D[3] * w;
                    D += ncomp;
                }
                H[0] = w0;
                if(ncomp > 1) H[1] = w1;
                if(ncomp > 2) H[2] = w2;
                if(ncomp > 3) H[3] = w3;
                H += ncomp;
            }
            ++scanline_h;
        }

        while(scanline_v < to_height && scanline_h >= (C_y[scanline_v].first + C_y[scanline_v].n)) {
            int t = C_y[scanline_v].first;
            int n = C_y[scanline_v].n;

            const float init_val = std::is_same<T_out, float>::value ? 0.0f : 0.5f;
                                   
            float * __restrict H = ring_buf + (t%ring_size)*to_width*ncomp;
            float * __restrict V = ftmp;
            float w = *W_y++;
            for(int i = 0; i < to_width*ncomp; ++i) {
                V[i] = init_val + H[i] * w;
            }
            H += to_width*ncomp;
            for(int j = 1; j < n; ++j) {
                H = ring_buf + ((t+j)%ring_size)*to_width*ncomp;
                w = *W_y++;
                for(int i = 0; i < to_width*ncomp; ++i) {
                    V[i] += H[i] * w;
                }
            }
            for(int i = 0; i < to_width*ncomp; ++i) {
                float w = V[i];
                if(!std::is_same<T_out, float>::value) {
                    if(!std::is_same<T_in, T_out>::value && !std::is_same<T_in, float>::value) {
                        w *= (float)std::numeric_limits<T_out>::max() / (float)(std::numeric_limits<T_in>::max()); 
                    }
                    w = w < min_val ? min_val : w;
                    w = w > max_val ? max_val : w;
                }
                out[i] = (T_out)w;
            }
            out = (T_out*)((char*)out + to_stride);
            ++scanline_v;
        }
    }

    jo_resize_free(ring_buf);
    jo_resize_free(ftmp);
}

template<const int ncomp, typename T_in, typename T_out>
void jo_resize_omp(T_in *from, int from_width, int from_height, int from_stride, T_out *to, int to_width, int to_height, int to_stride, const jo_resize_weights_t &wx, const jo_resize_weights_t &wy) {
    int from_max = from_width > from_height ? from_width : from_height;
    int to_max = to_width > to_height ? to_width : to_height;
    float * __restrict ftmp = (float*)jo_resize_alloc(sizeof(*ftmp)*from_max*ncomp*JO_RESIZE_NUM_THREADS, 64);
    T_out * __restrict hdst = (T_out *)jo_resize_alloc(to_width*from_height*ncomp*sizeof(*hdst),64);

    const float * __restrict W_x = wx.weights;
    const float * __restrict W_y = wy.weights;
    const jo_resize_contrib_t * __restrict C_x = wx.contrib;
    const jo_resize_contrib_t * __restrict C_y = wy.contrib;

    float min_weight = 1.f / (1<<20);
    if(sizeof(T_out) == 1) min_weight = 1.f / (1<<10);
    if(sizeof(T_out) == 2) min_weight = 1.f / (1<<18);

    const float min_val = std::numeric_limits<T_out>::lowest();
    const float max_val = std::numeric_limits<T_out>::max();

    // apply filter to zoom horizontally from -> hdst
#pragma omp parallel for
    for(int l = 0; l < JO_RESIZE_NUM_THREADS; ++l) {
        float *R = ftmp + l*from_width*ncomp;
        int kstart = l*from_height/JO_RESIZE_NUM_THREADS;
        int kend = (l+1)*from_height/JO_RESIZE_NUM_THREADS;
        kend = kend > from_height ? from_height : kend;
        for(int k = kstart; k < kend; ++k) {
            const T_in *F = (T_in*)((char*)from + k*from_stride);
            for(int i = 0; i < from_width*ncomp; ++i) {
                R[i] = F[i];
            }
            T_out *O = hdst+k*ncomp;
            const float *W = W_x;
            for(int i = 0; i < to_width; ++i) {
                int t = C_x[i].first;
                int n = C_x[i].n;
                const float *D = R + t;
                float w0,w1,w2,w3;
                float init_val = std::is_same<T_out, float>::value ? 0.0f : 0.5f;
                float w = *W++;
                w0 = init_val + D[0] * w;
                if(ncomp > 1) w1 = init_val + D[1] * w;
                if(ncomp > 2) w2 = init_val + D[2] * w;
                if(ncomp > 3) w3 = init_val + D[3] * w;
                D += ncomp;
                for(int j = 1; j < n; ++j) {
                    w = *W++;
                    w0 += D[0] * w;
                    if(ncomp > 1) w1 += D[1] * w;
                    if(ncomp > 2) w2 += D[2] * w;
                    if(ncomp > 3) w3 += D[3] * w;
                    D += ncomp;
                }
                if(!std::is_same<T_out, float>::value) {
                    if(!std::is_same<T_in, T_out>::value && !std::is_same<T_in, float>::value) {
                        w *= (float)std::numeric_limits<T_out>::max() / (float)(std::numeric_limits<T_in>::max()); 
                    }
                    w0 = w0 < min_val ? min_val : w0;
                    if(ncomp > 1) w1 = w1 < min_val ? min_val : w1;
                    if(ncomp > 2) w2 = w2 < min_val ? min_val : w2;
                    if(ncomp > 3) w3 = w3 < min_val ? min_val : w3;
                    w0 = w0 > max_val ? max_val : w0;
                    if(ncomp > 1) w1 = w1 > max_val ? max_val : w1;
                    if(ncomp > 2) w2 = w2 > max_val ? max_val : w2;
                    if(ncomp > 3) w3 = w3 > max_val ? max_val : w3;
                }
                O[0] = (T_out)w0;
                if(ncomp > 1) O[1] = (T_out)w1;
                if(ncomp > 2) O[2] = (T_out)w2;
                if(ncomp > 3) O[3] = (T_out)w3;
                O += from_height*ncomp;
            }
        }
    }

    // apply filter to zoom vertically from hdst -> to
#pragma omp parallel for
    for(int l = 0; l < JO_RESIZE_NUM_THREADS; ++l) {
        float *R = ftmp + l*from_height*ncomp;
        int kstart = l*to_width/JO_RESIZE_NUM_THREADS;
        int kend = (l+1)*to_width/JO_RESIZE_NUM_THREADS;
        kend = kend > to_width ? to_width : kend;
        for(int k = kstart; k < kend; ++k) {
            const T_out *F = hdst + k*from_height*ncomp;
            for(int i = 0; i < from_height*ncomp; ++i) {
                R[i] = F[i];
            }
            T_out *O = to + k*ncomp;
            const float *W = W_y;
            for(int i = 0; i < to_height; ++i) {
                int t = C_y[i].first;
                int n = C_y[i].n;
                const float *D = R + t;
                float w0,w1,w2,w3;
                float init_val = std::is_same<T_out, float>::value ? 0.0f : 0.5f;
                float w = *W++;
                w0 = init_val + D[0] * w;
                if(ncomp > 1) w1 = init_val + D[1] * w;
                if(ncomp > 2) w2 = init_val + D[2] * w;
                if(ncomp > 3) w3 = init_val + D[3] * w;
                D += ncomp;
                for(int j = 1; j < n; ++j) {
                    w = *W++;
                    w0 += D[0] * w;
                    if(ncomp > 1) w1 += D[1] * w;
                    if(ncomp > 2) w2 += D[2] * w;
                    if(ncomp > 3) w3 += D[3] * w;
                    D += ncomp;
                }
                if(!std::is_same<T_out, float>::value) {
                    w0 = w0 < min_val ? min_val : w0;
                    if(ncomp > 1) w1 = w1 < min_val ? min_val : w1;
                    if(ncomp > 2) w2 = w2 < min_val ? min_val : w2;
                    if(ncomp > 3) w3 = w3 < min_val ? min_val : w3;
                    w0 = w0 > max_val ? max_val : w0;
                    if(ncomp > 1) w1 = w1 > max_val ? max_val : w1;
                    if(ncomp > 2) w2 = w2 > max_val ? max_val : w2;
                    if(ncomp > 3) w3 = w3 > max_val ? max_val : w3;
                }
                O[0] = (T_out)w0;
                if(ncomp > 1) O[1] = (T_out)w1;
                if(ncomp > 2) O[2] = (T_out)w2;
                if(ncomp > 3) O[3] = (T_out)w3;
                O = (T_out*)((char*)O + to_stride);
            }
        }
    }

    jo_resize_free(ftmp);
    jo_resize_free(hdst);
}

template<const int ncomp, typename T_in, typename T_out>
void jo_resize(T_in *from, int from_width, int from_height, int from_stride, T_out *to, int to_width, int to_height, int to_stride, filter_func_t filter, const int support) {
    jo_resize_weights_t wx = jo_resize_make_weights<ncomp, T_out>(from_width, to_width, filter, support);
    jo_resize_weights_t wy = jo_resize_make_weights<ncomp, T_out>(from_height, to_height, filter, support);

    jo_resize<ncomp>(from, from_width, from_height, from_stride, to, to_width, to_height, to_stride, wx, wy);

    jo_resize_free_weights(&wx);
    jo_resize_free_weights(&wy);
}


template<const int ncomp, typename T_in, typename T_out>
void jo_resize_omp(T_in *from, int from_width, int from_height, int from_stride, T_out *to, int to_width, int to_height, int to_stride, filter_func_t filter, const int support) {
    jo_resize_weights_t wx = jo_resize_make_weights<ncomp, T_out>(from_width, to_width, filter, support);
    jo_resize_weights_t wy = jo_resize_make_weights<ncomp, T_out>(from_height, to_height, filter, support);

    jo_resize_omp<ncomp>(from, from_width, from_height, from_stride, to, to_width, to_height, to_stride, wx, wy);

    jo_resize_free_weights(&wx);
    jo_resize_free_weights(&wy);
}

#endif // JO_RESIZE_H

