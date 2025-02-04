# Makefile for Lab 3 - SystemC Project

# Path to SystemC installation
SYSTEMC_HOME ?= /opt/homebrew/include/systemc

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -I$(SYSTEMC_HOME) -std=c++17 -O2 -pthread

# Linker flags
LDFLAGS = -L$(SYSTEMC_HOME)/lib-linux64 -lsystemc -lm -pthread

# Source files
SOURCES = main.cpp tb.cpp npu.cpp sfr.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Executable name
EXEC = npu_simulation

# Default target
all: $(EXEC)

# Link object files to create executable
$(EXEC): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(EXEC) $(OBJECTS) $(LDFLAGS)

# Compile .cpp files to .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJECTS) $(EXEC)

# Phony targets
.PHONY: all clean
