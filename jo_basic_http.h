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

static node_idx_t native_http_get(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_string url = get_node_string(*it++);
    return http_get(url);
}

void jo_basic_http_init(env_ptr_t env) {
	env->set("http/get", new_node_native_function("http/get", &native_http_get, false, NODE_FLAG_PRERESOLVE));
}


