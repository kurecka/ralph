namespace gym {
namespace ts {

/***************************************************
 * UCT implementation
 * *************************************************/

template <typename S, typename A>
void dual_uct<S, A>::play() {
    uct_state<S, A, data_t, mode>* root = ts.get_root();

    spdlog::debug("Running simulations");
    for (int i = 0; i < num_sim; i++) {
        spdlog::trace("Simulation " + std::to_string(i));
        uct_state<S, A, data_t, mode>* leaf = ts.select();
        leaf->expand(&common_data);
        ts.propagate(leaf);

        A a = root->select_action(false);
        uct_action<S, A, data_t, mode>* action_node = root->get_child(a);

        common_data.lambda += lr * (action_node->expected_penalty - common_data.risk_thd);
        if (common_data.lambda < 0) {
            common_data.lambda = 0;
        }
    }

    A a = root->select_action(false);

    spdlog::trace("Play action: " + std::to_string(a));
    auto [s, r, p, e] = agent<S, A>::handler.play_action(a);
    spdlog::trace("  Result: s=" + std::to_string(s) + ", r=" + std::to_string(r) + ", p=" + std::to_string(p));
    
    root->get_child(a)->add_outcome(s, r, p, e);
    root->descend_update(a, s, false);
    ts.descent(a, s);
}

template <typename S, typename A>
void dual_uct<S, A>::reset() {
    agent<S, A>::reset();
    ts.reset();
    common_data.lambda = 0;
    common_data.risk_thd = risk_thd;
}

} // namespace ts
} // namespace gym