#pragma once
#include <list>
#include <string>
#include <unordered_map>
#include <memory>
#include <iterator>
#include "PluginInterface.hpp"

/**
 * @class OrderedStringToPluginInterfaceMap
 * @brief A key-value store for plugin interfaces that maintains insertion order.
 */
class OrderedStringToPluginInterfaceMap {
public:
    using KeyValuePair = std::pair<const std::string&, std::shared_ptr<PluginInterface>&>;

    class OrderedIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = KeyValuePair;
        using difference_type = std::ptrdiff_t;
        using pointer = KeyValuePair*;
        using reference = KeyValuePair&;

        OrderedIterator(std::list<std::string>::iterator listIter, std::unordered_map<std::string, std::shared_ptr<PluginInterface>>& map)
            : listIter_(listIter), map_(map) {}

        OrderedIterator& operator++() {
            ++listIter_;
            return *this;
        }

        OrderedIterator operator++(int) {
            OrderedIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const OrderedIterator& other) const {
            return listIter_ == other.listIter_;
        }

        bool operator!=(const OrderedIterator& other) const {
            return !(*this == other);
        }

        KeyValuePair operator*() {
            return KeyValuePair(*listIter_, map_.at(*listIter_));
        }

    private:
        std::list<std::string>::iterator listIter_;
        std::unordered_map<std::string, std::shared_ptr<PluginInterface>>& map_;
    };

    OrderedIterator begin() { return OrderedIterator(order_.begin(), map_); }
    OrderedIterator end() { return OrderedIterator(order_.end(), map_); }

    /**
     * @brief Inserts a key-value pair into the map.
     *
     * If the key does not already exist, it is added while maintaining insertion order.
     * If the key already exists, its value is updated.
     *
     * @param key The key to insert.
     * @param value The value associated with the key.
     */
    void insert(const std::string& key, const std::shared_ptr<PluginInterface>& value) {
        if (map_.find(key) == map_.end()) {
            order_.push_back(key);  // Maintain insertion order
        }
        map_[key] = value;
    }

    /**
     * @brief Removes a key-value pair from the map.
     *
     * If the key exists, it is erased from both the unordered_map and the list that tracks insertion order.
     *
     * @param key The key to remove from the map.
     */
    void erase(const std::string& key) {
        if (map_.find(key) != map_.end()) {
            order_.remove(key);  // Remove key from the order list
            map_.erase(key);
        }
    }

    /**
     * @brief Clears the entire map.
     */
    void clear() {
        map_.clear();
        order_.clear();
    }

    /**
     * @brief Retrieves a reference to a value by key (with subscript access).
     *
     * If the key does not exist, a new entry is created with an empty string.
     *
     * @param key The key to access.
     * @return Reference to the value associated with the key.
     */
    std::shared_ptr<PluginInterface>& operator[](const std::string& key) {
        if (map_.find(key) == map_.end()) {
            order_.push_back(key); // Maintain insertion order
        }
        return map_[key]; // Inserts a default value if key doesn't exist
    }

    /**
     * @brief Retrieves a constant reference to a value by key.
     *
     * Throws an exception if the key does not exist.
     *
     * @param key The key to access.
     * @return Const reference to the value associated with the key.
     */
    const std::shared_ptr<PluginInterface>& at(const std::string& key) const {
        auto it = map_.find(key);
        if (it == map_.end()) {
            throw std::out_of_range("Key not found in OrderedMap");
        }
        return it->second;
    }

    bool empty() {
        return map_.empty();
    }

private:
    std::unordered_map<std::string, std::shared_ptr<PluginInterface>> map_;  // Stores key-value pairs for fast lookup.
    std::list<std::string> order_; // Maintains the order of insertion for execution.
};
