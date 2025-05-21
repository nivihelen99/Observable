#ifndef CHANGE_EVENT_H
#define CHANGE_EVENT_H

// Define an enum class ChangeType
enum class ChangeType {
    ElementAdded,
    ElementRemoved,
    ElementModified,
    SizeChanged,
    BatchUpdate // Added new change type
};

// Define a struct ChangeEvent
struct ChangeEvent {
    // Public member ChangeType type
    ChangeType type;

    // Constructor that takes a ChangeType to initialize type
    ChangeEvent(ChangeType type) : type(type) {}
};

#endif // CHANGE_EVENT_H
