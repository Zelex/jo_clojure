#pragma once

#include "jo_stdcpp.h"

static const char b64_table[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

static node_idx_t native_b64_decode_file(env_ptr_t env, list_ptr_t args) {
	list_t::iterator it(args);
	jo_string src_string = get_node(*it++)->as_string();
  const char *src = src_string.c_str();
  size_t len = src_string.length();
	jo_string out_file = get_node(*it++)->as_string();

  int i = 0, j = 0, l = 0;
  size_t size = 0;
  unsigned char buf[3];
  unsigned char tmp[4];

  // alloc
  unsigned char *dec = (unsigned char *) malloc(1);
  if (NULL == dec) { return NIL_NODE; }

  // parse until end of source
  while (len--) {
    // break if char is `=' or not base64 char
    if ('=' == src[j]) { break; }
    if (!(isalnum(src[j]) || '+' == src[j] || '/' == src[j])) { break; }

    // read up to 4 bytes at a time into `tmp'
    tmp[i++] = src[j++];

    // if 4 bytes read then decode into `buf'
    if (4 == i) {
      // translate values in `tmp' from table
      for (i = 0; i < 4; ++i) {
        // find translation char in `b64_table'
        for (l = 0; l < 64; ++l) {
          if (tmp[i] == b64_table[l]) {
            tmp[i] = l;
            break;
          }
        }
      }

      // decode
      buf[0] = (tmp[0] << 2) + ((tmp[1] & 0x30) >> 4);
      buf[1] = ((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2);
      buf[2] = ((tmp[2] & 0x3) << 6) + tmp[3];

      // write decoded buffer to `dec'
      dec = (unsigned char *) realloc(dec, size + 3);
      if (dec != NULL){
        for (i = 0; i < 3; ++i) {
          dec[size++] = buf[i];
        }
      } else {
        return NIL_NODE;
      }

      // reset
      i = 0;
    }
  }

  // remainder
  if (i > 0) {
    // fill `tmp' with `\0' at most 4 times
    for (j = i; j < 4; ++j) {
      tmp[j] = '\0';
    }

    // translate remainder
    for (j = 0; j < 4; ++j) {
        // find translation char in `b64_table'
        for (l = 0; l < 64; ++l) {
          if (tmp[j] == b64_table[l]) {
            tmp[j] = l;
            break;
          }
        }
    }

    // decode remainder
    buf[0] = (tmp[0] << 2) + ((tmp[1] & 0x30) >> 4);
    buf[1] = ((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2);
    buf[2] = ((tmp[2] & 0x3) << 6) + tmp[3];

    // write remainer decoded buffer to `dec'
    dec = (unsigned char *) realloc(dec, size + (i - 1));
    if (dec != NULL){
      for (j = 0; (j < i - 1); ++j) {
        dec[size++] = buf[j];
      }
    } else {
      return NIL_NODE;
    }
  }

  // Make sure we have enough space to add '\0' character at end.
  dec = (unsigned char *) realloc(dec, size + 1);
  if (dec != NULL){
    dec[size] = '\0';
  } else {
    return NIL_NODE;
  }

  jo_spit_file(out_file.c_str(), dec, size);

  return new_node_int(size);
}

void jo_basic_b64_init(env_ptr_t env) {
	env->set("b64/decode-to-file", new_node_native_function("b64/decode-to-file", &native_b64_decode_file, false, NODE_FLAG_PRERESOLVE));
}