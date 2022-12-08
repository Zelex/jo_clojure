#pragma once

#include "jo_stdcpp.h"

static void base64_decode(const char *in, unsigned char *out, int len) {
	//printf("base64_decode(%s, %d)\n", in, len);
	int i, j;
	unsigned char a, b, c, d;
	for (i = 0, j = 0; i < len; i += 4, j += 3) {
		a = in[i++];
		b = i < len ? in[i++] : 0;
		if(a == '=' || b == '=' || i >= len) break;
		a -= 'A';
		b -= 'A';
		out[j++] = (a << 2) | (b >> 4);
		c = i < len ? in[i++] : 0;
		if(c == '=' || i >= len) break;
		c -= 'A';
		out[j++] = ((b & 0xf) << 4) | (c >> 2);
		d = i < len ? in[i++] : 0;
		if(d == '=' || i >= len) break;
		d -= 'A';
		out[j++] = ((c & 0x3) << 6) | d;
		//printf("%c,%c,%c\n", out[j-3], out[j-2], out[j-1]);
	}
}

static void base64_encode(const unsigned char *in, char *out, int len) {
	int i, j;
	unsigned char a, b, c;
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
	unsigned char *out = (unsigned char*)calloc(1, out_len);
	base64_decode(src_string.c_str(), out, len);
	jo_spit_file(out_file.c_str(), out, out_len);
	free(out);
	return new_node_int(out_len);
}

static node_idx_t native_b64_decode_to_array(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	jo_string src_string = get_node(*it++)->as_string();
	jo_string out_file = get_node(*it++)->as_string();
	size_t len = src_string.length();
	size_t out_len = len * 3 / 4; // assume properly formed base64 here... ? 
	unsigned char *out = new unsigned char[out_len];
	base64_decode(src_string.c_str(), out, len);
	jo_basic_array_ptr_t array = new_array(out_len, 1, TYPE_BYTE);
	for(size_t i = 0; i < out_len; i++) {
		array->poke_byte(i, out[i]);
	}
	delete [] out;
	return new_node_array(array);
}

static node_idx_t native_b64_encode_from_array(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	jo_basic_array_ptr_t array = get_node(*it++)->t_object.cast<jo_basic_array_t>();
	jo_string out_file = get_node(*it++)->as_string();
	size_t len = array->length();
	size_t out_len = len * 4 / 3; // assume properly formed base64 here... ? 
	char *out = new char[out_len];
	unsigned char *in = new unsigned char[len];
	for(size_t i = 0; i < len; i++) {
		in[i] = array->peek_byte(i);
	}
	base64_encode(in, out, len);
	jo_spit_file(out_file.c_str(), out, out_len);
	delete [] out;
	delete [] in;
	return new_node_int(out_len);
}

void jo_basic_b64_init(env_ptr_t env) {
	env->set("b64/decode-to-file", new_node_native_function("b64/decode-to-file", &native_b64_decode_to_file, false, NODE_FLAG_PRERESOLVE));
	env->set("b64/decode-to-array", new_node_native_function("b64/decode-to-array", &native_b64_decode_to_array, false, NODE_FLAG_PRERESOLVE));
	env->set("b64/encode-from-array", new_node_native_function("b64/encode-from-array", &native_b64_encode_from_array, false, NODE_FLAG_PRERESOLVE));
}
