#ifndef OBSERVABLE_CONTAINER_H
#define OBSERVABLE_CONTAINER_H

#include <vector>
#include <list> // Make sure std::list is included
#include <functional>
#include <algorithm> // For std::remove_if, std::advance
#include <mutex>     // Required for std::mutex, std::lock_guard
#include <utility>   // Required for std::pair
#include <cstdint>   // Required for uint64_t
#include <optional>  // Already in ChangeEvent.h, but good for explicitness
#include <iterator>  // Required for std::advance, std::distance
#include <stdexcept> // Required for std::out_of_range
#include "ChangeEvent.h"

// Forward declaration
template <
    typename T,
    template <typename, typename> class ActualContainer, 
    typename Allocator
>
class ObservableContainer;

// Helper for container access to abstract away operator[] vs std::advance
namespace ObservableContainerHelpers {
    template <typename ContainerType, typename ValueType, class Enable = void>
    struct ContainerAccess;

    // Specialization for containers supporting operator[] (like std::vector, std::deque)
    // SFINAE check for operator[]
    template <typename ContainerType, typename ValueType>
    struct ContainerAccess<ContainerType, ValueType,
        typename std::enable_if<
            std::is_same<decltype(std::declval<ContainerType>()[0]), ValueType&>::value &&
            std::is_same<decltype(std::declval<const ContainerType>()[0]), const ValueType&>::value
        >::type
    > {
        static ValueType& get(ContainerType& c, size_t index) {
            if (index >= c.size()) throw std::out_of_range("Index out of range");
            return c[index];
        }
        static const ValueType& get(const ContainerType& c, size_t index) {
            if (index >= c.size()) throw std::out_of_range("Index out of range");
            return c[index];
        }
        static void set(ContainerType& c, size_t index, const ValueType& value) {
            if (index >= c.size()) throw std::out_of_range("Index out of range");
            c[index] = value;
        }
        static void set(ContainerType& c, size_t index, ValueType&& value) {
            if (index >= c.size()) throw std::out_of_range("Index out of range");
            c[index] = std::move(value);
        }
    };

    // Specialization for std::list
    template <typename ValueType, typename Alloc>
    struct ContainerAccess<std::list<ValueType, Alloc>, ValueType> {
        static ValueType& get(std::list<ValueType, Alloc>& c, size_t index) {
            if (index >= c.size()) throw std::out_of_range("Index out of range for list access");
            auto it = c.begin();
            std::advance(it, index);
            return *it;
        }
        static const ValueType& get(const std::list<ValueType, Alloc>& c, size_t index) {
            if (index >= c.size()) throw std::out_of_range("Index out of range for list access");
            auto it = c.cbegin();
            std::advance(it, index);
            return *it;
        }
        static void set(std::list<ValueType, Alloc>& c, size_t index, const ValueType& value) {
            if (index >= c.size()) throw std::out_of_range("Index out of range for list access");
            auto it = c.begin();
            std::advance(it, index);
            *it = value;
        }
        static void set(std::list<ValueType, Alloc>& c, size_t index, ValueType&& value) {
            if (index >= c.size()) throw std::out_of_range("Index out of range for list access");
            auto it = c.begin();
            std::advance(it, index);
            *it = std::move(value);
        }
    };
} // namespace ObservableContainerHelpers

template <
    typename T,
    template <typename, typename> class ActualContainer = std::vector, // Default here
    typename Allocator = std::allocator<T>
>
class ObservableContainer {
public: // Public type aliases
    using ObserverCallback = std::function<void(const ChangeEvent<T>&)>;
    using ObserverHandle = uint64_t;

private:
    ActualContainer<T, Allocator> data_; // Use the templated container type
    std::list<std::pair<ObserverHandle, ObserverCallback>> observers_;
    int defer_level_ = 0;
    bool batch_changed_ = false;
    mutable std::mutex mutex_; // Mutex for thread safety
    inline static ObserverHandle nextHandleId_ = 0; 
    bool is_moved_from_ = false;

    void notify(ChangeType type,
                std::optional<size_t> index = std::nullopt,
                std::optional<T> oldValue = std::nullopt,
                std::optional<T> newValue = std::nullopt) {
        if (is_moved_from_) {
            return; // Moved-from object should not send notifications
        }
        std::list<ObserverCallback> observers_to_call_functions;
        bool should_call_observers = false;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (type != ChangeType::BatchUpdate && defer_level_ > 0) {
                batch_changed_ = true;
            } else {
                for (const auto& pair : observers_) {
                    observers_to_call_functions.push_back(pair.second);
                }
                should_call_observers = true;
            }
        }

