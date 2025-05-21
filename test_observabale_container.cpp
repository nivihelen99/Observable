#include "gtest/gtest.h"
#include "ObservableContainer.h" // Now uses the new interface
#include "ChangeEvent.h"         // Now uses the new interface
#include <string>                // Required for std::string
#include <vector>                // Required for std::vector
#include <functional>            // Required for std::function
#include <iterator>              // Required for std::distance, std::begin, std::end
#include <utility>               // Required for std::move
#include <algorithm>             // Required for std::equal

// Test fixture
class ObservableContainerTest : public ::testing::Test {
protected:
    ObservableContainerTest() {}
    ~ObservableContainerTest() override {}

    // Helper to convert event types to string for better error messages
    static std::string EventTypeToString(ChangeType type) {
        switch (type) {
            case ChangeType::ElementAdded: return "ElementAdded";
            case ChangeType::ElementRemoved: return "ElementRemoved";
            case ChangeType::ElementModified: return "ElementModified";
            case ChangeType::SizeChanged: return "SizeChanged";
            case ChangeType::BatchUpdate: return "BatchUpdate";
            default: return "Unknown"; // Should not happen with a well-defined enum
        }
    }

    // Helper to compare expected and actual event sequences
    static void AssertEventSequence(const std::vector<ChangeType>& actual, const std::vector<ChangeType>& expected) {
        ASSERT_EQ(actual.size(), expected.size()) << "Event sequence length mismatch.";
        for (size_t i = 0; i < actual.size(); ++i) {
            if (actual[i] != expected[i]) {
                std::string actual_str, expected_str;
                for(size_t j=0; j<actual.size(); ++j) actual_str += EventTypeToString(actual[j]) + (j == actual.size()-1 ? "" : ", ");
                for(size_t j=0; j<expected.size(); ++j) expected_str += EventTypeToString(expected[j]) + (j == expected.size()-1 ? "" : ", ");
                GTEST_FAIL() << "Event mismatch at index " << i
                             << ". Expected sequence: [" << expected_str
                             << "], Got sequence: [" << actual_str << "]";
            }
        }
    }
};

// Test case for default construction of ObservableContainer<int>
TEST_F(ObservableContainerTest, DefaultConstructionIntIsEmpty) {
    ObservableContainer<int> container;
    ASSERT_TRUE(container.empty());
    ASSERT_EQ(0, container.size());
}

// Test case for default construction of ObservableContainer<std::string>
TEST_F(ObservableContainerTest, DefaultConstructionStringIsEmpty) {
    ObservableContainer<std::string> container;
    ASSERT_TRUE(container.empty());
    ASSERT_EQ(0, container.size());
}

// Test case for push_back with ObservableContainer<int>
TEST_F(ObservableContainerTest, PushBackObservesCorrectEventsInt) {
    ObservableContainer<int> container;
    std::vector<ChangeType> received_event_types;

    container.addObserver([&](const ChangeEvent& event) {
        received_event_types.push_back(event.type);
    });

    container.push_back(10);
    EXPECT_EQ(container.size(), 1);
    ASSERT_FALSE(container.empty());
    EXPECT_EQ(container[0], 10);
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    received_event_types.clear();

    container.push_back(20);
    EXPECT_EQ(container.size(), 2);
    EXPECT_EQ(container[1], 20);
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    received_event_types.clear();
}

// Test case for push_back with ObservableContainer<std::string>
TEST_F(ObservableContainerTest, PushBackObservesCorrectEventsString) {
    ObservableContainer<std::string> container;
    std::vector<ChangeType> received_event_types;

    container.addObserver([&](const ChangeEvent& event) {
        received_event_types.push_back(event.type);
    });

    container.push_back("hello");
    EXPECT_EQ(container.size(), 1);
    ASSERT_FALSE(container.empty());
    EXPECT_EQ(container[0], "hello");
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    received_event_types.clear();

    container.push_back("world");
    EXPECT_EQ(container.size(), 2);
    EXPECT_EQ(container[1], "world");
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    received_event_types.clear();
}

// Test case for pop_back with ObservableContainer<int>
TEST_F(ObservableContainerTest, PopBackObservesCorrectEventsInt) {
    ObservableContainer<int> container;
    container.push_back(10); 
    container.push_back(20);
    container.push_back(30);

    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event) {
       received_event_types.push_back(event.type);
    });
    received_event_types.clear(); 

    container.pop_back(); 
    EXPECT_EQ(container.size(), 2);
    AssertEventSequence(received_event_types, {ChangeType::ElementRemoved, ChangeType::SizeChanged});
    received_event_types.clear();

    container.pop_back();
    EXPECT_EQ(container.size(), 1);
    AssertEventSequence(received_event_types, {ChangeType::ElementRemoved, ChangeType::SizeChanged});
    received_event_types.clear();

    container.pop_back();
    EXPECT_EQ(container.size(), 0);
    EXPECT_TRUE(container.empty());
    AssertEventSequence(received_event_types, {ChangeType::ElementRemoved, ChangeType::SizeChanged});
    received_event_types.clear();

    container.pop_back(); 
    EXPECT_EQ(container.size(), 0);
    EXPECT_TRUE(container.empty());
    EXPECT_TRUE(received_event_types.empty()) << "Expected no events on pop_back from empty container.";
}

