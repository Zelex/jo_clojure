#pragma once

#ifdef _WIN32
#define USE_WINSOCK
#endif

#ifdef USE_WINSOCK

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#ifndef SD_BOTH
#define SD_BOTH 2
#endif

#define socklen_t int

#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
//#include <sys/socket.h>
//#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

#ifdef USE_WINSOCK
typedef signed long long sockfile_t;
#else
typedef int sockfile_t;
#endif

static bool parse_sockaddr(const char *addr_str, int port, struct sockaddr_in &sock) {
    char justAddr[256];
    memset(&sock, 0, sizeof(sock));
    sock.sin_family = AF_INET;
    strncpy(justAddr, addr_str, sizeof(justAddr)-1);
    justAddr[255] = 0;
    char *p = strchr(justAddr, ':');
    if(p) {
        *p = 0;
        if(port == -1) {
            port = atol(p+1);
        }
    }
    if(port == -1) {
        port = 0;
    }
    sock.sin_port = htons( (unsigned short)port );

    if(justAddr[0] >= '0' && justAddr[0] <= '9') {
        unsigned addr = inet_addr(justAddr);
        if ( addr == INADDR_NONE ) {
            return false;
        }
        *(int *)&sock.sin_addr = addr;
    } else {
        struct hostent *h = gethostbyname(justAddr);
        if(!h){
            return false;
        }
        *(int*)&sock.sin_addr = *(int*)h->h_addr_list[0];
    }
    return true;
}

static bool sock_opt(sockfile_t fd, int option, bool enable) {
    int yes = (enable ? 1 : 0);
    int err = setsockopt(fd, SOL_SOCKET, option, (char*)&yes, sizeof(yes));
    return !err;
}

static node_idx_t native_socket(env_ptr_t env, list_ptr_t args) {
    sockfile_t fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        return NIL_NODE;
    }
    return new_node_int(fd);
}

static node_idx_t native_socket_connect(env_ptr_t enve, list_ptr_t args) {
    list_t::iterator it(args);
    sockfile_t fd = get_node_int(*it++);
    jo_string addr_str = get_node_string(*it++);
    int port = get_node_int(*it++);

    struct sockaddr_in sock;
    if (!parse_sockaddr(addr_str.c_str(), port, sock)) {
        return FALSE_NODE;
    }

    if (connect(fd, (struct sockaddr *)&sock, sizeof(sock)) == -1) {
        return FALSE_NODE;
    }

    return TRUE_NODE;
}

static node_idx_t native_socket_bind(env_ptr_t enve, list_ptr_t args) {
    list_t::iterator it(args);
    sockfile_t fd = get_node_int(*it++);
    jo_string addr_str = get_node_string(*it++);
    int port = get_node_int(*it++);

    struct sockaddr_in sock;
    if (!parse_sockaddr(addr_str.c_str(), port, sock)) {
        return FALSE_NODE;
    }

    if(bind(fd, (struct sockaddr*)&sock, sizeof(sock))) {
        return FALSE_NODE;
    }

    return TRUE_NODE;
}

static node_idx_t native_socket_no_delay(env_ptr_t env, list_ptr_t args) {
    int fd = get_node_int(args->first_value());
    int optval = 1;
    int err = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&optval, sizeof(optval));
    return err ? FALSE_NODE : TRUE_NODE;
}

static node_idx_t native_socket_no_blocking(env_ptr_t env, list_ptr_t args) {
    int fd = get_node_int(args->first_value());
#ifdef _WIN32
    u_long mode = 1;
    int e = ioctlsocket(fd, FIONBIO, &mode);
    if (e != NO_ERROR) {
        warnf("ioctlsocket failed with error: %ld\n", e);
    }
#else
    fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
    return NIL_NODE;
}

static node_idx_t native_socket_reuse_addr(env_ptr_t env, list_ptr_t args) {
    int fd = get_node_int(args->first_value());
    bool enable = get_node_bool(args->second_value());
    return sock_opt(fd, SO_REUSEADDR, enable) ? TRUE_NODE : FALSE_NODE;
}

static node_idx_t native_socket_reuse_port(env_ptr_t env, list_ptr_t args) {
#ifdef SO_REUSEPORT
    int fd = get_node_int(args->first_value());
    bool enable = get_node_bool(args->second_value());
    return sock_opt(fd, SO_REUSEPORT, enable) ? TRUE_NODE : FALSE_NODE;
#else
    return TRUE_NODE;
#endif
}

static node_idx_t native_socket_listen(env_ptr_t env, list_ptr_t args) {
    int fd = get_node_int(args->first_value());
    int backlog = get_node_int(args->second_value());
	return listen(fd, backlog) ? FALSE_NODE : TRUE_NODE;
}

