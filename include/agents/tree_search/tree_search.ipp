#pragma once

#include <vector>
#include <map>
#include <cassert>


namespace gym {
namespace ts {

template <typename S, typename A, typename SN, typename AN, typename DATA>
requires CompatibleNodes<S, A, SN, AN>
SN* tree_search<S, A, SN, AN, DATA>::select() {
    spdlog::trace("Selecting node");
    state_node_t* current_state_node = root.get();

    int depth = 0;

    while (!is_leaf<S, A, SN, AN>(current_state_node) && depth < max_depth && !current_state_node->is_terminal()) {
        A action = current_state_node->select_action(true);
        action_node_t *current_action_node = current_state_node->get_child(action);
        auto [s, r, p, t] = common_data->handler.sim_action(action);
        current_action_node->add_outcome(s, r, p, t);
        current_state_node->descend_update(action, s, true);
        current_state_node = current_action_node->get_child(s);
        depth++;
    }

    common_data->handler.sim_reset();
    return current_state_node;
}

template <typename S, typename A, typename SN, typename AN, typename DATA>
requires CompatibleNodes<S, A, SN, AN>
void tree_search<S, A, SN, AN, DATA>::propagate(state_node_t* leaf) {
    spdlog::trace("Propagatin gresults");
    action_node_t* prev_action_node = nullptr;
    state_node_t* current_state_node = leaf;

    while (!is_root<S, A, SN, AN>(current_state_node)) {
        current_state_node->propagate(prev_action_node, gamma);
        action_node_t* current_action_node = current_state_node->get_parent();
        current_action_node->propagate(current_state_node, gamma);
        current_state_node = current_action_node->get_parent();
        prev_action_node = current_action_node;
    }

    current_state_node->propagate(prev_action_node, gamma);
}

template <typename S, typename A, typename SN, typename AN, typename DATA>
requires CompatibleNodes<S, A, SN, AN>
void tree_search<S, A, SN, AN, DATA>::descent(A a, S s) {
    spdlog::trace("Descending tree");
    root->descend_update(a, s, false);
    action_node_t* action_node = root->get_child(a);
    std::unique_ptr<SN> new_root = action_node->get_child_unique_ptr(s);
    root = std::move(new_root);
    root->get_parent() = nullptr;
    for (auto& child : root->children) {
        child.parent = root.get();
    }
}

} // namespace ts
} // namespace gym