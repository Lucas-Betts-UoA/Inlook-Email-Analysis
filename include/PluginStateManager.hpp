#pragma once

#include <set>
#include <unordered_map>
#include <string>
#include "Logger.hpp"

class PluginStateManager {
public:
    explicit PluginStateManager(bool addDefaultTransitions = true);

    void addTransition(const std::string &from, const std::string &to);
    std::set<std::string> getStates() const;
    std::set<std::string> getTransitions() const;
    std::set<std::string> getTransitions(const std::string& state) const;
    bool removeTransition(const std::string &from, const std::string &to);
    bool transitionTo(const std::string& newState, const std::string& callerName, int line);
    std::string getState() const;

private:
    std::string currentState;
    std::set<std::string> states;
    std::unordered_map<std::string, std::set<std::string>> transitions;
};