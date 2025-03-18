#pragma once

#include <vector>
#include <string>
#include <shared_mutex>
#include <queue>
#include "Email.hpp"
#include "nlohmann/json.hpp"

class EmailListView;

class EmailStorage {
private:
    mutable std::shared_mutex storageMutex_;
    std::vector<Email> emails_;
    std::queue<Email> pendingInserts_;
    bool refresh_full_view_size_(size_t *s, size_t *e);

friend class EmailListView;

public:
    EmailStorage() = default;

    // Insert Email (Thread-Safe)
    void insertEmail(const Email& email) {
        std::unique_lock lock(storageMutex_);
        emails_.push_back(email);
    }

    // Get a full view (Read-Only)
    EmailListView getFullView();

    // Splits storage into numParts partitions
    std::vector<EmailListView> split(int numParts);

    // Commits queued insertions to storage
    void commitPendingInserts();

    nlohmann::json getSimpleEmailJsonList();

    nlohmann::json getEmailsByNumber(int start, int num_returned);

    // Removes Email (Thread-Safe)
    void removeEmail(const Email& email) {
        std::unique_lock lock(storageMutex_);
        emails_.erase(std::remove(emails_.begin(), emails_.end(), email), emails_.end());
    }

    // Get size (Thread-Safe)
     int getSize() const {
        std::shared_lock lock(storageMutex_);
        return emails_.size();
    }
};