// --- Observer Management Tests ---

TEST_F(ObservableContainerTest, SingleObserverReceivesNotification) {
    ObservableContainer<int> container;
    bool flag = false;
    std::vector<ChangeType> events;
    
    container.addObserver([&](const ChangeEvent& event) {
        flag = true;
        events.push_back(event.type);
    });

    container.push_back(1);
    EXPECT_TRUE(flag);
    AssertEventSequence(events, {ChangeType::ElementAdded, ChangeType::SizeChanged});
}

TEST_F(ObservableContainerTest, MultipleObserversReceiveNotifications) {
    ObservableContainer<int> container;
    int counterA = 0;
    int counterB = 0;

    container.addObserver([&](const ChangeEvent& event) {
        if (event.type == ChangeType::ElementAdded) counterA++;
    });
    container.addObserver([&](const ChangeEvent& event) {
        if (event.type == ChangeType::ElementAdded) counterB++;
    });

    container.push_back(1);
    EXPECT_EQ(counterA, 1);
    EXPECT_EQ(counterB, 1);
}

TEST_F(ObservableContainerTest, RemoveObserverStopsNotifications) {
    ObservableContainer<int> container;
    int countA = 0;
    int countB = 0;

    std::function<void(const ChangeEvent&)> observerA = 
        [&](const ChangeEvent& event) { if(event.type == ChangeType::ElementAdded) countA++; };
    std::function<void(const ChangeEvent&)> observerB = 
        [&](const ChangeEvent& event) { if(event.type == ChangeType::ElementAdded) countB++; };

    container.addObserver(observerA);
    container.addObserver(observerB);
    container.push_back(1);
    EXPECT_EQ(countA, 1);
    EXPECT_EQ(countB, 1);

    container.removeObserver(observerA);
    countA = 0; countB = 0; 
    container.push_back(2);
    EXPECT_EQ(countA, 0);
    EXPECT_EQ(countB, 1);
}

TEST_F(ObservableContainerTest, RemoveNonExistentObserver) {
    ObservableContainer<int> container;
    int countActual = 0;
    
    std::function<void(const ChangeEvent&)> observerToTryToRemove = 
        [](const ChangeEvent& event) { /* NOP */ };
    std::function<void(const ChangeEvent&)> observerAdded = 
        [&](const ChangeEvent& event) { if(event.type == ChangeType::ElementAdded) countActual++; };

    container.addObserver(observerAdded);
    ASSERT_NO_THROW(container.removeObserver(observerToTryToRemove));
    container.push_back(1);
    EXPECT_EQ(countActual, 1);
}

TEST_F(ObservableContainerTest, RemoveAlreadyRemovedObserver) {
    ObservableContainer<int> container;
    int count1 = 0;

    std::function<void(const ChangeEvent&)> observer1 = 
        [&](const ChangeEvent& event) { if(event.type == ChangeType::ElementAdded) count1++; };
    
    container.addObserver(observer1);
    container.push_back(1);
    EXPECT_EQ(count1, 1);

    container.removeObserver(observer1);
    count1 = 0;
    container.push_back(2);
    EXPECT_EQ(count1, 0);

    ASSERT_NO_THROW(container.removeObserver(observer1));
}

TEST_F(ObservableContainerTest, RemoveObserverRobustness_SimpleLambda) {
    ObservableContainer<int> container;
    int call_count = 0;

    std::function<void(const ChangeEvent&)> observer_to_remove = 
        [&call_count](const ChangeEvent& event) { 
            if (event.type == ChangeType::ElementAdded) call_count++;
        };

    container.addObserver(observer_to_remove);
    container.push_back(1);
    ASSERT_EQ(call_count, 1);

    container.removeObserver(observer_to_remove);
    call_count = 0; 
    container.push_back(2);
    ASSERT_EQ(call_count, 0);
}

// --- operator[] Tests ---

TEST_F(ObservableContainerTest, OperatorBracketAccessAndModification) {
    ObservableContainer<int> container;
    container.push_back(10);
    container.push_back(20);

    bool notified = false;
    std::vector<ChangeType> received_events;
    container.addObserver([&](const ChangeEvent& event){
        notified = true;
        received_events.push_back(event.type);
    });
    received_events.clear(); 
    notified = false;

    const auto& const_container = container;
    EXPECT_EQ(const_container[0], 10);
    EXPECT_EQ(const_container[1], 20);
    EXPECT_FALSE(notified);
    EXPECT_TRUE(received_events.empty());

    EXPECT_EQ(container[0], 10);
    EXPECT_FALSE(notified);
    EXPECT_TRUE(received_events.empty());

    container[0] = 100;
    EXPECT_EQ(container[0], 100);
    EXPECT_FALSE(notified);
    EXPECT_TRUE(received_events.empty());

    container[1] = 200;
    EXPECT_EQ(container[1], 200);
    EXPECT_FALSE(notified);
    EXPECT_TRUE(received_events.empty());
}

