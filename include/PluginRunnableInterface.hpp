#pragma once
#include <string>
#include <vector>
#include "PluginInterface.hpp"
#include "PluginRegistry.hpp"
#include "nlohmann/json.hpp"

/**
 * @brief Abstract interface for plugins that can be executed.
 *
 * PluginRunnableInterface extends PluginInterface by adding an execute()
 * method which operates on an EmailList. Derived classes must implement
 * execute() to perform their runnable behavior.
 */
class PluginRunnableInterface : public PluginInterface {
public:
    /**
     * @brief Pure virtual destructor.
     *
     * Ensures that PluginRunnableInterface is abstract and derived classes are
     * properly destructed.
     */
    ~PluginRunnableInterface() override = default;
protected:
    /**
     * @brief Protected default constructor.
     *
     * Only derived classes can instantiate PluginRunnableInterface.
     */
    explicit PluginRunnableInterface(const std::string& name) : PluginInterface(name) {};
};