        if (should_call_observers && !observers_to_call_functions.empty()) {
            ChangeEvent<T> event{type, index, oldValue, newValue};
            for (const auto& observer_func : observers_to_call_functions) {
                if (observer_func) {
                    observer_func(event);
                }
            }
        }
    }

public:
    ObservableContainer() = default;

    // Copy Constructor
    ObservableContainer(const ObservableContainer& other)
        : observers_(), 
          defer_level_(0),      
          batch_changed_(false) 
    {
        std::lock_guard<std::mutex> lock(other.mutex_); 
        data_ = other.data_; 
    }

    // Copy Assignment Operator
    ObservableContainer& operator=(const ObservableContainer& other) {
        if (this == &other) {
            return *this;
        }
        bool data_actually_changed = false;
        { 
            std::scoped_lock lock(mutex_, other.mutex_);
            // Assuming ActualContainer supports operator!= for comparison
            if (data_ != other.data_) { 
                data_ = other.data_;
                data_actually_changed = true;
            }
            observers_.clear(); 
            defer_level_ = 0;   
            batch_changed_ = false; 
        } 
        if (data_actually_changed) {
            notify(ChangeType::BatchUpdate);
        }
        return *this;
    }

    // Move Constructor
    ObservableContainer(ObservableContainer&& other) noexcept
    {
        std::lock_guard<std::mutex> lock(other.mutex_); 
        data_ = std::move(other.data_);
        // observers_ list is default-initialized (empty)
        defer_level_ = other.defer_level_;
        batch_changed_ = other.batch_changed_;
        other.data_.clear(); 
        other.observers_.clear(); 
        other.defer_level_ = 0;
        other.batch_changed_ = false;
        other.is_moved_from_ = true;
    }

    // Move Assignment Operator
    ObservableContainer& operator=(ObservableContainer&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        { 
            std::scoped_lock lock(mutex_, other.mutex_);
            data_ = std::move(other.data_);
            // observers_ are intentionally not cleared for 'this'
            defer_level_ = other.defer_level_; // Or reset, depending on desired state for 'this'
            batch_changed_ = other.batch_changed_; // Or reset
            
            other.data_.clear(); 
            other.observers_.clear();
            other.defer_level_ = 0;
            other.batch_changed_ = false;
            other.is_moved_from_ = true;
        } 
        notify(ChangeType::BatchUpdate);
        return *this;
    }

    // ObserverCallback is already public

    void beginUpdate() {
        std::lock_guard<std::mutex> lock(mutex_);
        defer_level_++;
    }

    void endUpdate() {
        bool should_notify_batch_update = false;
        { 
            std::lock_guard<std::mutex> lock(mutex_);
            if (defer_level_ > 0) { 
                defer_level_--;
                if (defer_level_ == 0 && batch_changed_) {
                    should_notify_batch_update = true;
                    batch_changed_ = false; 
                }
            }
        } 
        if (should_notify_batch_update) {
            notify(ChangeType::BatchUpdate);
        }
    }

    ObserverHandle addObserver(const ObserverCallback& observer) {
        std::lock_guard<std::mutex> lock(mutex_);
        ObserverHandle handle = ++nextHandleId_;
        observers_.push_back({handle, observer});
        return handle;
    }

    bool removeObserver(ObserverHandle handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto original_size = observers_.size();
        observers_.remove_if([handle](const auto& pair) {
            return pair.first == handle;
        });
        return observers_.size() < original_size;
    }

    size_t size() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.size();
    }

    bool empty() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.empty();
    }

    // Generic operations
    void push_back(const T& value) {
        size_t pushed_at_index;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            data_.push_back(value);
            pushed_at_index = data_.size() - 1; 
        }
        notify(ChangeType::ElementAdded, pushed_at_index, std::nullopt, value);
        notify(ChangeType::SizeChanged);
    }

    void pop_back() {
        bool modified = false;
        T old_value;
        size_t original_size = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!data_.empty()) {
                original_size = data_.size();
                old_value = data_.back(); 
                data_.pop_back();
                modified = true;
            }
        }
        if (modified) {
            notify(ChangeType::ElementRemoved, original_size - 1, old_value, std::nullopt);
            notify(ChangeType::SizeChanged);
        }
    }
    
    T& front() {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.front();
    }

    const T& front() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.front();
    }

    T& back() {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.back();
    }

    const T& back() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.back();
    }

    // New at() methods using ContainerAccess
    T& at(size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        return ObservableContainerHelpers::ContainerAccess<ActualContainer<T, Allocator>, T>::get(data_, index);
    }

    const T& const_at(size_t index) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return ObservableContainerHelpers::ContainerAccess<ActualContainer<T, Allocator>, T>::get(data_, index);
    }

    // Iterators
    using iterator = typename ActualContainer<T, Allocator>::iterator;
    using const_iterator = typename ActualContainer<T, Allocator>::const_iterator;

    iterator begin() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.begin();
    }

    const_iterator begin() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.begin();
    }

    iterator end() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.end();
    }

    const_iterator end() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.end();
    }

    const_iterator cbegin() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.cbegin();
    }

    const_iterator cend() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.cend();
    }

    void clear() {
        bool was_not_empty = false;
        { 
            std::lock_guard<std::mutex> lock(mutex_);
            if (!data_.empty()) {
                was_not_empty = true;
                data_.clear();
            }
        } 
        if (was_not_empty) {
            notify(ChangeType::SizeChanged);
        }
    }

    iterator insert(const_iterator pos, const T& value) {
        iterator result_it;
        ptrdiff_t insert_idx = -1;
        size_t current_size = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            current_size = data_.size();
            // For std::list, pos needs to be converted to a non-const iterator if insert takes non-const
            // For std::vector, pos can be const. std::list::insert takes const_iterator.
            // The main issue is calculating index if needed for notification.
            // We calculate distance to pos for index notification.
            insert_idx = std::distance(data_.cbegin(), pos);
            
            // To get a non-const iterator for insertion for vector, if it were needed:
            auto it_non_const = data_.begin();
            std::advance(it_non_const, insert_idx); 
            // For std::list, insert directly takes const_iterator 'pos'.
            // For std::vector, insert also takes const_iterator 'pos'.
            // So no conversion of 'pos' is strictly needed for the call to data_.insert itself.

            if (insert_idx >= 0 && static_cast<size_t>(insert_idx) <= current_size) {
                 result_it = data_.insert(pos, value); // Use original pos (const_iterator)
            } else {
                 result_it = data_.end(); 
                 insert_idx = -1; 
            }
        }

        if (insert_idx != -1) {
            notify(ChangeType::ElementAdded, static_cast<size_t>(insert_idx), std::nullopt, value);
            notify(ChangeType::SizeChanged);
        }
        return result_it;
    }

    iterator erase(const_iterator pos) {
        iterator result_it;
        bool erased = false;
        T old_value;
        ptrdiff_t erase_idx = -1;
        size_t current_size = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            current_size = data_.size();
            erase_idx = std::distance(data_.cbegin(), pos);

            if (erase_idx >= 0 && static_cast<size_t>(erase_idx) < current_size) {
                // Need to capture old_value before erasing
                // For std::list, *pos is fine. For vector, also *pos.
                old_value = *pos; 
                // std::list::erase and std::vector::erase take const_iterator
                result_it = data_.erase(pos);
                erased = true;
            } else {
                result_it = data_.end(); 
                erase_idx = -1;
            }
        }

        if (erased) {
            notify(ChangeType::ElementRemoved, static_cast<size_t>(erase_idx), old_value, std::nullopt);
            notify(ChangeType::SizeChanged);
        }
        return result_it;
    }

    // Modify using ContainerAccess helper
    void modify(size_t index, const T& newValue) {
        bool modified_flag = false;
        T old_value;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (index < data_.size()) {
                old_value = ObservableContainerHelpers::ContainerAccess<ActualContainer<T, Allocator>, T>::get(data_, index);
                if (old_value != newValue) { // Optional: notify only if value actually changes
                    ObservableContainerHelpers::ContainerAccess<ActualContainer<T, Allocator>, T>::set(data_, index, newValue);
                    modified_flag = true;
                }
            }
        }
        if (modified_flag) {
            notify(ChangeType::ElementModified, index, old_value, newValue);
        }
    }

    void modify(size_t index, T&& newValue) {
        bool modified_flag = false;
        T old_value;
        T final_new_value; // To capture the state of newValue after potential move
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (index < data_.size()) {
                old_value = ObservableContainerHelpers::ContainerAccess<ActualContainer<T, Allocator>, T>::get(data_, index);
                // Capture newValue for notification *before* it's moved from if it's an rvalue reference
                // If T is a simple type, this is just a copy. If T is complex, this might copy.
                // This is tricky. The most accurate newValue for notification is data_[index] after modification.
                ObservableContainerHelpers::ContainerAccess<ActualContainer<T, Allocator>, T>::set(data_, index, std::move(newValue));
                modified_flag = true; 
                final_new_value = ObservableContainerHelpers::ContainerAccess<ActualContainer<T, Allocator>, T>::get(data_, index);
            }
        }
        if (modified_flag) {
            notify(ChangeType::ElementModified, index, old_value, final_new_value);
        }
    }
};

#endif // OBSERVABLE_CONTAINER_H