static node_idx_t native_socket_accept(env_ptr_t env, list_ptr_t args) {
    int fd = get_node_int(args->first_value());
    int wait_for = get_node_int(args->second_value());

    fd_set rfds, wfds, efds;
    FD_ZERO(&efds); FD_SET(fd, &efds);
    FD_ZERO(&wfds); 
    FD_ZERO(&rfds); FD_SET(fd, &rfds);
    struct timeval tv = {0, wait_for};
    if (select(fd+1, &rfds, &wfds, &efds, &tv) <= 0 || FD_ISSET(fd, &efds) || FD_ISSET(fd, &rfds) == 0 ) {
        // Nothing connecting
        return NIL_NODE;
    }    

    struct sockaddr_in sock;
    socklen_t socklen = sizeof(sock);
	sockfile_t cfd = accept(fd, (struct sockaddr *)&sock, &socklen);
    if(cfd == -1) {
        // Connection error
        return NIL_NODE;
    }

    return new_node_int(cfd);
}

static node_idx_t native_socket_can_read_q(env_ptr_t env, list_ptr_t args) {
    int fd = get_node_int(args->first_value());
    fd_set rfds, wfds, efds;
    FD_ZERO(&efds); FD_SET(fd, &efds);
    FD_ZERO(&wfds); 
    FD_ZERO(&rfds); FD_SET(fd, &rfds);
    struct timeval tv = {0, 0}; 
    if (select(fd+1, &rfds, &wfds, &efds, &tv) <= 0 || FD_ISSET(fd, &efds) || FD_ISSET(fd, &rfds) == 0 ) {
        return FALSE_NODE;
    }
    return TRUE_NODE;
}

static node_idx_t native_socket_live_q(env_ptr_t env, list_ptr_t args) {
    int fd = get_node_int(args->first_value());
    fd_set myset, exceptset;
    struct timeval tv = { 0, 0 };
    int res;

    FD_ZERO( &myset );
    FD_SET( fd, &myset );
    FD_ZERO( &exceptset );
    FD_SET( fd, &exceptset );

    res = select( fd + 1, NULL, &myset, &exceptset, &tv );
    if(res < 0) {
        return FALSE_NODE;
    }

#ifndef USE_WINSOCK
    int valopt;
    socklen_t l = sizeof(valopt);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&valopt, &l) >= 0) {
        if (valopt) {
            return FALSE_NODE;
        }
    }
#endif
    return TRUE_NODE;
}

static node_idx_t native_socket_disconnect(env_ptr_t env, list_ptr_t args) {
    int fd = get_node_int(args->first_value());
#ifdef USE_WINSOCK
	shutdown(fd, SD_BOTH);
	closesocket(fd);
#else
	close(fd);
#endif
    return NIL_NODE;
}

static node_idx_t native_socket_recv(env_ptr_t env, list_ptr_t args) {
    int fd = get_node_int(args->first_value());
    int tmp_size = get_node_int(args->second_value());
    tmp_size = tmp_size <= 0 ? 8192 : tmp_size;
    char *tmp = (char*)jo_alloca(tmp_size+1);
    int count = recv(fd, (char*)tmp, tmp_size, 0);
    tmp[count] = 0; // make sure its zero terminated
    return new_node_string(tmp);
}

static node_idx_t native_socket_send(env_ptr_t env, list_ptr_t args) {
    int fd = get_node_int(args->first_value());
    jo_string tmp = get_node_string(args->second_value());
#ifdef USE_WINSOCK
    int written = ::send(fd, tmp.c_str(), tmp.size, 0);
#else
    int written = ::send(fd, tmp.c_str(), tmp.size, MSG_NOSIGNAL);
#endif
    return new_node_int(written);
}

void jo_basic_socket_init(env_ptr_t env) {
	env->set("socket", new_node_native_function("socket", &native_socket, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/bind", new_node_native_function("socket/bind", &native_socket_bind, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/connect", new_node_native_function("socket/connect", &native_socket_connect, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/disconnect", new_node_native_function("socket/disconnect", &native_socket_disconnect, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/listen", new_node_native_function("socket/listen", &native_socket_listen, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/accept", new_node_native_function("socket/accept", &native_socket_accept, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/live?", new_node_native_function("socket/live?", &native_socket_live_q, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/no-delay", new_node_native_function("socket/no-delay", &native_socket_no_delay, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/no-blocking", new_node_native_function("socket/no-blocking", &native_socket_no_blocking, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/reuse-addr", new_node_native_function("socket/reuse-addr", &native_socket_reuse_addr, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/reuse-port", new_node_native_function("socket/reuse-port", &native_socket_reuse_port, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/can-read?", new_node_native_function("socket/can-read?", &native_socket_can_read_q, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/recv", new_node_native_function("socket/recv", &native_socket_recv, false, NODE_FLAG_PRERESOLVE));
	env->set("socket/send", new_node_native_function("socket/send", &native_socket_send, false, NODE_FLAG_PRERESOLVE));

}
