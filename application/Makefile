TARGET   = marbling_app

CC       = g++
CFLAGS   = -Wall -std=c++17 -I./src -I../utils/src -I../marbling/src -I../rendering/src
CFLAGS   += -I../external/imgui -I../external/imgui/backends
CFLAGS   += -I../external/glm-1.0.2 -I../external/earcut
CFLAGS   += -g -fsanitize=address
CFLAGS   += -DIMGUI_HAS_DOCKING -DIMGUI_HAS_VIEWPORT
# -MMD -MP emit a .d file of header dependencies per object, so changing a
# header rebuilds every object that includes it (-MP adds phantom targets so a
# deleted header does not break the build).
CFLAGS   += -MMD -MP

LINKER   = g++
LFLAGS   = -Wl,-rpath,'$$ORIGIN/../../lib' -fsanitize=address

LIB_DIRS = -L../lib
LIBS     = -lutils -lmarbling -lrenderer -lGLEW -lglfw -lGL -lX11 -lm

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

SOURCES  := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
MKDIR_P  = mkdir -p

IMGUI_OBJECTS := $(OBJDIR)/imgui.o \
                 $(OBJDIR)/imgui_draw.o \
                 $(OBJDIR)/imgui_tables.o \
                 $(OBJDIR)/imgui_widgets.o \
                 $(OBJDIR)/imgui_impl_glfw.o \
                 $(OBJDIR)/imgui_impl_opengl3.o

DEPS := $(OBJECTS:.o=.d) $(IMGUI_OBJECTS:.o=.d)

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS) $(IMGUI_OBJECTS)
	@$(MKDIR_P) $(BINDIR)
	$(LINKER) $(LFLAGS) -o $@ $(OBJECTS) $(IMGUI_OBJECTS) $(LIB_DIRS) $(LIBS)
	@echo "Linking complete: Created $(TARGET)"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@$(MKDIR_P) $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

$(OBJDIR)/imgui.o: ../external/imgui/imgui.cpp
	@$(MKDIR_P) $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

$(OBJDIR)/imgui_draw.o: ../external/imgui/imgui_draw.cpp
	@$(MKDIR_P) $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

$(OBJDIR)/imgui_tables.o: ../external/imgui/imgui_tables.cpp
	@$(MKDIR_P) $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

$(OBJDIR)/imgui_widgets.o: ../external/imgui/imgui_widgets.cpp
	@$(MKDIR_P) $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

$(OBJDIR)/imgui_impl_glfw.o: ../external/imgui/backends/imgui_impl_glfw.cpp
	@$(MKDIR_P) $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

$(OBJDIR)/imgui_impl_opengl3.o: ../external/imgui/backends/imgui_impl_opengl3.cpp
	@$(MKDIR_P) $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

# Pull in the generated header-dependency files (absent on the first build).
-include $(DEPS)

.PHONY: clean
clean:
	@rm -rf $(OBJDIR)
	@rm -f $(BINDIR)/$(TARGET)
	@echo "Cleanup complete for application!"