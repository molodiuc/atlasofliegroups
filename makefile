# See the file INSTALL for detailed instructions

# the following line avoids trouble on some systems (GNU make does this anyway)
SHELL = /bin/sh
INSTALL = /usr/bin/install

# You may edit the INSTALLDIR and BINDIR variables

# INSTALLDIR is where the executables atlas, realex and the messages directory
# will be moved after successfull compilation

# BINDIR is where a symbolic link or shell script 'atlas' calling to the
# executable, and a symbolic link 'realex' will be placed; provided this
# directory is in the search path, you can then execute unsing atlas, realex

# In a single-user situation, you might want something like this:
#   INSTALLDIR := /home/fokko/myatlas
#   BINDIR     := /home/fokko/bin

# In a multi-user situation, you might want this (requires root):
#   INSTALLDIR := /usr/local/atlas
#   BINDIR     := /usr/local/bin

# The default: use the current directory, binaries in parent subdirectory bin
INSTALLDIR := $(shell pwd)
BINDIR := $(INSTALLDIR)/../bin


#Don't edit below this line, with the possible exception of rl_libs
###############################

version = $(shell perl getversion.pl)
messagedir := $(INSTALLDIR)/messages/
cweb_dir = cweb-x3.51

# atlas_dirs contains subdirectories of 'atlas/sources' that need compilation
# realex_dirs are where the object files for realex are situated
atlas_dirs := utilities error structure gkmod io interface test
realex_dirs := utilities structure gkmod error io interpreter

