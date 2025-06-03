#include "gtest/gtest.h"
#include "ObservableContainer.h" // Now uses the new interface
#include "ChangeEvent.h"         // Now uses the new interface
#include <string>                // Required for std::string
#include <vector>                // Required for std::vector
#include <functional>            // Required for std::function
#include <iterator>              // Required for std::distance, std::begin, std::end
#include <utility>               // Required for std::move
#include <algorithm>             // Required for std::equal

#include <list> // Required for std::list

// Helper to extract value_type from ObservableContainer specialization
template <typename OC_Type> struct GetValueTypeHelper;
template <typename T_val, template<typename,typename> class Cont_val, typename Alloc_val>
struct GetValueTypeHelper<ObservableContainer<T_val, Cont_val, Alloc_val>> {
    using type = T_val;
};

// Test fixture for typed tests
template <typename TypeParam_FullContainer> // Renamed to avoid conflict with gtest's TypeParam
class ObservableContainerTest : public ::testing::Test {
public: // GTest requires public members for fixture
    using FullContainerType = TypeParam_FullContainer;
    using T = typename GetValueTypeHelper<FullContainerType>::type;

protected:
    ObservableContainerTest() {}
    ~ObservableContainerTest() override {}

    // Helper to convert event types to string for better error messages
    static std::string EventTypeToString(const ChangeEvent<T>& event) {
        switch (event.type) {
            case ChangeType::ElementAdded: return "ElementAdded";
            case ChangeType::ElementRemoved: return "ElementRemoved";
            case ChangeType::ElementModified: return "ElementModified";
            case ChangeType::SizeChanged: return "SizeChanged";
            case ChangeType::BatchUpdate: return "BatchUpdate";
            default: return "Unknown";
        }
    }

    // Helper to compare expected and actual event sequences (checks types only)
    static void AssertEventSequenceTypes(const std::vector<ChangeEvent<T>>& actual_events, const std::vector<ChangeType>& expected_types) {
        ASSERT_EQ(actual_events.size(), expected_types.size()) << "Event sequence length mismatch.";
        for (size_t i = 0; i < actual_events.size(); ++i) {
            if (actual_events[i].type != expected_types[i]) {
                std::string actual_str, expected_str;
                for(size_t j=0; j<actual_events.size(); ++j) actual_str += EventTypeToString(actual_events[j]) + (j == actual_events.size()-1 ? "" : ", ");
                for(size_t j=0; j<expected_types.size(); ++j) expected_str += EventTypeToString(ChangeEvent<T>(expected_types[j])) + (j == expected_types.size()-1 ? "" : ", ");
                GTEST_FAIL() << "Event type mismatch at index " << i
                             << ". Expected sequence: [" << expected_str
                             << "], Got sequence: [" << actual_str << "]";
            }
        }
    }
};

// Define the types to be tested
using MyTypes = ::testing::Types<
    ObservableContainer<int, std::vector>, 
    ObservableContainer<int, std::list>,
    ObservableContainer<std::string, std::vector>,
    ObservableContainer<std::string, std::list>
>;
TYPED_TEST_SUITE(ObservableContainerTest, MyTypes);


// Test case for default construction
TYPED_TEST(ObservableContainerTest, DefaultConstructionIsEmpty) {
    typename TestFixture::FullContainerType container; // Use FullContainerType from fixture
    ASSERT_TRUE(container.empty());
    ASSERT_EQ(0, container.size());
}

