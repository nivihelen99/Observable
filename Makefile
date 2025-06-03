# Compiler and Flags
CXX = g++
CXXFLAGS = -std=c++17 -pthread
LDFLAGS = -pthread -lgtest # Removed -lgtest_main

# Compiler and Flags
# CXX = g++ # Already defined
# CXXFLAGS = -std=c++17 -pthread # Already defined
# LDFLAGS = -pthread -lgtest # Already defined (this is for test_runner)
LDFLAGS_APP = -pthread # For main_app, no gtest needed

# Test Sources & Objects
TEST_SOURCES = test_observable_container.cpp
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)
TEST_TARGET = test_runner

# Main App Sources & Objects
APP_SOURCES = main.cpp
APP_OBJECTS = $(APP_SOURCES:.cpp=.o) # main.o
APP_TARGET = main_app

# Default target - build both
all: $(TEST_TARGET) $(APP_TARGET)

# Rule for test_runner
$(TEST_TARGET): $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) $(TEST_OBJECTS) -o $(TEST_TARGET) $(LDFLAGS)

# Rule for main_app
$(APP_TARGET): $(APP_OBJECTS)
	$(CXX) $(CXXFLAGS) $(APP_OBJECTS) -o $(APP_TARGET) $(LDFLAGS_APP)

# Generic rule for .o files (compiles .cpp to .o)
# This will be used for test_observable_container.cpp and main.cpp
%.o: %.cpp ObservableContainer.h ChangeEvent.h ScopedModifier.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TEST_OBJECTS) $(TEST_TARGET) $(APP_OBJECTS) $(APP_TARGET)

.PHONY: all test clean
