#include <iostream>
#include <string>
#include <vector> // Required for iterating through container in print
#include "ObservableContainer.h" // Assuming ObservableContainer.h is in the same directory
#include "ChangeEvent.h"       // Assuming ChangeEvent.h is in the same directory
#include "ScopedModifier.h"    // For ScopedModifier

// Helper function to convert ChangeType enum to a string
std::string ChangeTypeToString(ChangeType type) {
    switch (type) {
        case ChangeType::ElementAdded:   return "ElementAdded";
        case ChangeType::ElementRemoved: return "ElementRemoved";
        case ChangeType::ElementModified:return "ElementModified";
        case ChangeType::SizeChanged:    return "SizeChanged";
        case ChangeType::BatchUpdate:    return "BatchUpdate"; // Added
        default:                         return "UnknownChange";
    }
}

// Helper function to print container contents
template <typename T>
void PrintContainer(const ObservableContainer<T>& container, const std::string& label = "Container") {
    std::cout << label << " (size: " << container.size() << "): [ ";
    for (const auto& elem : container) {
        std::cout << elem << " ";
    }
    std::cout << "]" << std::endl;
}

int main() {
    std::cout << "--- Initializing ObservableContainer<int> ---" << std::endl;
    ObservableContainer<int> oc;

    // Define observer lambdas
    auto observer1 = [](const ChangeEvent& event) {
        std::cout << "Observer 1 detected: " << ChangeTypeToString(event.type) << std::endl;
    };

    auto observer2 = [](const ChangeEvent& event) {
        std::cout << "  Observer 2 detected: " << ChangeTypeToString(event.type) << std::endl;
    };

    std::cout << "\n--- Registering observers ---" << std::endl;
    oc.addObserver(observer1);
    oc.addObserver(observer2);

    PrintContainer(oc, "Initial state");

    std::cout << "\n--- Testing push_back(10) ---" << std::endl;
    oc.push_back(10);
    PrintContainer(oc);

    std::cout << "\n--- Testing push_back(20) ---" << std::endl;
    oc.push_back(20);
    PrintContainer(oc);

    std::cout << "\n--- Testing modify(0, 15) ---" << std::endl;
    if (oc.size() > 0) {
        oc.modify(0, 15);
    } else {
        std::cout << "Container empty, skipping modify." << std::endl;
    }
    PrintContainer(oc);

    std::cout << "\n--- Testing insert(oc.begin() + 1, 25) ---" << std::endl;
    if (oc.size() >= 1) { // Need at least one element to insert at begin() + 1
        oc.insert(oc.cbegin() + 1, 25); // Use cbegin() for const_iterator
    } else if (oc.empty()){
         oc.insert(oc.cbegin(), 25); // Insert at beginning if empty
    }
     else {
        std::cout << "Container too small or empty, adjusting insert or skipping." << std::endl;
    }
    PrintContainer(oc);

    std::cout << "\n--- Testing erase(oc.begin()) ---" << std::endl;
    if (!oc.empty()) {
        oc.erase(oc.cbegin()); // Use cbegin() for const_iterator
    } else {
        std::cout << "Container empty, skipping erase." << std::endl;
    }
    PrintContainer(oc);
    
    std::cout << "\n--- Testing pop_back() ---" << std::endl;
    if (!oc.empty()) {
        oc.pop_back();
    } else {
        std::cout << "Container empty, skipping pop_back." << std::endl;
    }
    PrintContainer(oc);

    std::cout << "\n--- Testing removeObserver ---" << std::endl;
    auto observer3 = [](const ChangeEvent& event) {
        std::cout << "    Observer 3 detected: " << ChangeTypeToString(event.type) << std::endl;
    };

    std::cout << "Adding Observer 3..." << std::endl;
    oc.addObserver(observer3);

    std::cout << "\n--- Performing action with Observer 3 active: push_back(30) ---" << std::endl;
    oc.push_back(30);
    PrintContainer(oc);

    std::cout << "\nRemoving Observer 3..." << std::endl;
    oc.removeObserver(observer3); // Attempt to remove observer3

    std::cout << "\n--- Performing action after Observer 3 removal: push_back(40) ---" << std::endl;
    oc.push_back(40);
    PrintContainer(oc);
    std::cout << "(Observer 3 should not print for the push_back(40) operation if removal was successful)" << std::endl;


    std::cout << "\n--- Testing clear() ---" << std::endl;
    oc.clear();
    PrintContainer(oc);
    
    std::cout << "\n--- Testing operations on empty container after clear ---" << std::endl;
    std::cout << "--- push_back(5) ---" << std::endl;
    oc.push_back(5);
    PrintContainer(oc);

    std::cout << "--- pop_back() ---" << std::endl;
    oc.pop_back();
    PrintContainer(oc);

    std::cout << "\n--- End of tests ---" << std::endl;


    std::cout << "\n\n--- Testing Copy and Move Semantics ---" << std::endl;
    ObservableContainer<int> oc_source;
    auto source_obs = [](const ChangeEvent& event) {
        std::cout << "Source Observer (oc_source) detected: " << ChangeTypeToString(event.type) << std::endl;
    };
    oc_source.addObserver(source_obs);
    oc_source.push_back(1);
    oc_source.push_back(2);
    oc_source.push_back(3);
    PrintContainer(oc_source, "Initial oc_source");

    // Test Copy Construction
    std::cout << "\n--- Test: Copy Construction (oc_copy_ctor = oc_source) ---" << std::endl;
    ObservableContainer<int> oc_copy_ctor = oc_source;
    PrintContainer(oc_copy_ctor, "oc_copy_ctor (after copy from oc_source)");
    PrintContainer(oc_source, "oc_source (after copy to oc_copy_ctor - should be unchanged)");
    auto copy_ctor_obs = [](const ChangeEvent& event) {
        std::cout << "CopyCtor Observer (oc_copy_ctor) detected: " << ChangeTypeToString(event.type) << std::endl;
    };
    oc_copy_ctor.addObserver(copy_ctor_obs);
    std::cout << "Action: oc_copy_ctor.push_back(4)" << std::endl;
    oc_copy_ctor.push_back(4); // copy_ctor_obs should trigger
    std::cout << "Action: oc_source.push_back(0)" << std::endl;
    oc_source.push_back(0);   // source_obs should trigger

    // Test Copy Assignment
    std::cout << "\n--- Test: Copy Assignment (oc_copy_assign = oc_source) ---" << std::endl;
    ObservableContainer<int> oc_copy_assign;
    oc_copy_assign.push_back(99);
    auto copy_assign_obs_old = [](const ChangeEvent& event) {
        std::cout << "CopyAssign OLD Observer (oc_copy_assign) detected: " << ChangeTypeToString(event.type) << std::endl;
    };
    oc_copy_assign.addObserver(copy_assign_obs_old);
    std::cout << "Action: oc_copy_assign.push_back(77) (before assignment)" << std::endl;
    oc_copy_assign.push_back(77); // copy_assign_obs_old should trigger
    std::cout << "Action: oc_copy_assign = oc_source" << std::endl;
    oc_copy_assign = oc_source; // BatchUpdate should be logged by oc_copy_assign's own observers (none after clear)
                               // For this test, we expect NO output from copy_assign_obs_old for this line.
                               // Any new observers added after this would see BatchUpdate if it was implemented
                               // to use its *own* observer list for the assignment notification.
                               // My implementation notifies its *current* (empty) list.
    PrintContainer(oc_copy_assign, "oc_copy_assign (after assign from oc_source)");
    PrintContainer(oc_source, "oc_source (after assign to oc_copy_assign - should be unchanged)");
    std::cout << "Action: oc_copy_assign.push_back(5) (after assignment)" << std::endl;
    oc_copy_assign.push_back(5); // copy_assign_obs_old should NOT trigger
    auto copy_assign_obs_new = [](const ChangeEvent& event) {
        std::cout << "CopyAssign NEW Observer (oc_copy_assign) detected: " << ChangeTypeToString(event.type) << std::endl;
    };
    oc_copy_assign.addObserver(copy_assign_obs_new);
    std::cout << "Action: oc_copy_assign.push_back(6) (with new observer)" << std::endl;
    oc_copy_assign.push_back(6); // copy_assign_obs_new should trigger
    std::cout << "Action: oc_source.modify(0, 11)" << std::endl;
    oc_source.modify(0, 11);     // source_obs should trigger

    // Test Move Construction
    std::cout << "\n--- Test: Move Construction (oc_move_ctor = std::move(oc_source_for_move)) ---" << std::endl;
    ObservableContainer<int> oc_source_for_move;
    auto source_for_move_obs = [](const ChangeEvent& event) {
        std::cout << "SourceForMove Observer (oc_source_for_move) detected: " << ChangeTypeToString(event.type) << std::endl;
    };
    oc_source_for_move.addObserver(source_for_move_obs);
    oc_source_for_move.push_back(10);
    oc_source_for_move.push_back(20);
    PrintContainer(oc_source_for_move, "oc_source_for_move (before move)");
    ObservableContainer<int> oc_move_ctor = std::move(oc_source_for_move);
    PrintContainer(oc_move_ctor, "oc_move_ctor (after move from oc_source_for_move)");
    PrintContainer(oc_source_for_move, "oc_source_for_move (after move - should be empty/valid)");
    auto move_ctor_obs = [](const ChangeEvent& event) {
        std::cout << "MoveCtor Observer (oc_move_ctor) detected: " << ChangeTypeToString(event.type) << std::endl;
    };
    oc_move_ctor.addObserver(move_ctor_obs);
    std::cout << "Action: oc_move_ctor.push_back(30)" << std::endl;
    oc_move_ctor.push_back(30); // move_ctor_obs should trigger
    std::cout << "Action: oc_source_for_move.push_back(1000) (after being moved from)" << std::endl;
    oc_source_for_move.push_back(1000); // source_for_move_obs should trigger

    // Test Move Assignment
    std::cout << "\n--- Test: Move Assignment (oc_move_assign = std::move(oc_source_for_move2)) ---" << std::endl;
    ObservableContainer<int> oc_move_assign;
    oc_move_assign.push_back(55);
    auto move_assign_obs_old = [](const ChangeEvent& event) {
        std::cout << "MoveAssign OLD Observer (oc_move_assign) detected: " << ChangeTypeToString(event.type) << std::endl;
    };
    oc_move_assign.addObserver(move_assign_obs_old);
    std::cout << "Action: oc_move_assign.push_back(66) (before assignment)" << std::endl;
    oc_move_assign.push_back(66); // move_assign_obs_old should trigger
    ObservableContainer<int> oc_source_for_move2;
    oc_source_for_move2.push_back(70);
    oc_source_for_move2.push_back(80);
    auto source_for_move2_obs = [](const ChangeEvent& event) {
        std::cout << "SourceForMove2 Observer (oc_source_for_move2) detected: " << ChangeTypeToString(event.type) << std::endl;
    };
    oc_source_for_move2.addObserver(source_for_move2_obs);
    PrintContainer(oc_source_for_move2, "oc_source_for_move2 (before move assign)");
    std::cout << "Action: oc_move_assign = std::move(oc_source_for_move2)" << std::endl;
    oc_move_assign = std::move(oc_source_for_move2); // BatchUpdate should be logged by oc_move_assign's own observers (none after clear)
    PrintContainer(oc_move_assign, "oc_move_assign (after move assign from oc_source_for_move2)");
    PrintContainer(oc_source_for_move2, "oc_source_for_move2 (after move assign - should be empty/valid)");
    std::cout << "Action: oc_move_assign.push_back(90) (after assignment)" << std::endl;
    oc_move_assign.push_back(90); // move_assign_obs_old should NOT trigger
    auto move_assign_obs_new = [](const ChangeEvent& event) {
        std::cout << "MoveAssign NEW Observer (oc_move_assign) detected: " << ChangeTypeToString(event.type) << std::endl;
    };
    oc_move_assign.addObserver(move_assign_obs_new);
    std::cout << "Action: oc_move_assign.push_back(100) (with new observer)" << std::endl;
    oc_move_assign.push_back(100); // move_assign_obs_new should trigger
    std::cout << "Action: oc_source_for_move2.push_back(2000) (after being moved from)" << std::endl;
    oc_source_for_move2.push_back(2000); // source_for_move2_obs should trigger
    
    std::cout << "\n--- End of Copy and Move Semantics Tests ---" << std::endl;

    // Clean up main observers from original tests if necessary, or re-declare 'oc' for ScopedModifier test
    // For simplicity, the 'oc' from ScopedModifier test is separate.
    // We can remove observers from oc_source and the 'oc' used in ScopedModifier to avoid confusion if tests were merged.
    oc_source.removeObserver(source_obs); // removeObserver has limitations with lambdas, but good practice to call


    std::cout << "\n--- Testing ScopedModifier ---" << std::endl;
    // Add a new observer to specifically see ScopedModifier effects
    auto scopedTestObserver = [](const ChangeEvent& event){
        std::cout << "ScopedTestObserver detected: " << ChangeTypeToString(event.type) << std::endl;
    };
    oc.addObserver(scopedTestObserver);

    std::cout << "Clearing container before ScopedModifier test..." << std::endl;
    oc.clear(); // This clear is outside the batch, should notify normally.
    PrintContainer(oc, "After clear, before ScopedModifier");

    std::cout << "\nStarting batched operations using ScopedModifier..." << std::endl;
    {
        ScopedModifier<int> sm(oc); // beginUpdate() called
        std::cout << "Inside ScopedModifier scope." << std::endl;
        
        std::cout << "Action: oc.push_back(100)" << std::endl;
        oc.push_back(100); // Should be batched
        PrintContainer(oc, "After push_back(100)");

        std::cout << "Action: oc.push_back(200)" << std::endl;
        oc.push_back(200); // Should be batched
        PrintContainer(oc, "After push_back(200)");

        std::cout << "Action: oc.modify(0, 101)" << std::endl;
        if(oc.size() > 0) {
            oc.modify(0, 101); // Should be batched
        }
        PrintContainer(oc, "After modify(0, 101)");

        std::cout << "Action: oc.pop_back()" << std::endl;
        if(!oc.empty()) {
            oc.pop_back();     // Should be batched
        }
        PrintContainer(oc, "After pop_back()");
        
        std::cout << "Inside ScopedModifier, current size: " << oc.size() << std::endl;
        std::cout << "No individual notifications should have appeared from ScopedTestObserver yet." << std::endl;
        std::cout << "Exiting ScopedModifier scope..." << std::endl;
    } // ScopedModifier sm goes out of scope here, endUpdate() called, BatchUpdate should be sent

    std::cout << "\nFinished batched operations." << std::endl;
    PrintContainer(oc, "Final state after ScopedModifier");
    std::cout << "A 'BatchUpdate' notification should have appeared from all observers." << std::endl;

    // Clean up the specific observer for this test if desired, though not strictly necessary for this example
    // oc.removeObserver(scopedTestObserver); // `removeObserver` has known limitations with lambdas

    std::cout << "\n--- End of ScopedModifier tests ---" << std::endl;


    return 0;
}
