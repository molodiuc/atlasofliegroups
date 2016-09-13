# See the file INSTALL for detailed instructions

# the following line avoids trouble on some systems (GNU make does this anyway)
SHELL = /bin/sh
INSTALL = /usr/bin/install

# You may edit the INSTALLDIR and BINDIR variables

# INSTALLDIR is where the executables Fokko, atlas and the messages directory
# will be moved after successfull compilation

# BINDIR is where a symbolic link to the 'Fokko' executable will be created,
# and shell script executing 'atlas' will be placed; provided this directory
# is in the search path, you can then call Fokko, atlas from anywhere

# However when BINDIR is INSTALLDIR one cannot have those shell scripts since
# they would be at the same place as the executable to call; since both default
# to the current directory, you are advised to set at least BINDIR explicitly.
# INSTALLDIR only needs to be changed if the current directory is temporary

# In a single-user situation, you might want something like this:
#   INSTALLDIR := /home/ducloux/atlas
#   BINDIR     := /home/ducloux/bin

# In a multi-user situation, you might want this (requires root privilege):
#   INSTALLDIR := /usr/local/atlas
#   BINDIR     := /usr/local/bin

# The default: use current directoty for both
INSTALLDIR := $(shell pwd)
BINDIR := $(shell pwd)

version := $(shell ./getversion.pl)

messagedir := $(INSTALLDIR)/messages/
cweb_dir := cwebx
sources_dir := sources
atlas_dir := sources/interpreter

# Fokko_dirs contains subdirectories of 'atlas/sources' needed for 'Fokko'
# atlas_dirs are where the object files for 'atlas' are situated
Fokko_dirs  := utilities structure gkmod io error interface test
atlas_dirs := utilities structure gkmod io error interpreter