TEST_F(ObservableContainerTest, OperatorBracketAccessString) {
    ObservableContainer<std::string> container;
    container.push_back("alpha");
    container.push_back("beta");

    bool notified = false;
    std::vector<ChangeType> received_events;
    container.addObserver([&](const ChangeEvent& event){
        notified = true;
        received_events.push_back(event.type);
    });
    received_events.clear();
    notified = false;

    const auto& const_container = container;
    EXPECT_EQ(const_container[0], "alpha");
    EXPECT_EQ(const_container[1], "beta");
    EXPECT_FALSE(notified);
    EXPECT_TRUE(received_events.empty());

    EXPECT_EQ(container[0], "alpha");
    EXPECT_FALSE(notified);
    EXPECT_TRUE(received_events.empty());
    
    container[0] = "gamma";
    EXPECT_EQ(container[0], "gamma");
    EXPECT_FALSE(notified);
    EXPECT_TRUE(received_events.empty());

    container[1] = "delta";
    EXPECT_EQ(container[1], "delta");
    EXPECT_FALSE(notified);
    EXPECT_TRUE(received_events.empty());
}

// --- modify() method Tests ---

TEST_F(ObservableContainerTest, ModifyExistingElement) {
    ObservableContainer<int> container;
    container.push_back(10);
    container.push_back(20);
    container.push_back(30);

    bool notified = false;
    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        notified = true;
        received_event_types.push_back(event.type);
    });
    received_event_types.clear(); 
    notified = false;

    size_t original_size = container.size();
    container.modify(1, 25); 

    EXPECT_EQ(container[1], 25);
    EXPECT_TRUE(notified);
    AssertEventSequence(received_event_types, {ChangeType::ElementModified});
    
    EXPECT_EQ(container[0], 10); 
    EXPECT_EQ(container[2], 30);
    EXPECT_EQ(container.size(), original_size); 
}

TEST_F(ObservableContainerTest, ModifyExistingElementString) {
    ObservableContainer<std::string> container;
    container.push_back("a");
    container.push_back("b");
    container.push_back("c");

    bool notified = false;
    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        notified = true;
        received_event_types.push_back(event.type);
    });
    received_event_types.clear();
    notified = false;

    size_t original_size = container.size();
    container.modify(0, "apple"); 

    EXPECT_EQ(container[0], "apple");
    EXPECT_TRUE(notified);
    AssertEventSequence(received_event_types, {ChangeType::ElementModified});

    EXPECT_EQ(container[1], "b"); 
    EXPECT_EQ(container[2], "c");
    EXPECT_EQ(container.size(), original_size); 
}

TEST_F(ObservableContainerTest, ModifyOutOfBounds) {
    ObservableContainer<int> container;
    container.push_back(10);

    bool notified = false;
    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        notified = true;
        received_event_types.push_back(event.type);
    });
    received_event_types.clear();
    notified = false;

    size_t original_size = container.size();
    int original_value = container[0];

    container.modify(5, 100); 

    EXPECT_FALSE(notified);
    EXPECT_TRUE(received_event_types.empty());
    EXPECT_EQ(container.size(), original_size);
    EXPECT_EQ(container[0], original_value); 
}

TEST_F(ObservableContainerTest, ModifyEmptyContainer) {
    ObservableContainer<int> container; 

    bool notified = false;
    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        notified = true;
        received_event_types.push_back(event.type);
    });
    
    container.modify(0, 100); 

    EXPECT_FALSE(notified);
    EXPECT_TRUE(received_event_types.empty());
    EXPECT_TRUE(container.empty());
    EXPECT_EQ(container.size(), 0);
}

// --- insert() method Tests ---

TEST_F(ObservableContainerTest, InsertIntoEmptyContainer) {
    ObservableContainer<int> container;
    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        received_event_types.push_back(event.type);
    });

    auto it = container.insert(container.begin(), 10);
    
    ASSERT_EQ(container.size(), 1);
    EXPECT_EQ(container[0], 10);
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    ASSERT_NE(it, container.end());
    EXPECT_EQ(*it, 10);
}

TEST_F(ObservableContainerTest, InsertAtBeginning) {
    ObservableContainer<int> container;
    container.push_back(10);
    container.push_back(20);

    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        received_event_types.push_back(event.type);
    });

    auto it = container.insert(container.begin(), 5);

    ASSERT_EQ(container.size(), 3);
    EXPECT_EQ(container[0], 5);
    EXPECT_EQ(container[1], 10);
    EXPECT_EQ(container[2], 20);
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    ASSERT_NE(it, container.end());
    EXPECT_EQ(*it, 5);
}

