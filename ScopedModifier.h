#ifndef SCOPED_MODIFIER_H
#define SCOPED_MODIFIER_H

#include "ObservableContainer.h" // Needs the definition of ObservableContainer

template <typename T>
class ScopedModifier {
private:
    ObservableContainer<T>& container_ref_;

public:
    // Constructor: stores the reference and calls beginUpdate()
    explicit ScopedModifier(ObservableContainer<T>& container)
        : container_ref_(container) {
        container_ref_.beginUpdate();
    }

    // Destructor: calls endUpdate()
    ~ScopedModifier() {
        container_ref_.endUpdate();
    }

    // Disable Copy and Move operations
    ScopedModifier(const ScopedModifier&) = delete;
    ScopedModifier& operator=(const ScopedModifier&) = delete;
    ScopedModifier(ScopedModifier&&) = delete;
    ScopedModifier& operator=(ScopedModifier&&) = delete;
};

#endif // SCOPED_MODIFIER_H
