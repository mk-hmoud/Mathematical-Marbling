TARGET   = marbling_app

CC       = g++
CFLAGS   = -Wall -std=c++17 -I./src -I../utils/src -I../marbling/src -I../rendering/src

LINKER   = g++
LFLAGS   = -Wl,-rpath,'$$ORIGIN/../../lib'

LIB_DIRS = -L../lib
LIBS     = -lutils -lmarbling -lrenderer -lGLEW -lglfw -lGL -lm

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

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
	@echo "Cleanup complete for application!"