TEST_F(ObservableContainerTest, InsertInMiddle) {
    ObservableContainer<int> container;
    container.push_back(10);
    container.push_back(20);

    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        received_event_types.push_back(event.type);
    });

    auto it = container.insert(container.begin() + 1, 15);

    ASSERT_EQ(container.size(), 3);
    EXPECT_EQ(container[0], 10);
    EXPECT_EQ(container[1], 15);
    EXPECT_EQ(container[2], 20);
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    ASSERT_NE(it, container.end());
    EXPECT_EQ(*it, 15);
}

TEST_F(ObservableContainerTest, InsertAtEnd) {
    ObservableContainer<int> container;
    container.push_back(10);
    container.push_back(20);

    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        received_event_types.push_back(event.type);
    });

    auto it = container.insert(container.end(), 30);

    ASSERT_EQ(container.size(), 3);
    EXPECT_EQ(container[0], 10);
    EXPECT_EQ(container[1], 20);
    EXPECT_EQ(container[2], 30);
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    ASSERT_NE(it, container.end()); 
    EXPECT_EQ(*it, 30);
}

TEST_F(ObservableContainerTest, InsertStringElements) {
    ObservableContainer<std::string> container;
    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        received_event_types.push_back(event.type);
    });

    auto it1 = container.insert(container.begin(), "alpha");
    ASSERT_EQ(container.size(), 1);
    EXPECT_EQ(container[0], "alpha");
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    ASSERT_NE(it1, container.end());
    EXPECT_EQ(*it1, "alpha");
    received_event_types.clear();

    auto it2 = container.insert(container.begin(), "beta");
    ASSERT_EQ(container.size(), 2);
    EXPECT_EQ(container[0], "beta");
    EXPECT_EQ(container[1], "alpha");
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    ASSERT_NE(it2, container.end());
    EXPECT_EQ(*it2, "beta");
    received_event_types.clear();

    auto it3 = container.insert(container.begin() + 1, "gamma");
    ASSERT_EQ(container.size(), 3);
    EXPECT_EQ(container[0], "beta");
    EXPECT_EQ(container[1], "gamma");
    EXPECT_EQ(container[2], "alpha");
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    ASSERT_NE(it3, container.end());
    EXPECT_EQ(*it3, "gamma");
    received_event_types.clear();
}

// --- erase() method Tests ---

TEST_F(ObservableContainerTest, EraseFromBeginning) {
    ObservableContainer<int> container;
    container.push_back(10);
    container.push_back(20);
    container.push_back(30);

    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        received_event_types.push_back(event.type);
    });

    auto it = container.erase(container.begin());
    
    ASSERT_EQ(container.size(), 2);
    EXPECT_EQ(container[0], 20);
    EXPECT_EQ(container[1], 30);
    AssertEventSequence(received_event_types, {ChangeType::ElementRemoved, ChangeType::SizeChanged});
    ASSERT_NE(it, container.end());
    EXPECT_EQ(*it, 20);
}

TEST_F(ObservableContainerTest, EraseFromMiddle) {
    ObservableContainer<int> container;
    container.push_back(10);
    container.push_back(20);
    container.push_back(30);

    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        received_event_types.push_back(event.type);
    });

    auto it = container.erase(container.begin() + 1);

    ASSERT_EQ(container.size(), 2);
    EXPECT_EQ(container[0], 10);
    EXPECT_EQ(container[1], 30);
    AssertEventSequence(received_event_types, {ChangeType::ElementRemoved, ChangeType::SizeChanged});
    ASSERT_NE(it, container.end());
    EXPECT_EQ(*it, 30);
}

TEST_F(ObservableContainerTest, EraseFromEnd) { 
    ObservableContainer<int> container;
    container.push_back(10);
    container.push_back(20);
    container.push_back(30);

    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        received_event_types.push_back(event.type);
    });

    auto it = container.erase(container.begin() + 2); 

    ASSERT_EQ(container.size(), 2);
    EXPECT_EQ(container[0], 10);
    EXPECT_EQ(container[1], 20);
    AssertEventSequence(received_event_types, {ChangeType::ElementRemoved, ChangeType::SizeChanged});
    EXPECT_EQ(it, container.end()); 
}

TEST_F(ObservableContainerTest, EraseOnlyElement) {
    ObservableContainer<int> container;
    container.push_back(10);

    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        received_event_types.push_back(event.type);
    });
    
    auto it = container.erase(container.begin());

    ASSERT_TRUE(container.empty());
    AssertEventSequence(received_event_types, {ChangeType::ElementRemoved, ChangeType::SizeChanged});
    EXPECT_EQ(it, container.end());
}

TEST_F(ObservableContainerTest, EraseFromEmptyContainer) {
    ObservableContainer<int> container; 
    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        received_event_types.push_back(event.type);
    });

    auto it = container.erase(container.begin()); 

    ASSERT_TRUE(container.empty());
    EXPECT_TRUE(received_event_types.empty()) << "No events should be sent when erasing from empty container.";
    EXPECT_EQ(it, container.end());
}

