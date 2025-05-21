#ifndef OBSERVABLE_CONTAINER_H
#define OBSERVABLE_CONTAINER_H

#include <vector>
#include <functional>
#include <list>
#include <algorithm> // For std::remove for removeObserver (though std::function comparison is tricky)
#include "ChangeEvent.h"

template <typename T>
class ObservableContainer {
private:
    std::vector<T> data_;
    std::list<std::function<void(const ChangeEvent&)>> observers_;
    int defer_level_ = 0;
    bool batch_changed_ = false;

    void notify(ChangeType type) {
        // If type is BatchUpdate, it's coming from endUpdate and should not be deferred.
        if (type != ChangeType::BatchUpdate && defer_level_ > 0) {
            batch_changed_ = true;
            return;
        }

        ChangeEvent event{type};
        for (const auto& observer : observers_) {
            if (observer) { // Check if observer is callable
                observer(event);
            }
        }
    }

public:
    // Default constructor (implicit or explicit)
    ObservableContainer() = default;

    // Copy Constructor
    ObservableContainer(const ObservableContainer& other)
        : data_(other.data_),
          observers_(), // Observers are not copied
          defer_level_(0),
          batch_changed_(false) {
        // No notification on construction
    }

    // Copy Assignment Operator
    ObservableContainer& operator=(const ObservableContainer& other) {
        if (this == &other) {
            return *this;
        }

        bool data_changed = (data_ != other.data_); // Basic check, could be more sophisticated

        data_ = other.data_;
        observers_.clear(); // Clear existing observers
        defer_level_ = 0;
        batch_changed_ = false;

        if (data_changed) { // If the content actually changed.
            notify(ChangeType::BatchUpdate);
        }
        return *this;
    }

    // Move Constructor
    ObservableContainer(ObservableContainer&& other) noexcept
        : data_(std::move(other.data_)),
          observers_(), // Observers are not moved, initialize as empty
          defer_level_(other.defer_level_),
          batch_changed_(other.batch_changed_) {

        other.defer_level_ = 0;
        other.batch_changed_ = false;
        // other.data_ is already in a valid but unspecified state after std::move
        // other.observers_ remains empty as it was not moved.
    }

    // Move Assignment Operator
    ObservableContainer& operator=(ObservableContainer&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        data_ = std::move(other.data_);
        observers_.clear(); // Clear existing observers

        defer_level_ = other.defer_level_;
        batch_changed_ = other.batch_changed_;

        other.defer_level_ = 0;
        other.batch_changed_ = false;

        notify(ChangeType::BatchUpdate); // Notify a major change occurred

        return *this;
    }

    using ObserverCallback = std::function<void(const ChangeEvent&)>;

    void beginUpdate() {
        defer_level_++;
    }

    void endUpdate() {
        if (defer_level_ > 0) { // Ensure defer_level_ doesn't go below zero
            defer_level_--;
            if (defer_level_ == 0 && batch_changed_) {
                notify(ChangeType::BatchUpdate); // This notification should not be deferred
                batch_changed_ = false;
            }
        }
    }

    void addObserver(const ObserverCallback& observer) {
        observers_.push_back(observer);
    }

    void removeObserver(const ObserverCallback& observer) {
        observers_.remove_if([&observer](const ObserverCallback& obs_func) {
            return observer.target_type() == obs_func.target_type() &&
                   observer.template target<void(const ChangeEvent&)>() == obs_func.template target<void(const ChangeEvent&)>();
        });
    }

    size_t size() const noexcept {
        return data_.size();
    }

    bool empty() const noexcept {
        return data_.empty();
    }

    void push_back(const T& value) {
        data_.push_back(value);
        notify(ChangeType::ElementAdded);
        notify(ChangeType::SizeChanged);
    }

    void pop_back() {
        if (!data_.empty()) {
            data_.pop_back();
            notify(ChangeType::ElementRemoved);
            notify(ChangeType::SizeChanged);
        }
    }

    T& operator[](size_t index) {
        return data_[index];
    }

    const T& operator[](size_t index) const {
        return data_[index];
    }

    typename std::vector<T>::iterator begin() noexcept {
        return data_.begin();
    }

    typename std::vector<T>::const_iterator begin() const noexcept {
        return data_.begin();
    }

    typename std::vector<T>::iterator end() noexcept {
        return data_.end();
    }

    typename std::vector<T>::const_iterator end() const noexcept {
        return data_.end();
    }

    typename std::vector<T>::const_iterator cbegin() const noexcept {
        return data_.cbegin();
    }

    typename std::vector<T>::const_iterator cend() const noexcept {
        return data_.cend();
    }

    void clear() {
        bool was_empty = data_.empty();
        data_.clear();
        if (!was_empty) {
            notify(ChangeType::SizeChanged);
        }
    }

    typename std::vector<T>::iterator insert(typename std::vector<T>::const_iterator pos, const T& value) {
        auto dist = std::distance(data_.cbegin(), pos);
        typename std::vector<T>::iterator it_pos = data_.begin() + dist;
        typename std::vector<T>::iterator result_it = data_.insert(it_pos, value);
        notify(ChangeType::ElementAdded);
        notify(ChangeType::SizeChanged);
        return result_it;
    }

    typename std::vector<T>::iterator erase(typename std::vector<T>::const_iterator pos) {
        auto dist = std::distance(data_.cbegin(), pos);
        typename std::vector<T>::iterator it_pos = data_.begin() + dist;
        if (it_pos == data_.end()) {
            return data_.end();
        }
        typename std::vector<T>::iterator result_it = data_.erase(it_pos);
        notify(ChangeType::ElementRemoved);
        notify(ChangeType::SizeChanged);
        return result_it;
    }

    void modify(size_t index, const T& newValue) {
        if (index < data_.size()) {
            data_[index] = newValue;
            notify(ChangeType::ElementModified);
        }
    }

    void modify(size_t index, T&& newValue) {
        if (index < data_.size()) {
            data_[index] = std::move(newValue);
            notify(ChangeType::ElementModified);
        }
    }
};

#endif // OBSERVABLE_CONTAINER_H
