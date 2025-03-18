#include "EmailStorage.hpp"
#include "EmailListView.hpp"

bool EmailStorage::refresh_full_view_size_(size_t *s, size_t *e) {
    s = 0;
    *e = emails_.size();
    return 1;
}

EmailListView EmailStorage::getFullView() {
    std::shared_lock lock(storageMutex_);
    return EmailListView(this, 0, emails_.size());
}

std::vector<EmailListView> EmailStorage::split(int numParts) {
    std::shared_lock lock(storageMutex_);
    return getFullView().split(numParts);
}

void EmailStorage::commitPendingInserts() {
    std::unique_lock lock(storageMutex_);
    while (!pendingInserts_.empty()) {
        emails_.push_back(pendingInserts_.front());
        pendingInserts_.pop();
    }
}

nlohmann::json EmailStorage::getSimpleEmailJsonList() {
    std::shared_lock lock(storageMutex_);
    nlohmann::json jsonEmails = nlohmann::json::array();
    try {
        for (auto& email : emails_) {
            jsonEmails.push_back(email.toJson());
        }
    } catch (std::exception &e) {
        LOG_ERROR << "Issue";
    }
    return jsonEmails;
}

nlohmann::json EmailStorage::getEmailsByNumber(int start, int num_returned) {
    std::shared_lock lock(storageMutex_);
    nlohmann::json jsonEmails = nlohmann::json::array();

    if (start < 0 || start >= static_cast<int>(emails_.size())) {
        return jsonEmails;
    }

    int end = std::min(start + num_returned, static_cast<int>(emails_.size()));

    try {
        for (int i = start; i < end; ++i) {
            jsonEmails.push_back(emails_[i].toJson());
        }
    } catch (const std::exception &e) {
        LOG_ERROR << "Error fetching emails: " << e.what();
    }

    return jsonEmails;
}