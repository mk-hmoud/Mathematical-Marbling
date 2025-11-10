TARGET   = librenderer.so

CC       = g++
CFLAGS   = -Wall -std=c++17 -fPIC -I./src -I../utils/src
CFLAGS   += -I../external/glm-1.0.2 -I../external/earcut
CFLAGS   += -g -fsanitize=address

LINKER   = g++
LFLAGS   = -shared -fsanitize=address

LIB_DIRS = -L../lib
LIBS     = -lutils -lGL

SRCDIR   = src
OBJDIR   = obj
BINDIR   = ../lib

SOURCES  := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
MKDIR_P  = mkdir -p

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(MKDIR_P) $(BINDIR)
	$(LINKER) $(LFLAGS) -o $@ $(OBJECTS) $(LIB_DIRS) $(LIBS)
	@echo "Linking complete: Created $(TARGET)"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@$(MKDIR_P) $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@rm -rf $(OBJDIR)
	@rm -f $(BINDIR)/$(TARGET)
	@echo "Cleanup complete for rendering!"