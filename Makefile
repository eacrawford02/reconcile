.POSIX: # Written for IEEE Std 1003.1-2024 compliancy

# Directory/file names output by the build process. Note that both the
# dependency and object file directories will inherit the same nested directory
# structure of $(SRCDIR). Also note that the following macro defintions should
# technically be immediate-expansion (::=) under 1003.1-2024; however, FreeBSD
# only supports the 1003.2 standard so the definitions are left as
# delayed-expansion for compatibility with that platform
SRCDIR = src
OBJDIR = build # DO NOT SET TO obj. This directory name has a special meaning in
	       # FreeBSD make
DEPDIR = .deps
BIN = reconcile
DEBUGBIN = $(BIN).debug
SAMPLECONF = sample_config.toml
CONF = config.toml # Installed config file name
PREFIX = @prefix@
BINDIR = $(PREFIX)/bin
SAMPLECONFDIR = $(PREFIX)/etc
CONFDIR = .config/$(BIN)

# Clang compiler flags/defines (again, should be immediate-expansion)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP)
CXXFLAGS = -std=c++20
DEBUGFLAGS = -g -O0
PRODDEFS = -DCONF=\"$(CONFDIR)/$(CONF)\" \
	   -DSAMPLE_CONF=\"$(SAMPLECONFDIR)/$(SAMPLECONF)\"
DEBUGDEFS = -DDEBUG -DCONF=\"$(SAMPLECONF)\"
LDLIBS = -lform -lncurses

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

# Generate a temporary filepath to signify enabling debug mode. When the user
# makes the debug target, we force the creation of a file at that path before
# building the application binary with the .WAIT prerequisite. The rest of the
# build process can then check the existence of that file to determine whether
# the current target is a production or debuggable binary. This is a workaround
# for the inability to reassign variables in a recipe (e.g., initializing a
# debug variable to false and then reassigning it to true in the debug recipe)
# and effectively mimics GNU Make's target-specific variables
DEBUG != mktemp --dry-run -t enable_debug

debug: $(DEBUG) .WAIT all
	@rm $(DEBUG)

$(DEBUG): ; @touch $@

install:
	-@bin_count=$$(ls -a1 | grep --count $(BIN)); \
	if [ "$$bin_count" -eq 1 ]; then \
	  bin="$$(ls -a1 | grep $(BIN))"; \
	  if [ "$$(echo "$$bin" | grep $(DEBUGBIN))" ]; then \
	    echo "Warning: Installing $$bin in $(BINDIR)"; \
	  else \
	    install -d $(BINDIR) $(SAMPLECONFDIR); \
	    echo "Installing $$bin in $(BINDIR)"; \
	    install $$bin $(BINDIR)/; \
	    echo "Installing $(SAMPLECONF) in $(SAMPLECONFDIR)"; \
	    install $(SAMPLECONF) $(SAMPLECONFDIR)/; \
	  fi; \
	elif [ "$$bin_count" -gt 1 ]; then \
	  echo "Error: Multiple binaries present in working directory"; \
	else \
	  echo "Error: No binary present in working directory"; \
	fi

$(BIN): $(OBJ)
	# Invoke linker
	@if [ -e "$(DEBUG)" ]; then \
	  echo "c++ -o $(DEBUGBIN) $(OBJ) $(LDLIBS)"; \
	  c++ -o $(DEBUGBIN) $(OBJ) $(LDLIBS); \
	else \
	  echo "c++ -o $(BIN) $(OBJ) $(LDLIBS)"; \
	  c++ -o $(BIN) $(OBJ) $(LDLIBS); \
	fi

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
	@if [ -e "$(DEBUG)" ]; then \
	  debug_opts="$(DEBUGFLAGS) $(DEBUGDEFS)"; \
	  echo "c++ $(DEPFLAGS) $(CXXFLAGS) $$debug_opts -o $@ -c $(SRC)"; \
	  c++ $(DEPFLAGS) $(CXXFLAGS) $$debug_opts -o $@ -c $(SRC); \
	else \
	  echo "c++ $(DEPFLAGS) $(CXXFLAGS) $(PRODDEFS) -o $@ -c $(SRC)"; \
	  c++ $(DEPFLAGS) $(CXXFLAGS) $(PRODDEFS) -o $@ -c $(SRC); \
	fi

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
