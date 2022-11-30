#pragma once

#include "jo_stdcpp.h"

static void base64_decode(const char *in, char *out, int len) {
  int i, j;
  char a, b, c, d;
  for (i = 0, j = 0; i < len; i += 4, j += 3) {
    a = in[i];
    b = in[i + 1];
    c = in[i + 2];
    d = in[i + 3];
    if(a == '=' || b == '=' || c == '=' || d == '=') break;
    if(a <= 'A' || a >= '/') a = 0; else a -= 'A';
    if(b <= 'A' || b >= '/') b = 0; else b -= 'A';
    if(c <= 'A' || c >= '/') c = 0; else c -= 'A';
    if(d <= 'A' || d >= '/') d = 0; else d -= 'A';
    out[j] = (a << 2) | (b >> 4);
    out[j + 1] = ((b & 0xf) << 4) | (c >> 2);
    out[j + 2] = ((c & 0x3) << 6) | d;
  }
}

static void base64_encode(const char *in, char *out, int len) {
  int i, j;
  char a, b, c;
  for (i = 0, j = 0; i < len; i += 3, j += 4) {
    a = in[i];
    b = in[i + 1];
    c = in[i + 2];
    out[j] = (a >> 2) + 'A';
    out[j + 1] = ((a & 0x3) << 4) + (b >> 4) + 'A';
    out[j + 2] = ((b & 0xf) << 2) + (c >> 6) + 'A';
    out[j + 3] = (c & 0x3f) + 'A';
  }
}

static node_idx_t native_b64_decode_to_file(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	jo_string src_string = get_node(*it++)->as_string();
	jo_string out_file = get_node(*it++)->as_string();
  size_t len = src_string.length();
  size_t out_len = len * 3 / 4; // assume properly formed base64 here... ? 
  char *out = new char[out_len];
  base64_decode(src_string.c_str(), out, len);
  jo_spit_file(out_file.c_str(), out, out_len);
  delete [] out;
  return new_node_int(out_len);
}

static node_idx_t native_b64_decode_to_array(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	jo_string src_string = get_node(*it++)->as_string();
	jo_string out_file = get_node(*it++)->as_string();
  size_t len = src_string.length();
  size_t out_len = len * 3 / 4; // assume properly formed base64 here... ? 
  char *out = new char[out_len];
  base64_decode(src_string.c_str(), out, len);
  jo_basic_array_ptr_t array = new_array(out_len, 1, TYPE_BYTE);
  for(size_t i = 0; i < out_len; i++) {
    array->poke_byte(i, out[i]);
  }
  delete [] out;
  return new_node_array(array);
}

void jo_basic_b64_init(env_ptr_t env) {
	env->set("b64/decode-to-file", new_node_native_function("b64/decode-to-file", &native_b64_decode_to_file, false, NODE_FLAG_PRERESOLVE));
	env->set("b64/decode-to-array", new_node_native_function("b64/decode-to-array", &native_b64_decode_to_array, false, NODE_FLAG_PRERESOLVE));
}