// Test case for push_back
TYPED_TEST(ObservableContainerTest, PushBackObservesCorrectEvents) {
    using T = typename TestFixture::T; // Use T from fixture
    typename TestFixture::FullContainerType container;
    std::vector<ChangeEvent<T>> received_events;

    container.addObserver([&](const ChangeEvent<T>& event) {
        received_events.push_back(event);
    });

    T val1;
    T val2;
    if constexpr (std::is_same_v<T, int>) {
        val1 = 10;
        val2 = 20;
    } else if constexpr (std::is_same_v<T, std::string>) {
        val1 = "hello";
        val2 = "world";
    }
    // Note: Add static_assert(false, "Unsupported type") for non-test types if necessary

    container.push_back(val1);
    EXPECT_EQ(container.size(), 1);
    ASSERT_FALSE(container.empty());
    EXPECT_EQ(container.at(0), val1); // Changed from front() to at(0) for consistency if available
    this->AssertEventSequenceTypes(received_events, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    ASSERT_EQ(received_events.size(), 2);
    EXPECT_EQ(received_events[0].type, ChangeType::ElementAdded);
    ASSERT_TRUE(received_events[0].index.has_value());
    EXPECT_EQ(received_events[0].index.value(), 0);
    ASSERT_TRUE(received_events[0].newValue.has_value());
    EXPECT_EQ(received_events[0].newValue.value(), val1);
    // Check newSize for SizeChanged event (event at index 1)
    if (received_events.size() == 2 && received_events[1].type == ChangeType::SizeChanged) {
        ASSERT_TRUE(received_events[1].newSize.has_value());
        EXPECT_EQ(received_events[1].newSize.value(), container.size());
    }
    received_events.clear();

    container.push_back(val2);
    EXPECT_EQ(container.size(), 2);
    EXPECT_EQ(container.at(container.size()-1), val2); // Changed from back() to at()
    this->AssertEventSequenceTypes(received_events, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    ASSERT_EQ(received_events.size(), 2);
    EXPECT_EQ(received_events[0].type, ChangeType::ElementAdded);
    ASSERT_TRUE(received_events[0].index.has_value());
    EXPECT_EQ(received_events[0].index.value(), 1);
    ASSERT_TRUE(received_events[0].newValue.has_value());
    EXPECT_EQ(received_events[0].newValue.value(), val2);
    // Check newSize for SizeChanged event (event at index 1)
    if (received_events.size() == 2 && received_events[1].type == ChangeType::SizeChanged) {
        ASSERT_TRUE(received_events[1].newSize.has_value());
        EXPECT_EQ(received_events[1].newSize.value(), container.size());
    }
    received_events.clear();
}

TYPED_TEST(ObservableContainerTest, PushBackRvalueObservesCorrectEvents) {
    using T = typename TestFixture::T;
    typename TestFixture::FullContainerType container;
    std::vector<ChangeEvent<T>> received_events;

    container.addObserver([&](const ChangeEvent<T>& event) {
        received_events.push_back(event);
    });

    T val1_orig;
    if constexpr (std::is_same_v<T, int>) {
        val1_orig = 10;
    } else if constexpr (std::is_same_v<T, std::string>) {
        val1_orig = "movable_string";
    }

    T val1_to_move = val1_orig; // Copy for moving

    container.push_back(std::move(val1_to_move));
    EXPECT_EQ(container.size(), 1);
    ASSERT_FALSE(container.empty());
    EXPECT_EQ(container.at(0), val1_orig);

    this->AssertEventSequenceTypes(received_events, {ChangeType::ElementAdded, ChangeType::SizeChanged});
    ASSERT_EQ(received_events.size(), 2);
    EXPECT_EQ(received_events[0].type, ChangeType::ElementAdded);
    ASSERT_TRUE(received_events[0].index.has_value());
    EXPECT_EQ(received_events[0].index.value(), 0);
    ASSERT_TRUE(received_events[0].newValue.has_value());
    EXPECT_EQ(received_events[0].newValue.value(), val1_orig); // Check the value is correct
    // Check newSize for SizeChanged event (event at index 1)
    if (received_events.size() == 2 && received_events[1].type == ChangeType::SizeChanged) {
        ASSERT_TRUE(received_events[1].newSize.has_value());
        EXPECT_EQ(received_events[1].newSize.value(), container.size());
    }

    // For std::string, check if val1_to_move is in a valid moved-from state.
    // The C++ standard states that a moved-from std::string is in a "valid but unspecified state".
    // A common outcome is an empty string, but this is not guaranteed.
    // Checking that it's not equal to its original state (if it was non-empty) or that it is empty
    // are reasonable checks, but the most important is that the container holds the correct value.
    if constexpr (std::is_same_v<T, std::string>) {
        // Example check: EXPECT_TRUE(val1_to_move.empty()); // This might be too strict.
        // It's enough that container.at(0) has the correct value.
    }
    received_events.clear();
}

// ... (ALL OTHER TESTS COMMENTED OUT FOR THIS STEP) ...



int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
