#pragma once

#include "httplib.h"

static jo_string http_url_server(jo_string url) {
    int pos = url.find("://");
    if (pos == jo_npos) return "";
    url = url.substr(pos + 3);
    pos = url.find("/");
    if (pos == jo_npos) return url;
    return url.substr(0, pos);
}

static jo_string http_url_path(jo_string url) {
    int pos = url.find("://");
    if (pos == jo_npos) return "";
    url = url.substr(pos + 3);
    pos = url.find("/");
    if (pos == jo_npos) return "/";
    return url.substr(pos);
}

static node_idx_t http_get(jo_string url) {
    jo_string server = http_url_server(url);
    jo_string path = http_url_path(url);
    httplib::Client cli(server.c_str());
    auto res = cli.Get(path.c_str());
    if (res) return new_node_string(res->body.c_str());
    return new_node_int(res->status);
}

static jo_string url_decode(jo_string url) {
    const char *str = url.c_str();
    jo_string result;
    for (int i = 0; i < url.length(); i++) {
        if (str[i] == '%') {
            if (i + 2 < url.length()) {
                int c = 0;
                if (str[i + 1] >= '0' && str[i + 1] <= '9') c = (str[i + 1] - '0') << 4;
                else if (str[i + 1] >= 'a' && str[i + 1] <= 'f') c = (str[i + 1] - 'a' + 10) << 4;
                else if (str[i + 1] >= 'A' && str[i + 1] <= 'F') c = (str[i + 1] - 'A' + 10) << 4;
                if (str[i + 2] >= '0' && str[i + 2] <= '9') c += (str[i + 2] - '0');
                else if (str[i + 2] >= 'a' && str[i + 2] <= 'f') c += (str[i + 2] - 'a' + 10);
                else if (str[i + 2] >= 'A' && str[i + 2] <= 'F') c += (str[i + 2] - 'A' + 10);
                result += (char)c;
                i += 2;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

static jo_string url_encode(const void *data, size_t len) {
    const unsigned char *str = (const unsigned char *)data;
    jo_string result;
    for (int i = 0; i < len; i++) {
        if (str[i] >= '0' && str[i] <= '9') result += str[i];
        else if (str[i] >= 'a' && str[i] <= 'z') result += str[i];
        else if (str[i] >= 'A' && str[i] <= 'Z') result += str[i];
        else if (str[i] == '-' || str[i] == '_' || str[i] == '.' || str[i] == '~') result += str[i];
        else {
            result += '%';
            result += "0123456789ABCDEF"[str[i] >> 4];
            result += "0123456789ABCDEF"[str[i] & 0x0F];
        }
    }
    return result;    
}

static node_idx_t native_url_decode(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_string url = get_node_string(*it++);
    return new_node_string(url_decode(url));
}

static node_idx_t native_url_encode(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_string url = get_node_string(*it++);
    return new_node_string(url_encode(url.c_str(), url.length()));
}

static node_idx_t native_http_get(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_string url = get_node_string(*it++);
    return http_get(url);
}

void jo_basic_http_init(env_ptr_t env) {
	env->set("url/encode", new_node_native_function("url/encode", &native_url_encode, false, NODE_FLAG_PRERESOLVE));
	env->set("url/decode", new_node_native_function("url/decode", &native_url_decode, false, NODE_FLAG_PRERESOLVE));
 
	env->set("http/get", new_node_native_function("http/get", &native_http_get, false, NODE_FLAG_PRERESOLVE));
}


