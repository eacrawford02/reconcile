.POSIX: # Written for IEEE Std 1003.1-2024 compliancy

# Note that both the dependency and object file directories will inherit the
# same nested directory structure of $(SRCDIR). Also note that the following
# macro defintions should technically be immediate-expansion (::=) under
# 1003.1-2024; however, FreeBSD only supports the 1003.2 standard so the
# definitions are left as delayed-expansion for compatibility with that platform
SRCDIR = src
# DO NOT SET TO obj. This directory name has a special meaning in FreeBSD make
OBJDIR = build
DEPDIR = .deps
BIN = debug

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
CXXFLAGS = -std=c++20 -g -O0
LDLIBS = -lncurses

# Create list of object file targets
OBJ != find $(SRCDIR) -name "*.cpp" \
       | sed -e "s/$(SRCDIR)/$(OBJDIR)/g" -e "s/\.cpp/\.o/g"

# Declare the dependency directory as a prerequisite of our default target,
# build, so that it will be created when needed (e.g., first build). This is an
# alternative to declaring DEPDIR as an order-only prerequisite of each object
# target (a GNU make feature). Because the timestamp of DEPDIR will be updated
# whenever a dependency file is added to it, we don't want to include it a
# prerequisite for the object targets, otherwise generating a single dependency
# file would cause every other object target to be out of date. Thus, we write
# our default target such that the DEPDIR prerequisite is processed first
all: $(DEPDIR) $(BIN)

$(BIN): $(OBJ)
	c++ -o $(BIN) $(OBJ) $(LDLIBS)

clean:
	-rm -rf .deps
	-rm -rf $(OBJDIR)/*.o
	-rm $(BIN)

SRC = $(@:$(OBJDIR)%.o=$(SRCDIR)%.cpp)
DEP = $(@:$(OBJDIR)%.o=$(DEPDIR)%.d)

# Common auto-dependency makefiles will use an inference rule to generate the
# object files. make does not allow slashes in the targets of inference rules,
# so in order to place the object files in a subdirectory we must instead use a
# regular target rule
$(OBJ): $(SRC) $(DEP)
	c++ $(DEPFLAGS) $(CXXFLAGS) -o $@ -c $(SRC)

$(DEPDIR): ; mkdir $@

# Should technically be immediate-expansion (::=) under 1003.1
DEPFILES = $(OBJ:$(OBJDIR)%.o=$(DEPDIR)%.d)

# Declare list of dependency files as empty targets so that if make is run and a
# a dependency file associated with a given source file doesn't exist (e.g.,
# first build, dependency file deleted), then the dependency target will be
# marked out-of-date. This will force the corresponding object target to be
# rebuilt and the pre-processor will generate the needed dependency file
$(DEPFILES):

-include $(DEPFILES)