TEST_F(ObservableContainerTest, EraseStringElements) {
    ObservableContainer<std::string> container;
    container.push_back("alpha");
    container.push_back("beta");
    container.push_back("gamma");

    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event){
        received_event_types.push_back(event.type);
    });

    auto it1 = container.erase(container.begin());
    ASSERT_EQ(container.size(), 2);
    EXPECT_EQ(container[0], "beta");
    EXPECT_EQ(container[1], "gamma");
    AssertEventSequence(received_event_types, {ChangeType::ElementRemoved, ChangeType::SizeChanged});
    ASSERT_NE(it1, container.end());
    EXPECT_EQ(*it1, "beta");
    received_event_types.clear();

    container.push_back("delta"); 
    received_event_types.clear(); 

    auto it2 = container.erase(container.begin() + 1); 
    ASSERT_EQ(container.size(), 2);
    EXPECT_EQ(container[0], "beta");
    EXPECT_EQ(container[1], "delta");
    AssertEventSequence(received_event_types, {ChangeType::ElementRemoved, ChangeType::SizeChanged});
    ASSERT_NE(it2, container.end());
    EXPECT_EQ(*it2, "delta");
    received_event_types.clear();
}

// --- clear() method Tests ---

TEST_F(ObservableContainerTest, ClearNonEmptyContainer) {
    ObservableContainer<int> container;
    container.push_back(10);
    container.push_back(20);
    container.push_back(30);

    bool notified = false;
    std::vector<ChangeType> events;
    container.addObserver([&](const ChangeEvent& ev){ 
        notified = true; 
        events.push_back(ev.type); 
    });
    notified = false; 
    events.clear();

    container.clear();

    EXPECT_TRUE(container.empty());
    EXPECT_EQ(container.size(), 0);
    EXPECT_TRUE(notified);
    AssertEventSequence(events, {ChangeType::SizeChanged});
}

TEST_F(ObservableContainerTest, ClearAlreadyEmptyContainer) {
    ObservableContainer<int> container; 

    bool notified = false;
    std::vector<ChangeType> events;
    container.addObserver([&](const ChangeEvent& ev){ 
        notified = true; 
        events.push_back(ev.type); 
    });
    notified = false;
    events.clear();

    container.clear();

    EXPECT_TRUE(container.empty());
    EXPECT_EQ(container.size(), 0);
    EXPECT_FALSE(notified); 
    EXPECT_TRUE(events.empty());
}

TEST_F(ObservableContainerTest, ClearStringContainer) {
    ObservableContainer<std::string> container;
    container.push_back("one");
    container.push_back("two");

    bool notified = false;
    std::vector<ChangeType> events;
    container.addObserver([&](const ChangeEvent& ev){ 
        notified = true; 
        events.push_back(ev.type); 
    });
    notified = false;
    events.clear();

    container.clear();

    EXPECT_TRUE(container.empty());
    EXPECT_EQ(container.size(), 0);
    EXPECT_TRUE(notified);
    AssertEventSequence(events, {ChangeType::SizeChanged});
}

// --- Batched Updates (beginUpdate, endUpdate) Tests ---

TEST_F(ObservableContainerTest, BatchedUpdatesSingleLevel) {
    ObservableContainer<int> container;
    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event) {
        received_event_types.push_back(event.type);
    });

    container.beginUpdate();
    container.push_back(10); 
    container.push_back(20); 
    container.modify(0, 15); 
    container.pop_back();    
    
    EXPECT_TRUE(received_event_types.empty()) << "No notifications should be sent during batch update.";

    container.endUpdate();

    ASSERT_EQ(container.size(), 1);
    EXPECT_EQ(container[0], 15);
    AssertEventSequence(received_event_types, {ChangeType::BatchUpdate});
}

TEST_F(ObservableContainerTest, BatchedUpdatesNoChanges) {
    ObservableContainer<int> container;
    container.push_back(10); 

    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event) {
        received_event_types.push_back(event.type);
    });
    received_event_types.clear(); 

    container.beginUpdate();
    container.endUpdate();

    EXPECT_TRUE(received_event_types.empty()) << "No BatchUpdate should be sent if no changes occurred.";
}

TEST_F(ObservableContainerTest, BatchedUpdatesNested) {
    ObservableContainer<int> container;
    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event) {
        received_event_types.push_back(event.type);
    });

    container.beginUpdate(); 
    container.push_back(10); 

    container.beginUpdate(); 
    container.push_back(20); 
    container.modify(0, 5);  
    
    EXPECT_TRUE(received_event_types.empty()) << "No events before inner endUpdate.";
    container.endUpdate();   
    
    EXPECT_TRUE(received_event_types.empty()) << "No events after inner endUpdate but before outer endUpdate.";
    
    container.push_back(30); 
    container.endUpdate();   

    ASSERT_EQ(container.size(), 3);
    EXPECT_EQ(container[0], 5);
    EXPECT_EQ(container[1], 20);
    EXPECT_EQ(container[2], 30);
    AssertEventSequence(received_event_types, {ChangeType::BatchUpdate});
}

