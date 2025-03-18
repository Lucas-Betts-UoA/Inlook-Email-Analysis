#include "PluginStateManager.hpp"

// Constructor implementation
PluginStateManager::PluginStateManager(bool addDefaultTransitions) : currentState("UNLOADED") {
    addTransition("UNLOADED", "FAILED"); // Only viable transitions at the start.
    addTransition("FAILED", "UNLOADED");

    if (addDefaultTransitions) { // Other states used by typical plugins.
        addTransition("UNLOADED", "LOADED");
        addTransition("LOADED", "READY");
        addTransition("READY", "RUNNING");
        addTransition("RUNNING", "COMPLETE");
    }
}

// Allows adding additional custom transitions.
void PluginStateManager::addTransition(const std::string &from, const std::string &to) {
    states.insert({from, to});         // Ensure states are tracked.
    transitions[from].insert("FAILED"); // Force transitions to FAILED from state.
    transitions[to].insert("FAILED");
    transitions[from].insert(to);       // Finally, insert the requested transition.
}

// Returns all states tracked in the state manager
std::set<std::string> PluginStateManager::getStates() const {
    return states;
}

// Gets viable transitions from the current state.
std::set<std::string> PluginStateManager::getTransitions() const {
    auto it = transitions.find(currentState);
    if (it != transitions.end()) {
        return it->second;
    }
    return {};
}

// Gets viable transitions from a given state.
std::set<std::string> PluginStateManager::getTransitions(const std::string& state) const {
    auto it = transitions.find(state);
    if (it != transitions.end()) {
        return it->second;
    }
    return {};
}

// Removes a transition between two states.
bool PluginStateManager::removeTransition(const std::string &from, const std::string &to) {
    auto fromIt = transitions.find(from);
    if (fromIt == transitions.end()) {
        return false;
    }

    fromIt->second.erase(to);

    if (fromIt->second.empty()) {
        transitions.erase(fromIt);
        states.erase(from);
    }

    bool toFound = false;
    for (const auto &pair : transitions) {
        if (pair.second.find(to) != pair.second.end()) {
            toFound = true;
            break;
        }
    }
    if (!toFound) {
        states.erase(to);
    }

    return true;
}

// Transition function that checks if a transition is allowed
bool PluginStateManager::transitionTo(const std::string& newState, const std::string& callerName, int line) {
    if (currentState == newState) {
        return true;
    }

    auto it = transitions.find(currentState);
    if (it != transitions.end() && it->second.contains(newState)) {
        //LOG_DEBUG_VERBOSE << "PluginManager for " << callerName << ":" << line
        //                  << " transitioning state from " << currentState << " to " << newState << ".";
        currentState = newState;
        return true;
    }

    LOG_ERROR << "PluginManager for " << callerName << ":" << line
              << " invalid state transition from " << currentState << " to " << newState;
    return false;
}

// Returns the current state.
std::string PluginStateManager::getState() const {
    return currentState;
}