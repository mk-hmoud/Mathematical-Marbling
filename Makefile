# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17
LDFLAGS = -lGLEW -lglfw -lGL -lm -lGLU

# Directories
SRC_DIR = Source
OBJ_DIR = Build
BIN_DIR = Bin

# Source files and output
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
TARGET = $(BIN_DIR)/opengl

# Build target
$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: clean