TEST_F(ObservableContainerTest, EndUpdateWithoutBegin) {
    ObservableContainer<int> container;
    container.push_back(10); 

    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event) {
        received_event_types.push_back(event.type);
    });
    received_event_types.clear(); 

    ASSERT_NO_THROW(container.endUpdate()); 
    EXPECT_TRUE(received_event_types.empty()) << "No events should be sent for endUpdate without begin.";

    container.push_back(20);
    AssertEventSequence(received_event_types, {ChangeType::ElementAdded, ChangeType::SizeChanged});
}

TEST_F(ObservableContainerTest, BatchUpdateTriggeredOnlyIfChangesOccurInScope) {
    ObservableContainer<int> container;
    std::vector<ChangeType> received_event_types;
    container.addObserver([&](const ChangeEvent& event) {
        received_event_types.push_back(event.type);
    });

    container.beginUpdate();
    container.endUpdate();
    EXPECT_TRUE(received_event_types.empty()) << "No BatchUpdate if no changes in scope 1.";

    container.push_back(1); 
    received_event_types.clear();

    container.beginUpdate();
    container.push_back(2); 
    container.endUpdate();
    AssertEventSequence(received_event_types, {ChangeType::BatchUpdate}) << "BatchUpdate expected for scope 2.";
}

// --- Copy Constructor and Copy Assignment Tests ---

TEST_F(ObservableContainerTest, CopyConstructorBasic) {
    ObservableContainer<int> source_container;
    source_container.push_back(10);
    source_container.push_back(20);

    bool source_notified = false;
    source_container.addObserver([&](const ChangeEvent&){ source_notified = true; });

    bool dest_notified = false; 
    ObservableContainer<int> dest_container = source_container;

    ASSERT_EQ(dest_container.size(), 2);
    EXPECT_EQ(dest_container[0], 10);
    EXPECT_EQ(dest_container[1], 20);
    ASSERT_EQ(source_container.size(), 2); 

    EXPECT_FALSE(source_notified);
    EXPECT_FALSE(dest_notified);

    source_notified = false; 
    source_container.push_back(30);
    EXPECT_TRUE(source_notified);
    EXPECT_FALSE(dest_notified);

    source_notified = false; 
    dest_notified = false;   

    dest_container.addObserver([&](const ChangeEvent&){ dest_notified = true; });
    dest_container.push_back(40); 
    
    EXPECT_TRUE(dest_notified);
    EXPECT_FALSE(source_notified); 
}

TEST_F(ObservableContainerTest, CopyConstructorObserversNotCopied) {
    ObservableContainer<int> source_container;
    source_container.push_back(1);

    int source_observer_calls = 0;
    source_container.addObserver([&](const ChangeEvent&){ source_observer_calls++; });
    source_observer_calls = 0; 

    ObservableContainer<int> dest_container = source_container;

    source_container.push_back(2); 
    EXPECT_EQ(source_observer_calls, 2);

    dest_container.push_back(3); 
    EXPECT_EQ(source_observer_calls, 2);

    int dest_observer_calls = 0;
    dest_container.addObserver([&](const ChangeEvent&){ dest_observer_calls++; });
    dest_container.push_back(4); 
    EXPECT_EQ(dest_observer_calls, 2);
    EXPECT_EQ(source_observer_calls, 2); 
}

TEST_F(ObservableContainerTest, CopyAssignmentBasic) {
    ObservableContainer<int> source_container;
    source_container.push_back(100);
    source_container.push_back(200);

    ObservableContainer<int> dest_container;
    dest_container.push_back(1);
    dest_container.push_back(2);

    int source_observer_calls = 0;
    std::vector<ChangeType> dest_events;
    
    source_container.addObserver([&](const ChangeEvent&){ source_observer_calls++; });
    dest_container.addObserver([&](const ChangeEvent& e){ dest_events.push_back(e.type); });
    
    source_observer_calls = 0; 
    dest_events.clear();

    dest_container = source_container;

    ASSERT_EQ(dest_container.size(), 2);
    EXPECT_EQ(dest_container[0], 100);
    EXPECT_EQ(dest_container[1], 200);
    ASSERT_EQ(source_container.size(), 2); 
    EXPECT_EQ(source_observer_calls, 0);   

    AssertEventSequence(dest_events, {ChangeType::BatchUpdate});

    dest_events.clear();
    int new_dest_observer_calls = 0;
    dest_container.addObserver([&](const ChangeEvent& e){ new_dest_observer_calls++; });
    
    dest_container.push_back(300); 
    EXPECT_EQ(new_dest_observer_calls, 2); 
}

TEST_F(ObservableContainerTest, CopyAssignmentClearsDestObservers) {
    ObservableContainer<int> dest_container;
    dest_container.push_back(1); 

    int old_dest_observer_calls = 0;
    dest_container.addObserver([&](const ChangeEvent& e){ 
        old_dest_observer_calls++;
    });
    old_dest_observer_calls = 0; 

    dest_container.push_back(5); 
    EXPECT_EQ(old_dest_observer_calls, 2);
    old_dest_observer_calls = 0; 

    ObservableContainer<int> source_container;
    source_container.push_back(10);
    
    dest_container = source_container; 

    EXPECT_EQ(old_dest_observer_calls, 0);

    dest_container.push_back(20); 
    EXPECT_EQ(old_dest_observer_calls, 0);
}