# atlas_sources contains a list of the source files (i.e., the .cpp files)
atlas_sources := $(wildcard $(atlas_dirs:%=sources/%/*.cpp))

# there is one .o file for each .cpp file, in the same directory
atlas_objects := $(atlas_sources:%.cpp=%.o)

# headers are searched in the all directories containing source files
includedirs := $(addprefix -Isources/,$(atlas_dirs))
realex_includes := $(addprefix -Isources/,$(realex_dirs))

# for the interpreter (realex sans the atlas library) sources are *.w files
interpreter_cwebs := $(wildcard sources/interpreter/*.w)
interpreter_objects := $(interpreter_cwebs:%.w=%.o)
interpreter_made_files := $(interpreter_cwebs:%.w=%.cpp) \
    $(filter-out %main.h,$(interpreter_cwebs:%.w=%.h))
# the following variable is a list of patterns, not files!
non_realex_objects := sources/io/interactive%.o \
    sources/interface/%.o sources/test/%.o \
    sources/io/poset.o sources/utilities/abelian.o sources/gkmod/kgp.o
realex_objects := $(interpreter_objects) sources/interpreter/parser.tab.o \
    $(filter-out $(non_realex_objects),$(atlas_objects))

objects := $(atlas_objects) $(interpreter_objects)

# the variable 'dependencies' groups the names of the files, generated by a
# preliminary run of the compiler, recording dependencies of object files
dependencies := $(atlas_objects:%.o=%.d)


# compiler flags
# whenever changing the setting of these flags, do 'make clean' first

# the following are predefined flavors for the compiler flags:
# optimizing (oflags), development with debugging (gflags), profiling (pflags)

oflags := -c $(includedirs) -Wall -O3 -DNDEBUG
gflags := -c $(includedirs) -Wall -ggdb
pflags := -c $(includedirs) -Wall -pg -O -DNREADLINE

# create flags specific for atlas and realex
atlas_flags :=
realex_flags := -Wno-parentheses

# the default setting is optimizing
cflags ?= $(oflags)

# to select another flavor, set debug=true or profile=true when calling make
# alternatively, you can set cflags="your personal flavor" to override default

ifeq ($(debug),true)
      cflags := $(gflags)
else
  ifeq ($(profile),true)
      cflags := $(pflags)
  endif
endif

# suppress readline by setting readline=false
# and/or make more verbose by setting verbose=true

ifeq ($(readline),false)
    cflags += -DNREADLINE
    rl_libs =
else
    rl_libs ?= -lreadline

# to override this, either define and export a shell variable 'rl_libs'
# or set LDFLAGS when calling make. For instance for readline on the Mac do:
# $ make LDFLAGS="-lreadline.5 -lcurses"
endif

ifeq ($(verbose),true)
    atlas_flags += -DVERBOSE
endif

# the default compiler
CXX = g++

# give compiler=icc argument to make to use the intel compiler
ifdef compiler
    CXX = $(compiler)
endif

LDFLAGS := $(rl_libs)



# RULES follow below

# This target causes failed actions to clean up their (corrupted) target
.DELETE_ON_ERROR:

# we use no suffix rules
.SUFFIXES:

.PHONY: all install distribution

# The default target is 'all', which builds the executable 'atlas', and 'realex'
all: atlas realex

# the following dependency forces emptymode.cpp to be recompiled whenever any
# of the object files changes; this guarantees that the date in the version
# string it prints will be that of the last recompilation.
sources/interface/emptymode.o: $(atlas_sources)

# For profiling not only 'cflags' used in compiling is modified, but linking
# also is different
atlas: $(atlas_objects)
ifeq ($(profile),true)
	$(CXX) -pg -o atlas $(atlas_objects) $(LDFLAGS)
else
	$(CXX) -o atlas $(atlas_objects) $(LDFLAGS)
endif


realex: $(cweb_dir)/ctanglex $(realex_objects)
	$(CXX) -o realex $(realex_includes) $(realex_objects) $(LDFLAGS)

# The following rules are static pattern rules: they are like implicit rules,
# but only apply to the files listed in their targets

$(filter-out sources/interface/io.o,$(atlas_objects)) : %.o : %.cpp
	$(CXX) $(cflags) $(atlas_flags) -o $*.o $*.cpp
sources/interface/io.o : sources/interface/io.cpp
	$(CXX) $(cflags) $(atlas_flags) -DMESSAGE_DIR_MACRO=\"$(messagedir)\" \
	  -o sources/interface/io.o sources/interface/io.cpp
$(interpreter_objects) : %.o : %.cpp
	$(CXX) $(cflags) $(realex_flags) -o $*.o $*.cpp

sources/interpreter/parser.tab.o: \
  sources/interpreter/parser.tab.c sources/interpreter/parsetree.h
	$(CC) -c -g -Wall $(CFLAGS) $(CPPFLAGS) -o $@ $<

# generate files that describe which .o files depend on which other (.h) files
$(dependencies) : %.d : %.cpp
	$(CXX) $(includedirs) -MM -MF $@ -MT "$*.o $*.d" $*.cpp

# now include all the files constructed by the previous rule
# this defines the dependencies, found by the preprocessor scanning #include
# directives, for all object files of the atlas
# make will automatically remake any of $(dependencies) if necessary

include $(dependencies)

include sources/interpreter/dependencies
# if these dependencies indicate that cwebx must be used, do recursive make
$(interpreter_made_files):
	cd sources/interpreter && $(MAKE) headers cppfiles
sources/interpreter/parser.tab.c sources/interpreter/parser.tab.h: \
  sources/interpreter/parser.y
	cd sources/interpreter && $(MAKE) parser.tab.c

install: atlas realex
ifneq ($(INSTALLDIR),$(shell pwd))
	@echo "Installing directories and files in $(INSTALLDIR)"
	$(INSTALL) -d $(INSTALLDIR)/www
	$(INSTALL) -d $(INSTALLDIR)/messages $(INSTALLDIR)/rx-scripts
	$(INSTALL) -m 644 LICENSE COPYRIGHT README $(INSTALLDIR)
	$(INSTALL) sources/interpreter/*.help $(INSTALLDIR)
	$(INSTALL) -p atlas realex $(INSTALLDIR)
	$(INSTALL) -m 644 www/*html $(INSTALLDIR)/www/
	$(INSTALL) -m 644 messages/*.help $(INSTALLDIR)/messages/
	$(INSTALL) -m 644 messages/*intro_mess $(INSTALLDIR)/messages/
	$(INSTALL) -m 644 rx-scripts/* $(INSTALLDIR)/rx-scripts/
endif
ifneq ($(BINDIR),$(INSTALLDIR))
	mkdir -p $(BINDIR)
	@if test -h $(BINDIR)/atlas; then rm -f $(BINDIR)/atlas; fi
	@if test -h $(BINDIR)/realex; then rm -f $(BINDIR)/realex; fi
ifeq ($(INSTALLDIR),$(shell pwd))
	ln -s $(INSTALLDIR)/atlas $(BINDIR)/atlas # make symbolic link
else
	echo "#!/bin/sh\n$(INSTALLDIR)/atlas $(INSTALLDIR)/messages/" \
	 >$(BINDIR)/atlas; chmod a+x $(BINDIR)/atlas
endif
	ln -s $(INSTALLDIR)/realex $(BINDIR)/realex
endif

version:
	@echo $(version)

distribution:
	bash make_distribution.sh $(version)

.PHONY: mostlyclean clean cleanall show
mostlyclean:
	rm -f $(objects) *~ *.out junk

clean: mostlyclean
	rm -f atlas realex

cleanall: clean
	rm -f $(dependencies)

$(cweb_dir)/ctanglex: \
  $(cweb_dir)/common.h $(cweb_dir)/ctangle.c $(cweb_dir)/ctangle.c
	cd $(cweb_dir) && $(MAKE) ctanglex
