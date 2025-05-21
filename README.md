# ObservableContainer in C++

## Overview

`ObservableContainer<T>` is a C++ template class that wraps a `std::vector<T>` and provides a mechanism to notify registered observers about changes to the container's content or state. This allows for decoupled components to react to modifications such as element additions, removals, or updates.

## Features

*   **Templated Container**: Works with any type `T`.
*   **Wraps `std::vector<T>`**: Provides a familiar vector-like interface.
*   **Observer Pattern**: Allows multiple observers to subscribe to changes.
    *   Observers are callback functions (`std::function<void(const ChangeEvent&)>`).
*   **Change Event Notifications**: Observers are notified of:
    *   `ElementAdded`: An element is added (e.g., via `push_back`, `insert`).
    *   `ElementRemoved`: An element is removed (e.g., via `pop_back`, `erase`, `clear`).
    *   `ElementModified`: An element is modified (e.g., via the `modify` method).
    *   `SizeChanged`: The size of the container changes.
    *   `BatchUpdate`: Multiple operations were grouped (e.g., via `ScopedModifier` or assignments).
*   **Supported Operations**:
    *   `addObserver(callback)` / `removeObserver(callback)`
    *   `push_back()`, `pop_back()`
    *   `insert()`, `erase()`
    *   `operator[]` (for access, use `modify()` for observed changes)
    *   `modify()` (for explicit, observed element modification)
    *   `clear()`
    *   `size()`, `empty()`
    *   `begin()`, `end()` iterators (const and non-const)
*   **Scoped Batch Updates (Bonus)**:
    *   `ScopedModifier<T>` class allows grouping multiple operations. Notifications are deferred until the `ScopedModifier` object goes out of scope, at which point a single `BatchUpdate` event is typically triggered if changes occurred.
*   **Copy and Move Semantics (Bonus)**:
    *   Supports copy construction, copy assignment, move construction, and move assignment.
    *   Observers are **not** copied or moved; the new or assigned-to container will have an empty list of observers.
    *   Assignment operations trigger a `BatchUpdate` notification.

## Files

*   `ChangeEvent.h`: Defines `ChangeType` enum and `ChangeEvent` struct.
*   `ObservableContainer.h`: Contains the implementation of `ObservableContainer<T>`.
*   `ScopedModifier.h`: Contains the implementation of `ScopedModifier<T>`.
*   `main.cpp`: Example usage and test cases.
*   `README.md`: This file.

## Usage Example

```cpp
#include <iostream>
#include "ObservableContainer.h"
#include "ChangeEvent.h"
#include "ScopedModifier.h" // If using ScopedModifier

// Helper to print event types (can be also defined in your main)
std::string ChangeTypeToString(ChangeType type) {
    switch (type) {
        case ChangeType::ElementAdded: return "ElementAdded";
        case ChangeType::ElementRemoved: return "ElementRemoved";
        case ChangeType::ElementModified: return "ElementModified";
        case ChangeType::SizeChanged: return "SizeChanged";
        case ChangeType::BatchUpdate: return "BatchUpdate";
        default: return "UnknownChange";
    }
}

int main() {
    ObservableContainer<int> oc;

    // Add an observer
    oc.addObserver([](const ChangeEvent& event) {
        std::cout << "Observer 1: Event - " << ChangeTypeToString(event.type) << std::endl;
        // For more details, ChangeEvent could be extended to include index, old/new values etc.
    });

    std::cout << "--- Pushing back 10 ---" << std::endl;
    oc.push_back(10); // Triggers ElementAdded, SizeChanged

    std::cout << "\n--- Modifying element at index 0 to 15 ---" << std::endl;
    if (!oc.empty()) {
        oc.modify(0, 15); // Triggers ElementModified
    }

    std::cout << "\n--- Using ScopedModifier ---" << std::endl;
    {
        ScopedModifier<int> sm(oc);
        std::cout << "Performing batch operations..." << std::endl;
        oc.push_back(20);
        oc.push_back(30);
        if (oc.size() > 2) oc.modify(1, 25); // Modify one of the new elements
        // No individual notifications during these operations from the ScopedModifier
    } // ScopedModifier goes out of scope
      // Triggers BatchUpdate

    std::cout << "\n--- Clearing container ---" << std::endl;
    oc.clear(); // Triggers SizeChanged (if not empty)

    return 0;
}

```

## Building and Running

To compile and run the example `main.cpp` (assuming a C++17 compatible compiler like g++):

1.  **Ensure all header files (`ChangeEvent.h`, `ObservableContainer.h`, `ScopedModifier.h`) are in the same directory as `main.cpp`.**
2.  **Compile:**
    ```bash
    g++ -std=c++17 main.cpp -o observer_test
    ```
3.  **Run:**
    ```bash
    ./observer_test
    ```

This will execute the test cases defined in `main.cpp`, and you should see output indicating the operations performed and the notifications received by the observers.

## Future Considerations (Not Implemented)

*   **Thread Safety**: The current implementation is not thread-safe. Mutexes or other synchronization primitives would be needed to protect shared data access in a multi-threaded environment.
*   **Enhanced `ChangeEvent`**: The `ChangeEvent` struct could be extended to include more details, such as the index of the changed element, the old value, and the new value.
*   **Observer Management**: For `removeObserver`, using handles or IDs returned by `addObserver` would make removal more robust than relying on `std::function` comparison.
*   **More Container Types**: Could be extended to wrap other standard containers.
*   **Fine-grained Notifications from `ScopedModifier`**: Instead of one `BatchUpdate`, the `ScopedModifier` could optionally queue all individual events and dispatch them at the end of its scope.
