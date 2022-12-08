#pragma once

#include "jo_stdcpp.h"

static void base64_decode(const char *in, unsigned char *out, int len) {
	static unsigned char decode_tbl[] = {
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
		64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
		64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	};

	for(int i = 0, j = 0; i < len;) {
		unsigned a = in[i] == '=' ? 0 & i++ : decode_tbl[(unsigned char)in[i++]];
		unsigned b = in[i] == '=' ? 0 & i++ : decode_tbl[(unsigned char)in[i++]];
		unsigned c = in[i] == '=' ? 0 & i++ : decode_tbl[(unsigned char)in[i++]];
		unsigned d = in[i] == '=' ? 0 & i++ : decode_tbl[(unsigned char)in[i++]];

		unsigned triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);

		out[j++] = (triple >> 2 * 8) & 0xFF;
		out[j++] = (triple >> 1 * 8) & 0xFF;
		out[j++] = (triple >> 0 * 8) & 0xFF;
	}
}

static void base64_encode(const unsigned char *in, char *out, int len) {
    static char encode_tbl[] = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
      'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
      'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
      'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
      'w', 'x', 'y', 'z', '0', '1', '2', '3',
      '4', '5', '6', '7', '8', '9', '+', '/'
    };
	int i, j;
	unsigned char a, b, c;
	for (i = 0, j = 0; i < len; i += 3, j += 4) {
		a = in[i];
		b = in[i + 1];
		c = in[i + 2];
		out[j] = encode_tbl[(a >> 2)];
		out[j + 1] = encode_tbl[((a & 0x3) << 4) + (b >> 4)];
		out[j + 2] = encode_tbl[((b & 0xf) << 2) + (c >> 6)];
		out[j + 3] = encode_tbl[(c & 0x3f)];
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
