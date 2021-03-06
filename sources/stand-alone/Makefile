# where to look for the atlas sources (in the parent directory)
atlas_sources := ..

# only the directories listed below of the atlas sources are actually used
ATLAS_DIRS := utilities structure gkmod error io

# this variable is used both by g++, and by cweavex needing the trailing slash
INCLUDE_FLAGS := $(ATLAS_DIRS:%=-I$(atlas_sources)/%/)

# please make sure cweavex and ctanglex refer to the executables of the latest
# version of CWEBx, see http://www-math.univ-poitiers.fr/~maavl/CWEBx
# the ++ flags make them expect C++ rather than C

CTANGLE := ctanglex ++
CWEAVE := cweavex ++ $(INCLUDE_FLAGS)

# these are the optional flags for CWEBx, modify them according to your taste
CWEAVEFLAGS  := -p +mde
CTANGLEFLAGS := -p

debug ?= false

ifeq ($(debug),true)
      OFLAG := -ggdb
else
      OFLAG := -O
endif

CXXFLAGS := -std=c++0x -Wall $(OFLAG) $(INCLUDE_FLAGS)

# our beloved C++ compiler
CXX = g++

cwebx_sources := matrix-merge.w coef-merge.w KLread.w Euler-201.w
webtexfiles   := $(cwebx_sources:%.w=%.tex)

dvifiles := $(cwebx_sources:%.w=%.dvi)
derived_cpp := $(cwebx_sources:%.w=%.cpp)

illiterate_sources := matstat.cpp polstat.cpp linear.cpp Poincare.cpp \
  lights-off.cpp lists.cpp

matrix-merge_objects := ../utilities/bitmap.o ../utilities/constants.o \
   ../utilities/bits.o ../utilities/arithmetic.o ../error/error.o

matstat_objects:=../io/filekl_in.o ../io/basic_io.o \
   ../utilities/bits.o ../utilities/bitset.o ../utilities/constants.o


polstat_objects:=../io/filekl_in.o ../io/basic_io.o \
   ../utilities/bits.o ../utilities/bitset.o ../utilities/constants.o

searchpol_objects:=

Poincare_objects:= ../io/basic_io.o \
   ../utilities/bitset.o ../utilities/bits.o ../utilities/constants.o

lights-off_objects:= ../utilities/mod2_system.o  ../utilities/constants.o \
   ../utilities/bitmap.o ../utilities/bits.o

Euler_objects:= ../utilities/bitmap.o  ../utilities/constants.o \
   ../utilities/bits.o

lists_objects :=

executables := $(cwebx_sources:%.w=%) $(illiterate_sources:%.cpp=%)

# RULES follow now

.PHONY: all mostlyclean clean compilable

all:	$(dvifiles) $(executables)


# 'make clean' removes all derived files
clean:  mostlyclean
	rm -f $(executables)

# 'make mostlyclean' removes all derived files except the executable program
mostlyclean:
	rm -f *.o *.log *.toc $(webtexfiles) *.dvi *~ $(derived_cpp)

# 'make compilable' prepares everything for compilation
compilable: clean
	$(MAKE) CTANGLEFLAGS=-pl $(derived_cpp)

# we use no implicit suffix rules in this Makefile
.SUFFIXES:


# TeX files are also made from CWEBx files (.w) with the ++ switch, by cweavex
$(webtexfiles): %.tex: %.w
	$(CWEAVE) $(CWEAVEFLAGS) $<

# and DVI files are made from TeX files of course
$(dvifiles): %.dvi: %.tex
	$(TEX) $<

$(derived_cpp): %.cpp: %.w
	$(CTANGLE) $(CTANGLEFLAGS) $<

KLread: KLread.cpp

matrix-merge: matrix-merge.cpp $(matrix-merge_objects)

coef-merge: coef-merge.cpp

matstat: matstat.cpp $(matstat_objects)

polstat: polstat.cpp $(polstat_objects)
linear: linear.cpp $(polstat_objects)

Poincare: Poincare.cpp $(Poincare_objects)

lights-off: lights-off.cpp $(lights-off_objects)

Euler-201: Euler-201.cpp $(Euler_objects)

lists: lists.cpp ../utilities/sl_list.h $(lists_objects)

# a static pattern rule giving the usual C++ single-module compilation
$(executables): %: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
