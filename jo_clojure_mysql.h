// Super basic Mysql wrapper. More will probably be needed here... TODO

#include <mysql/mysql.h>

struct jo_clojure_mysql_t : jo_object {
    MYSQL *conn;

    jo_clojure_mysql_t() {
        conn = mysql_init(NULL);
        if (conn == NULL) {
            warnf("ERROR: Could not create database connection\n");
        }
    }
};

typedef jo_alloc_t<jo_clojure_mysql_t> jo_clojure_mysql_alloc_t;
jo_clojure_mysql_alloc_t jo_clojure_mysql_alloc;
typedef jo_shared_ptr_t<jo_clojure_mysql_t> jo_clojure_mysql_ptr_t;
template<typename...A>
jo_clojure_mysql_ptr_t new_mysql(A...args) { return jo_clojure_mysql_ptr_t(jo_clojure_mysql_alloc.emplace(args...)); }
static node_idx_t new_node_mysql(jo_clojure_mysql_ptr_t nodes, int flags=0) { return new_node_object(NODE_MYSQL, nodes.cast<jo_object>(), flags); }

static node_idx_t native_mysql(env_ptr_t env, list_ptr_t args) {
    return new_node_mysql(new_mysql());
}

static node_idx_t native_mysql_connect(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_clojure_mysql_ptr_t mysql = get_node(*it++)->t_object.cast<jo_clojure_mysql_t>();
    if (mysql->conn == NULL) {
        warnf("ERROR: Could not create database connection\n");
        return FALSE_NODE;
    }

    jo_string host = get_node_string(*it++);
    jo_string user = get_node_string(*it++);
    jo_string password = get_node_string(*it++);
    jo_string database = get_node_string(*it++);
    unsigned int port = get_node_int(*it++);
    if (mysql_real_connect(mysql->conn, host.c_str(), user.c_str(), password.c_str(), database.c_str(), port, NULL, 0) == NULL) {
        warnf("ERROR: Could not connect to database\n");
        return FALSE_NODE;
    }
    return TRUE_NODE;
}

static node_idx_t native_mysql_select_db(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_clojure_mysql_ptr_t mysql = get_node(*it++)->t_object.cast<jo_clojure_mysql_t>();
    jo_string database = get_node_string(*it++);
    if (mysql_select_db(mysql->conn, database.c_str())) {
        warnf("ERROR: Could not select database\n");
        return FALSE_NODE;
    }
    return TRUE_NODE;
}

static node_idx_t native_mysql_query(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_clojure_mysql_ptr_t mysql = get_node(*it++)->t_object.cast<jo_clojure_mysql_t>();
    jo_string query = get_node_string(*it++);
    if (mysql_query(mysql->conn, query.c_str())) {
        warnf("ERROR: Could not execute query\n");
        return NIL_NODE;
    }

    MYSQL_RES *result = mysql_store_result(mysql->conn);
    if (result == NULL) {
        warnf("ERROR: Could not store result\n");
        return NIL_NODE;
    }

    int num_fields = mysql_num_fields(result);
    MYSQL_ROW row;
    MYSQL_FIELD *fields = mysql_fetch_fields(result);
    list_ptr_t list = new_list();
    while ((row = mysql_fetch_row(result))) {
        list_ptr_t row_list = new_list();
        for (int i = 0; i < num_fields; i++) {
            jo_string value = row[i] ? row[i] : "NULL";
            row_list->push_back_inplace(new_node_string(value));
        }
        list->push_back_inplace(new_node_list(row_list));
    }
    mysql_free_result(result);
    return new_node_list(list);
}

static node_idx_t native_mysql_close(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_clojure_mysql_ptr_t mysql = get_node(*it++)->t_object.cast<jo_clojure_mysql_t>();
    mysql_close(mysql->conn);
    return NIL_NODE;
}

static node_idx_t native_mysql_escape(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_clojure_mysql_ptr_t mysql = get_node(*it++)->t_object.cast<jo_clojure_mysql_t>();
    jo_string str = get_node_string(*it++);
    char *escaped = new char[str.length() * 2 + 1];
    mysql_real_escape_string(mysql->conn, escaped, str.c_str(), str.length());
    node_idx_t node = new_node_string(escaped);
    delete[] escaped;
    return node;
}

static node_idx_t native_mysql_reset(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    jo_clojure_mysql_ptr_t mysql = get_node(*it++)->t_object.cast<jo_clojure_mysql_t>();
    mysql_reset_connection(mysql->conn);
    return NIL_NODE;
}

void jo_clojure_mysql_init(env_ptr_t env) {
	env->set("mysql", new_node_native_function("mysql", &native_mysql, false, NODE_FLAG_PRERESOLVE));
	env->set("mysql/connect", new_node_native_function("mysql/connect", &native_mysql_connect, false, NODE_FLAG_PRERESOLVE));
	env->set("mysql/reset", new_node_native_function("mysql/reset", &native_mysql_reset, false, NODE_FLAG_PRERESOLVE));
	env->set("mysql/close", new_node_native_function("mysql/close", &native_mysql_close, false, NODE_FLAG_PRERESOLVE));
	env->set("mysql/select-db", new_node_native_function("mysql/select-db", &native_mysql_select_db, false, NODE_FLAG_PRERESOLVE));
	env->set("mysql/query", new_node_native_function("mysql/query", &native_mysql_query, false, NODE_FLAG_PRERESOLVE));
    env->set("mysql/escape", new_node_native_function("mysql/escape", &native_mysql_escape, false, NODE_FLAG_PRERESOLVE));
}


