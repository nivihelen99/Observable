# Compiler and Flags
CXX = g++
CXXFLAGS = -std=c++17 -pthread
LDFLAGS = -pthread -lgtest -lgtest_main

# Source Files
SOURCES = test_observable_container.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Target Executable
TARGET = test_runner

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all test clean
