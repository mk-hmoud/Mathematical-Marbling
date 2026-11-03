TARGET   = libmarbling.so

CC       = g++
CFLAGS   = -Wall -std=c++17 -fPIC -I./src -I../utils/src -g -fsanitize=address -fopenmp
# -MMD -MP emit a .d file of header dependencies per object, so changing a
# header rebuilds every object that includes it (-MP adds phantom targets so a
# deleted header does not break the build).
CFLAGS   += -MMD -MP

LINKER   = g++
LFLAGS   = -shared -fsanitize=address -fopenmp

LIB_DIRS = -L../lib
LIBS     = -lutils

SRCDIR   = src
OBJDIR   = obj
BINDIR   = ../lib

SOURCES  := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEPS     := $(OBJECTS:.o=.d)
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

# Pull in the generated header-dependency files (absent on the first build).
-include $(DEPS)

.PHONY: clean
clean:
	@rm -rf $(OBJDIR)
	@rm -f $(BINDIR)/$(TARGET)
	@echo "Cleanup complete for marbling!"