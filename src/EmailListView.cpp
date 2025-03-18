#include "EmailListView.hpp"
#include "EmailStorage.hpp"
#include <iterator>
#include <shared_mutex>

// Constructor
/**
 *
 * @param storage
 * @param s
 * @param e
 */
EmailListView::EmailListView(EmailStorage* storage, size_t startIndex, size_t endIndex)
    : storage_(storage),
    startIndex_(startIndex),
    endIndex_(endIndex) {}

EmailListView::~EmailListView() {
    commitInserts();
}

std::vector<Email>::iterator EmailListView::begin() { return storage_->emails_.begin() + startIndex_; }

std::vector<Email>::iterator EmailListView::end() { return storage_->emails_.begin() + endIndex_; }

size_t EmailListView::getSize() const {
    return (endIndex_ > startIndex_) ? (endIndex_ - startIndex_) : 0;
}

std::vector<EmailListView> EmailListView::split(int numParts) {
    size_t totalSize = getSize();
    if (numParts <= 0 || totalSize == 0) return {};

    std::vector<EmailListView> segments;
    size_t segmentSize = totalSize / numParts;
    size_t remainder = totalSize % numParts;

    size_t segmentStart = startIndex_;
    for (int i = 0; i < numParts; ++i) {
        size_t segmentEnd = segmentStart + segmentSize + (i < remainder ? 1 : 0);
        segments.emplace_back(EmailListView(storage_, segmentStart, segmentEnd));
        segmentStart = segmentEnd;
    }

    return segments;
}

void EmailListView::insertEmail(const Email& email) {
    insertQueue_.push(email);
}

void EmailListView::commitInserts() {
    while (!insertQueue_.empty()) {
        storage_->insertEmail(insertQueue_.front());
        insertQueue_.pop();
    }
    storage_->refresh_full_view_size_(&startIndex_, &endIndex_);
}

nlohmann::json EmailListView::getSimpleEmailJsonList() {
    nlohmann::json jsonEmails = nlohmann::json::array();

    if (!storage_) return jsonEmails;

    for (size_t i = startIndex_; i < endIndex_ && i < storage_->emails_.size(); ++i) {
        jsonEmails.push_back(storage_->emails_[i].toJson());
    }

    return jsonEmails;
}