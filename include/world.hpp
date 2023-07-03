#pragma once

#include <vector>
#include <tuple>
#include <memory>

#include "rand.hpp"
#include "spdlog/spdlog.h"

namespace world {

template <typename S>
using outcome_t = std::tuple<S, float, float, bool>; // state, reward, penalty, is_over

/*************************************************************************
 * ENVIRONTMENT INTERFACE
 *************************************************************************/

/**
 * @brief Abstract environment class
 * 
 */
template <typename S, typename A>
class environment {
public:
    virtual ~environment() = default;

    virtual std::string name() const = 0;

    /**
     * @brief Get the number of possible actions
     */
    virtual size_t num_actions() const = 0;

    /**
     * @brief Get the vector of all possible actions
     */
    virtual std::vector<A> possible_actions() const = 0;

    /**
     * @brief Get the ith action
     * 
     * @param i index of the action
     */
    virtual A get_action(size_t i) const = 0;

    /**
     * @brief Get current state of the environment
     * 
     * @return S 
     */
    virtual S current_state() const = 0;

    /**
     * @brief Return boolean indicating if the environment is over
     * 
     * @return bool
     */
    virtual bool is_over() const = 0;

    /**
     * @brief Play an action in the environment
     * 
     * Returns the next state, the reward and penalty, and a boolean indicating whether the episode is over
     * @return outcome_t
     */
    virtual outcome_t<S> play_action(A action) = 0;

    /**
     * @brief Make a checkpoint of the environment
     * 
     */
    virtual void make_checkpoint() = 0;

    /**
     * @brief Restore the environment to the last checkpoint
     * 
     */
    virtual void restore_checkpoint() = 0;

    /**
     * @brief Reset the environment
     * 
     */
    virtual void reset() = 0;
};


/*************************************************************************
 * ENVIRONMENT HANDLER
 *************************************************************************/
template<typename S, typename A>
class environment_handler {
private:
    environment<S, A>* env;
    bool is_simulating = false;

    float reward;
    float penalty;
    int num_steps;
public:
    // environment_handler(environment<S, A>* env) : env(env) {}
    environment_handler() = default;
    environment_handler(environment<S, A>& _env) : env(&_env) {}
    environment_handler(const environment_handler&) = default;

    ~environment_handler() = default;

    environment_handler& operator=(const environment_handler&) = default;

    float get_reward() const {
        return reward;
    }

    float get_penalty() const {
        return penalty;
    }

    int get_num_steps() const {
        return num_steps;
    }

    void reset() {
        spdlog::debug("Resetting handler");
        reward = penalty = num_steps = 0;
    }

    /**
     * @brief Play an action in the environment
     * 
     * Returns the next state, the reward and penalty, and a boolean indicating whether the episode is over
     * @return outcome_t
     */
    outcome_t<S> play_action(A action) {
        if (is_simulating) {
            env->restore_checkpoint();
            is_simulating = false;
        }
        outcome_t<S> o = env->play_action(action);
        float r = std::get<1>(o);
        float p = std::get<2>(o);
        ++num_steps;
        reward += r;
        penalty += p; 

        return o;
    }

    /**
     * @brief Simulate an action in the environment
     * 
     * Returns the next state, the reward and penalty, and a boolean indicating whether the episode is over
     * @return outcome_t
     */
    outcome_t<S> sim_action(A action) {
        if (!is_simulating) {
            env->make_checkpoint();
            is_simulating = true;
        }
        return env->play_action(action);
    }

    /**
     * @brief Restore the environment to the last checkpoint
     * 
     */
    void sim_reset() {
        if (is_simulating) {
            env->restore_checkpoint();
            is_simulating = false;
        }
    }

    /**
     * @brief Get the number of possible actions
     * 
     * @return size_t
     */
    size_t num_actions() const {
        return env->num_actions();
    }

    std::vector<A> possible_actions() const {
        return env->possible_actions();
    }

    A get_action(size_t i) const {
        return env->get_action(i);
    }

    /**
     * @brief Get current state of the environment
     * 
     * @return S 
     */
    S current_state() const {
        return env->current_state();
    }

    operator bool() const
    {
        return env;
    }
};


/*************************************************************************
 * AGENT INTERFACE
 *************************************************************************/
template<typename S, typename A>
class orchestrator;

template <typename S, typename A>
class agent {
protected:
    environment_handler<S, A> handler;
public:
    /**
     * @brief Set the handler object
     * 
     * @param _handler the handler that controls the environment
     */
    void set_handler(environment_handler<S, A> _handler) {
        spdlog::info("Setting agent handler");
        handler = _handler;
    }

    /**
     * @brief Set the handler object
     * 
     * @param _env environment that is xontrolled by the handler
     */
    void set_handler(environment<S, A>& _env) {
        spdlog::info("Setting agent handler");
        handler = environment_handler<S, A>(_env);
        _env.reset();
    }

    const environment_handler<S, A>& get_handler() const {
        return handler;
    }

    /**
     * @brief Construct a new agent object 
     * 
     */
    agent() {}

    /**
     * @brief Destroy the agent object
     * 
     */
    virtual ~agent() = default;

    /**
     * @brief Reset the agent
     * 
     */
    virtual void reset() {
        handler.reset();
    }

    /**
     * @brief Play through the environment handler
     *
     */
    virtual void play() = 0;

    /**
     * @brief Train the agent after the episode is over
     * 
     */
    virtual void train() {}

    /**
     * @brief Return boolean indicating if the agent is trainable
     * 
     */
    constexpr virtual bool is_trainable() const {
        // TODO: make this into a trait?
        return false;
    }

    /**
     * @brief Get the name of the agent
     * 
     * @return std::string 
     */
    virtual std::string name() const = 0;
};

} // namespace world
