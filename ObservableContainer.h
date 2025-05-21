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

        // Determine if data is likely to change.
        // This check is tricky before the actual move.
        // For simplicity, we'll notify if this != &other, assuming a change.
        // A more complex check would involve comparing data_ before and other.data_
        // or checking if other is empty.
        
        data_ = std::move(other.data_);
        observers_.clear(); // Clear existing observers
        
        defer_level_ = other.defer_level_;
        batch_changed_ = other.batch_changed_;

        other.defer_level_ = 0;
        other.batch_changed_ = false;
        // other.data_ is in a valid but unspecified state
        // other.observers_ remains empty

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
        // Comparing std::function objects for equality is not straightforward.
        // std::function doesn't have a reliable operator==.
        // The target_type() and target() methods can be used for comparison if the function wraps a free function or a lambda with no captures.
        // For member functions or lambdas with captures, this is more complex.
        // A common robust solution involves returning a handle/ID from addObserver.
        // For this exercise, we'll iterate and try a best-effort removal,
        // knowing it might not be perfectly robust for all std::function types.
        // This naive approach will likely only work if the exact same std::function object is passed.
        observers_.remove_if([&observer](const ObserverCallback& obs_func) {
            // This comparison is generally not guaranteed to work correctly for all std::function objects.
            // It relies on the std::function::target_type and potentially std::function::target,
            // or if the compiler/library implements some form of comparison.
            // A simple `obs_func == observer` is not defined.
            // For a more robust solution, one might compare the targets if available and types match.
            // However, for this specific problem, we'll use the list::remove method which uses operator==.
            // Since std::function does not have a global operator==, we need remove_if.
            // The comparison here is tricky; we're comparing the raw function pointers if possible.
            // This is a known limitation of std::function.
            return observer.target_type() == obs_func.target_type() && 
                   observer.template target<void(const ChangeEvent&)>() == obs_func.template target<void(const ChangeEvent&)>();
        });
        // A simpler, but potentially less effective way for some cases, if target comparison is too complex or not working:
        // observers_.remove(observer); // This relies on `std::function::operator==` which is not provided by the standard.
        // So, the remove_if with target comparison is a more explicit attempt, but still has limitations.
        // For the purpose of this exercise, we'll proceed with remove_if and target comparison.
        // If this proves problematic, an alternative is to require users to store and pass the exact iterator or a token.
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
            // T value = data_.back(); // Capture if needed for a more detailed event
            data_.pop_back();
            notify(ChangeType::ElementRemoved);
            notify(ChangeType::SizeChanged);
        }
    }

    T& operator[](size_t index) {
        // As per instructions, direct modification via operator[] won't automatically notify ElementModified.
        // A proxy object or a specific modify method would be needed for that.
        return data_[index];
    }

    const T& operator[](size_t index) const {
        return data_[index];
    }

    // Iterator methods
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

    // Clear method
    void clear() {
        bool was_empty = data_.empty();
        data_.clear();
        if (!was_empty) {
            notify(ChangeType::SizeChanged);
            // Optionally, one could also send a general ElementRemoved signal here,
            // or have a specific ContainerCleared ChangeType.
            // For now, SizeChanged is the primary notification.
        }
    }

    // Insert method
    typename std::vector<T>::iterator insert(typename std::vector<T>::const_iterator pos, const T& value) {
        // Need to convert const_iterator to iterator for insert.
        // This can be done by calculating the distance from the beginning.
        auto dist = std::distance(data_.cbegin(), pos);
        typename std::vector<T>::iterator it_pos = data_.begin() + dist;
        
        typename std::vector<T>::iterator result_it = data_.insert(it_pos, value);
        notify(ChangeType::ElementAdded);
        notify(ChangeType::SizeChanged);
        return result_it;
    }

    // Erase method
    typename std::vector<T>::iterator erase(typename std::vector<T>::const_iterator pos) {
        // Need to convert const_iterator to iterator for erase.
        auto dist = std::distance(data_.cbegin(), pos);
        typename std::vector<T>::iterator it_pos = data_.begin() + dist;

        if (it_pos == data_.end()) { // Cannot erase end iterator
            return data_.end();
        }
        
        typename std::vector<T>::iterator result_it = data_.erase(it_pos);
        notify(ChangeType::ElementRemoved);
        notify(ChangeType::SizeChanged);
        return result_it;
    }

    // Modify methods
    void modify(size_t index, const T& newValue) {
        if (index < data_.size()) {
            data_[index] = newValue;
            notify(ChangeType::ElementModified);
        }
        // Consider throwing an out_of_range exception if index is invalid
    }

    void modify(size_t index, T&& newValue) {
        if (index < data_.size()) {
            data_[index] = std::move(newValue);
            notify(ChangeType::ElementModified);
        }
        // Consider throwing an out_of_range exception if index is invalid
    }
};

#endif // OBSERVABLE_CONTAINER_H