TEST_F(ObservableContainerTest, CopyAssignmentSelf) {
    ObservableContainer<int> container;
    container.push_back(1);
    container.push_back(2);

    int observer_calls = 0;
    std::vector<ChangeType> events;
    container.addObserver([&](const ChangeEvent& e){ 
        observer_calls++; 
        events.push_back(e.type); 
    });
    observer_calls = 0; 
    events.clear();

    container = container; 

    ASSERT_EQ(container.size(), 2);
    EXPECT_EQ(container[0], 1);
    EXPECT_EQ(container[1], 2);
    EXPECT_EQ(observer_calls, 0);
    EXPECT_TRUE(events.empty());

    container.push_back(3);
    EXPECT_EQ(observer_calls, 2); 
    AssertEventSequence(events, {ChangeType::ElementAdded, ChangeType::SizeChanged});
}

TEST_F(ObservableContainerTest, CopyAssignmentNoChangeNoNotification) {
    ObservableContainer<int> source;
    source.push_back(1);

    ObservableContainer<int> dest;
    dest.push_back(1); 

    std::vector<ChangeType> dest_events;
    dest.addObserver([&](const ChangeEvent& e){ dest_events.push_back(e.type); });
    dest_events.clear(); 

    dest = source; 

    EXPECT_TRUE(dest_events.empty());
}

// --- Move Constructor and Move Assignment Tests ---

TEST_F(ObservableContainerTest, MoveConstructorBasic) {
    ObservableContainer<int> source_container;
    source_container.push_back(10);
    source_container.push_back(20);
    size_t original_source_size = source_container.size();

    bool source_notified = false;
    source_container.addObserver([&](const ChangeEvent&){ source_notified = true; });
    source_notified = false; // Reset after observer setup

    ObservableContainer<int> dest_container = std::move(source_container);

    ASSERT_EQ(dest_container.size(), original_source_size);
    EXPECT_EQ(dest_container[0], 10);
    EXPECT_EQ(dest_container[1], 20);
    EXPECT_TRUE(source_container.empty()); // Source should be empty

    EXPECT_FALSE(source_notified) << "Source should not be notified during move construction.";
    
    bool dest_notified = false;
    dest_container.addObserver([&](const ChangeEvent&){ dest_notified = true; });
    dest_container.push_back(30);
    EXPECT_TRUE(dest_notified) << "Dest observer should be notified after move.";
}

TEST_F(ObservableContainerTest, MoveConstructorObserversNotMoved) {
    ObservableContainer<int> source_container;
    source_container.push_back(1);

    int source_observer_calls = 0;
    source_container.addObserver([&](const ChangeEvent&){ source_observer_calls++; });
    source_observer_calls = 0; // Reset

    ObservableContainer<int> dest_container = std::move(source_container);

    source_container.push_back(2); 
    EXPECT_EQ(source_observer_calls, 0) << "Source observers should not be called after move, even if source is reused.";

    int dest_observer_calls = 0;
    dest_container.addObserver([&](const ChangeEvent&){ dest_observer_calls++; });
    dest_container.push_back(3); // ElementAdded, SizeChanged
    EXPECT_EQ(dest_observer_calls, 2);
}


TEST_F(ObservableContainerTest, MoveAssignmentBasic) {
    ObservableContainer<int> source_container;
    source_container.push_back(100);
    source_container.push_back(200);
    size_t original_source_size = source_container.size();

    ObservableContainer<int> dest_container;
    dest_container.push_back(1);
    dest_container.push_back(2);

    int source_observer_calls = 0;
    std::vector<ChangeType> dest_events;
    
    source_container.addObserver([&](const ChangeEvent&){ source_observer_calls++; });
    dest_container.addObserver([&](const ChangeEvent& e){ dest_events.push_back(e.type); });
    
    source_observer_calls = 0; 
    dest_events.clear();

    dest_container = std::move(source_container);

    ASSERT_EQ(dest_container.size(), original_source_size);
    EXPECT_EQ(dest_container[0], 100);
    EXPECT_EQ(dest_container[1], 200);
    EXPECT_TRUE(source_container.empty()); // Source should be empty
    EXPECT_EQ(source_observer_calls, 0);   // Source observers are gone with the source's state

    AssertEventSequence(dest_events, {ChangeType::BatchUpdate}); // Dest is notified of change

    dest_events.clear();
    int new_dest_observer_calls = 0;
    dest_container.addObserver([&](const ChangeEvent& e){ new_dest_observer_calls++; });
    
    dest_container.push_back(300); 
    EXPECT_EQ(new_dest_observer_calls, 2); 
}

