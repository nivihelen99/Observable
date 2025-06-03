#ifndef CHANGE_EVENT_H
#define CHANGE_EVENT_H

#include <optional> // Required for std::optional
#include <cstddef>  // Required for size_t

// Define an enum class ChangeType
enum class ChangeType {
    ElementAdded,
    ElementRemoved,
    ElementModified,
    SizeChanged,
    BatchUpdate // Added new change type
};

// Define a template struct ChangeEvent
template <typename T>
struct ChangeEvent {
    // Public member ChangeType type
    ChangeType type;

    // Optional members for detailed information
    std::optional<size_t> index;
    std::optional<T> oldValue;
    std::optional<T> newValue;
    std::optional<size_t> newSize; // New field

    // Constructor that takes a ChangeType to initialize type
    // And optionally index, old value, new value, and new size
    ChangeEvent(ChangeType type,
                std::optional<size_t> idx = std::nullopt,
                std::optional<T> old_v = std::nullopt,
                std::optional<T> new_v = std::nullopt,
                std::optional<size_t> new_s = std::nullopt) // New parameter
        : type(type), index(idx), oldValue(old_v), newValue(new_v), newSize(new_s) {} // Initialize new field
};

#endif // CHANGE_EVENT_H

