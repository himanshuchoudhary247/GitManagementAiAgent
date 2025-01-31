# Makefile for SystemC Simulation

# SystemC installation directory
SYSTEMC_HOME ?= /opt/homebrew

# Compiler
CXX = g++
CXXFLAGS = -I$(SYSTEMC_HOME)/include -std=c++17 -O2 -Wall -Wextra

# Linker flags
LDFLAGS = -L$(SYSTEMC_HOME)/lib -lsystemc -lm -pthread

# Source files
SRCS = ./main.cpp ./sfr.cpp ./npu.cpp ./tb.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable name
EXEC = simulation

all: $(EXEC)

$(EXEC): $(OBJS)
	@echo "Linking: $@"
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	rm -f $(OBJS) $(EXEC)
