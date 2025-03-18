#pragma once

#include <vector>
#include <iterator>
#include <shared_mutex>
#include "Email.hpp"

class EmailStorage;

class EmailListView {
private:
    EmailStorage* storage_;
    size_t startIndex_;
    size_t endIndex_;
    std::queue<Email> insertQueue_;

public:
    EmailListView(EmailStorage *storage, size_t startIndex, size_t endIndex);

    ~EmailListView();

    std::vector<Email>::iterator begin();
    std::vector<Email>::iterator end();

    // Splits this view into sub-views
    std::vector<EmailListView> split(int numParts);

    // Queue an Email for insertion
    void insertEmail(const Email& email);

    // Commit pending inserts to storage
    void commitInserts();

    // Get the size of this view
    size_t getSize() const;

    // Convert to JSON
    nlohmann::json getSimpleEmailJsonList();
};