TEST_F(ObservableContainerTest, MoveAssignmentClearsDestObservers) {
    ObservableContainer<int> dest_container;
    dest_container.push_back(1); 

    int old_dest_observer_calls = 0;
    dest_container.addObserver([&](const ChangeEvent&){ 
        old_dest_observer_calls++;
    });
    old_dest_observer_calls = 0; 

    dest_container.push_back(2); 
    EXPECT_EQ(old_dest_observer_calls, 2);
    old_dest_observer_calls = 0; 

    ObservableContainer<int> source_container;
    source_container.push_back(10);
    
    dest_container = std::move(source_container); 

    EXPECT_EQ(old_dest_observer_calls, 0); 

    dest_container.push_back(20); 
    EXPECT_EQ(old_dest_observer_calls, 0);
}

TEST_F(ObservableContainerTest, MoveAssignmentSelf) {
    ObservableContainer<int> container;
    container.push_back(1);
    container.push_back(2);

    int observer_calls = 0;
    std::vector<ChangeType> events;
    container.addObserver([&](const ChangeEvent& e){ 
        observer_calls++; 
        events.push_back(e.type); 
    });
    observer_calls = 0; 
    events.clear();

    container = std::move(container); 

    ASSERT_EQ(container.size(), 2);
    EXPECT_EQ(container[0], 1);
    EXPECT_EQ(container[1], 2);
    EXPECT_EQ(observer_calls, 0);
    EXPECT_TRUE(events.empty());

    container.push_back(3);
    EXPECT_EQ(observer_calls, 2); 
    AssertEventSequence(events, {ChangeType::ElementAdded, ChangeType::SizeChanged});
}

// --- Iterator Tests ---

TEST_F(ObservableContainerTest, IteratorsOnNonEmptyContainer) {
    ObservableContainer<int> container;
    container.push_back(10);
    container.push_back(20);
    container.push_back(30);

    // Non-const iterators
    std::vector<int> elements_found;
    for (auto it = container.begin(); it != container.end(); ++it) {
        elements_found.push_back(*it);
    }
    ASSERT_EQ(elements_found.size(), 3);
    EXPECT_EQ(elements_found[0], 10);
    EXPECT_EQ(elements_found[1], 20);
    EXPECT_EQ(elements_found[2], 30);

    // Test modification through iterator
    *(container.begin()) = 15;
    EXPECT_EQ(container[0], 15);

    // Const iterators (using cbegin/cend)
    elements_found.clear();
    for (auto it = container.cbegin(); it != container.cend(); ++it) {
        elements_found.push_back(*it);
    }
    ASSERT_EQ(elements_found.size(), 3);
    EXPECT_EQ(elements_found[0], 15); // Reflects previous modification
    EXPECT_EQ(elements_found[1], 20);
    EXPECT_EQ(elements_found[2], 30);

    // Const iterators (on const container)
    elements_found.clear();
    const ObservableContainer<int>& const_container = container;
    for (auto it = const_container.begin(); it != const_container.end(); ++it) {
        elements_found.push_back(*it);
    }
    ASSERT_EQ(elements_found.size(), 3);
    EXPECT_EQ(elements_found[0], 15);
    EXPECT_EQ(elements_found[1], 20);
    EXPECT_EQ(elements_found[2], 30);
}

TEST_F(ObservableContainerTest, IteratorsOnEmptyContainer) {
    ObservableContainer<int> empty_container;
    EXPECT_EQ(empty_container.begin(), empty_container.end());
    EXPECT_EQ(empty_container.cbegin(), empty_container.cend());

    const ObservableContainer<int>& const_empty_container = empty_container;
    EXPECT_EQ(const_empty_container.begin(), const_empty_container.end());
}

TEST_F(ObservableContainerTest, IteratorsStringContainer) {
    ObservableContainer<std::string> container;
    container.push_back("first");
    container.push_back("second");

    // Non-const iterators
    std::vector<std::string> elements_found;
    for (auto it = container.begin(); it != container.end(); ++it) {
        elements_found.push_back(*it);
    }
    ASSERT_EQ(elements_found.size(), 2);
    EXPECT_EQ(elements_found[0], "first");
    EXPECT_EQ(elements_found[1], "second");

    // Test modification through iterator
    *(container.begin()) = "new_first";
    EXPECT_EQ(container[0], "new_first");

    // Const iterators (using cbegin/cend)
    elements_found.clear();
    for (auto it = container.cbegin(); it != container.cend(); ++it) {
        elements_found.push_back(*it);
    }
    ASSERT_EQ(elements_found.size(), 2);
    EXPECT_EQ(elements_found[0], "new_first");
    EXPECT_EQ(elements_found[1], "second");

    // Const iterators (on const container)
    elements_found.clear();
    const ObservableContainer<std::string>& const_container = container;
    for (auto it = const_container.begin(); it != const_container.end(); ++it) {
        elements_found.push_back(*it);
    }
    ASSERT_EQ(elements_found.size(), 2);
    EXPECT_EQ(elements_found[0], "new_first");
    EXPECT_EQ(elements_found[1], "second");
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