# Fokko_sources contains a list of the source files (i.e., the .cpp files)
Fokko_sources := $(wildcard $(Fokko_dirs:%=sources/%/*.cpp))

# there is one .o file for each .cpp file, in the same directory
Fokko_objects := $(Fokko_sources:%.cpp=%.o)

# headers are searched in the all directories containing source files
Fokko_includes := $(addprefix -Isources/,$(Fokko_dirs))
atlas_includes := $(addprefix -Isources/,$(atlas_dirs))

# for the interpreter (atlas sans the Atlas library) sources are *.w files
interpreter_cwebs := $(wildcard sources/interpreter/*.w)
interpreter_cweb_objects := $(interpreter_cwebs:%.w=%.o)
interpreter_objects := $(interpreter_cweb_objects) \
    sources/interpreter/parser.tab.o
interpreter_made_files := $(interpreter_cwebs:%.w=%.cpp) \
    $(filter-out %main.h,$(interpreter_cwebs:%.w=%.h)) \
    sources/interpreter/parse_types.h \
    sources/interpreter/parser.tab.h sources/interpreter/parser.tab.c
# the following variable is a list of patterns, not files!
non_atlas_objects := sources/io/interactive%.o \
    sources/interface/%.o sources/test/%.o \
    sources/io/poset.o sources/utilities/abelian.o sources/gkmod/kgp.o
atlas_objects := $(interpreter_objects) \
    $(filter-out $(non_atlas_objects),$(Fokko_objects))

objects := $(Fokko_objects) $(interpreter_objects)

# the variable 'dependencies' groups the names of the files, generated by a
# preliminary run of the compiler, recording dependencies of object files
dependencies := $(Fokko_objects:%.o=%.d)


# compiler flags
# whenever changing the setting of these flags, do 'make clean' first

# the following are predefined flavors for the compiler flags:
# normal (nflags)
# optimizing (oflags)
# development with debugging (gflags)
# profiling (pflags)
nflags := -Wall -DNDEBUG
oflags := -Wall -O3 -DNDEBUG
gflags := -Wall -ggdb
pflags := -Wall -pg -O -DNREADLINE


# these flags are necessary for compilation, the -c should not be altered
CXXFLAGS  = -c $(CXXFLAVOR)

# additional flags specific for Fokko (atlas specifics are in its own Makefile)
Fokko_flags = $(Fokko_includes)

# to select another flavor, set optimize=true, debug=true or profile=true
# when calling make (as in "make optimize=true") or set CXXFLAVOR as an
# environment variable (only the first flavor set in above list takes effect)
# alternatively, you can do "make CXXFLAVOR='explicit options'" to specify

ifeq ($(optimize),true)
      CXXFLAVOR := $(oflags)
else
ifeq ($(debug),true)
      CXXFLAVOR := $(gflags)
else
ifeq ($(profile),true)
      CXXFLAVOR := $(pflags)
else # use value from environment or command line, or default to normal
CXXFLAVOR ?= $(nflags)
endif
endif
endif

# Now the CXXFLAVOR is defined in all cases, we can safely mark it for export
export CXXFLAVOR

# suppress readline by setting readline=false
# and/or make more verbose by setting verbose=true

ifeq ($(readline),false)
    CXXFLAVOR += -DNREADLINE
    rl_libs =
else
    rl_libs ?= -lreadline
endif

# To override this define and export a shell variable 'rl_libs', for instance
# for readline on the Mac give the command: make rl_libs="-lreadline.5 -lcurses"
# Any value of LDFLAGS set in the environment is also included in $(LDFLAGS)
ifdef LDFLAGS
    LDFLAGS := $(LDFLAGS) $(rl_libs)
else
    LDFLAGS := $(rl_libs)
endif

ifeq ($(verbose),true)
    Fokko_flags += -DVERBOSE
endif

# the compiler to use, including language switch
# some C++11 support needed (rvalue references, shared_ptr) but g++-4.4 suffices
CXX = g++ -std=c++0x

CXXVERSION := $(shell $(CXX) -dumpversion)
CXXVERSIONOLD := $(shell expr `echo $(CXXVERSION) | cut -f1-2 -d.` \< 4.8)
CXXVERSIONVERYOLD := $(shell expr `echo $(CXXVERSION) | cut -f1-2 -d.` \< 4.6)

ifeq "$(CXXVERSIONOLD)" "1"
  CXXFLAVOR += -Dincompletecpp11
endif
ifeq "$(CXXVERSIONVERYOLD)" "1"
  CXXFLAVOR += -Dnoexcept= -Dconstexpr= -Dnullptr=0
endif


# RULES follow below

# This target causes failed actions to clean up their (corrupted) target
.DELETE_ON_ERROR:

# we use no suffix rules
.SUFFIXES:

.PHONY: all install version distribution

# The default target is 'all', which builds the executable 'Fokko', and 'atlas'
all: Fokko atlas

# the following dependency forces emptymode.cpp to be recompiled whenever any
# of the object files changes; this guarantees that the date in the version
# string it prints will be that of the last recompilation.
sources/interface/emptymode.o: $(Fokko_sources)

# For profiling not only 'CXXFLAGS' used in compiling is modified, but linking
# also is different
Fokko: $(Fokko_objects)
ifeq ($(profile),true)
	$(CXX) -pg -o Fokko $(Fokko_objects) $(LDFLAGS)
else
	$(CXX) -o Fokko $(Fokko_objects) $(LDFLAGS)
endif

# Rules with two colons are static pattern rules: they are like implicit
# rules, but only apply to the files listed before the first colon

$(filter-out sources/interface/io.o,$(Fokko_objects)) : %.o : %.cpp
	$(CXX) $(CXXFLAGS) $(Fokko_flags) -o $*.o $*.cpp

# the $(messagedir) variable is only needed for the compilation of io.cpp
sources/interface/io.o : sources/interface/io.cpp
	$(CXX) $(CXXFLAGS) $(Fokko_flags) -DMESSAGE_DIR_MACRO=\"$(messagedir)\" -o $@ $<


# for files proper to atlas, the build is defined inside sources/interpreter

atlas: $(cweb_dir)/ctanglex $(interpreter_made_files) $(atlas_objects)
	cd sources/interpreter && $(MAKE) ../../atlas

$(filter-out sources/interpreter/parser.tab.%,$(interpreter_made_files)):
	cd sources/interpreter && $(MAKE) $(subst sources/interpreter/,,$@)

$(interpreter_cweb_objects) : %.o : %.w # skip %.cpp, go straight to sources
	cd sources/interpreter && $(MAKE) $(subst sources/interpreter/,,$@)

sources/interpreter/parser.tab.c sources/interpreter/parser.tab.h: \
  sources/interpreter/parser.y
	cd sources/interpreter && $(MAKE) $(subst sources/interpreter/,,$@)

# bison produces a file with extension .c, nonetheless it too needs $(CXX)
sources/interpreter/parser.tab.o: \
  sources/interpreter/parser.tab.c sources/interpreter/parsetree.h
	cd sources/interpreter && $(MAKE) parser.tab.o

# generate files that describe which .o files depend on which other (.h) files
$(dependencies) : %.d : %.cpp
	$(CXX) $(Fokko_includes) -MM -MF $@ -MT "$*.o $*.d" $*.cpp

# now include all the files constructed by the previous rule; this defines
# the dependencies, found by the preprocessor scanning #include directives,
# so for all object files used by Fokko, 'make' will automatically remake
# any of $(dependencies) if necessary. But 'veryclean' target should skip this

ifneq ($(MAKECMDGOALS),veryclean)
include $(dependencies)
endif

include sources/interpreter/dependencies # these are manually maintained for now

install: Fokko atlas
ifneq ($(INSTALLDIR),$(shell pwd))
	@echo "Installing directories and files in $(INSTALLDIR)"
	$(INSTALL) -d $(INSTALLDIR)/www
	$(INSTALL) -d $(INSTALLDIR)/messages $(INSTALLDIR)/atlas-scripts
	$(INSTALL) -m 644 LICENSE COPYRIGHT README $(INSTALLDIR)
	$(INSTALL) -p Fokko atlas $(INSTALLDIR)
	$(INSTALL) -m 644 www/*html $(INSTALLDIR)/www/
	$(INSTALL) -m 644 messages/*.help $(INSTALLDIR)/messages/
	$(INSTALL) -m 644 messages/intro_mess $(INSTALLDIR)/messages/
	$(INSTALL) -m 644 atlas-scripts/* $(INSTALLDIR)/atlas-scripts/
endif
ifneq ($(BINDIR),$(INSTALLDIR))
	mkdir -p $(BINDIR)
	@if test -h $(BINDIR)/Fokko; then rm -f $(BINDIR)/Fokko; fi
	@if test -h $(BINDIR)/atlas; then rm -f $(BINDIR)/atlas; fi
	ln -s $(INSTALLDIR)/Fokko $(BINDIR)/Fokko # make symbolic link
# whenever BINDIR is not INSTALLDIR, we can put a shell script as bin/atlas
# which ensures the search path will be properly set (no compiled-in path here)
	(echo '#!/bin/sh'; echo 'exec $(INSTALLDIR)/atlas --path=$(INSTALLDIR)/atlas-scripts basic.at "$$@"') >$(BINDIR)/atlas; chmod a+x $(BINDIR)/atlas
endif

version:
	@echo $(version)

distribution:
	bash make_distribution.sh $(version)

.PHONY: mostlyclean clean veryclean showobjects
mostlyclean:
	$(RM) -f $(objects) $(interpreter_made_files) *~ */*~ sources/*/*~ \
           sources/*/*.tex sources/*/*.dvi sources/*/*.log sources/*/*.toc

clean: mostlyclean
	$(RM) -f Fokko atlas

veryclean: clean
	$(RM) -f sources/*/*.d cwebx/*.o cwebx/ctanglex cwebx/cweavex \

$(cweb_dir)/ctanglex: $(cweb_dir)/common.h $(cweb_dir)/ctangle.c
	cd $(cweb_dir) && $(MAKE) ctanglex
$(cweb_dir)/cweavex: $(cweb_dir)/common.h $(cweb_dir)/cweave.c
	cd $(cweb_dir) && $(MAKE) cweavex

showobjects:
	@echo objects: $(objects)
	@echo
	@echo atlas_objects: $(atlas_objects)
	@echo
	@echo Fokko_objects: $(Fokko_objects)
