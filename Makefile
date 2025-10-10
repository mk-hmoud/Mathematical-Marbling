# ------------------------------------------------
# Generic Makefile
#
# Author: yanick.rochon@gmail.com
# Date  : 2011-08-10
#
# Changelog :
#   2010-11-05 - first version
#   2011-08-10 - added structure : sources, objects, binaries
#                thanks to http://stackoverflow.com/users/128940/beta
# ------------------------------------------------

# project name (generate executable with this name)
TARGET   = marbling

CC       = g++
# compiling flags here
# get from environment settings
CPPFLAGS = $(CPPFLAGS_ENV) -DNDEBUG -DSCTP_NOT_ENABLED -std=c++17

# CFLAGS   = -std=c99 -Wall -I.
#CFLAGS   = -Wall -DDRS_ENABLED -I./include -I../include -I../dcstack/base -I../dsi -g -O0
#CFLAGS   = -Wall -DNDEBUG -DTLS_NOT_ENABLED -I./include -I../include -I../dcstack/base -I../dcstack/cli -I../dcstack/parser -I../dcstack/dc -g -O0
CFLAGS   = $(CPPFLAGS) -Wall -I./include -g -O0 -fstack-protector-all -fsanitize=address -static-libasan
$(info CFLAGS is $(CFLAGS))

#LINKER   = g++ -o
LINKER   = g++
# linking flags here
LFLAGS   = -Wall 
#LIB_DIRS = -L../../lib -L../../lib/openssl/lib
#LIB_DIRS = -L../lib
#LIBS = -lm -ldl -lcrypt -lstdc++ -lssl -lcrypto -lpthread -lscbase -lsccli -lmsrp -ldatabase_driver -lhttp_parser
LIBS := -lasan -lGLEW -lglfw -lGL -lm -lGLU 
    
# change these to set the proper directories where each files shoould be
SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

SOURCES  := $(wildcard $(SRCDIR)/*.cpp)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
rm       = rm -f
MKDIR_P = mkdir -p

# create obj directory if does not exist
#${OBJDIR}:
#	${MKDIR_P} ${OBJDIR}

$(info $(SOURCES))

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(LINKER) $(OBJECTS) $(LFLAGS) -o $@ $(LIB_DIRS) $(LIBS)
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONEY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONEY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"