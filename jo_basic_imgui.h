#pragma once

static node_idx_t native_imgui_main_menu_bar(env_ptr_t env, list_ptr_t args) {
    if(ImGui::BeginMainMenuBar()) {
        node_idx_t ret = eval_node_list(env, args);
        ImGui::EndMainMenuBar();
        return ret;
    }
    return NIL_NODE;
}

static node_idx_t native_imgui_menu_bar(env_ptr_t env, list_ptr_t args) {
    if(ImGui::BeginMenuBar()) {
        node_idx_t ret = eval_node_list(env, args);
        ImGui::EndMenuBar();
        return ret;
    }
    return NIL_NODE;
}

static node_idx_t native_imgui_menu(env_ptr_t env, list_ptr_t args) {
    if(ImGui::BeginMenu(get_node_string(args->first_value()).c_str())) {
        node_idx_t ret = eval_node_list(env, args->pop_front());
        ImGui::EndMenu();
        return ret;
    }
    return NIL_NODE;
}

static node_idx_t native_imgui_menu_item(env_ptr_t env, list_ptr_t args) {
    if(ImGui::MenuItem(get_node_string(args->first_value()).c_str())) {
        return eval_node_list(env, args->pop_front());
    }
    return NIL_NODE;
}

void jo_basic_imgui_init(env_ptr_t env) {
	env->set("imgui/main-menu-bar", new_node_native_function("imgui/main-menu-bar", &native_imgui_main_menu_bar, true, NODE_FLAG_PRERESOLVE));
	env->set("imgui/menu-bar", new_node_native_function("imgui/menu-bar", &native_imgui_menu_bar, true, NODE_FLAG_PRERESOLVE));
	env->set("imgui/menu", new_node_native_function("imgui/menu", &native_imgui_menu, true, NODE_FLAG_PRERESOLVE));
	env->set("imgui/menu-item", new_node_native_function("imgui/menu-item", &native_imgui_menu_item, true, NODE_FLAG_PRERESOLVE));
}

