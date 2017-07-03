#  ____     ___ |    / _____ _____
# |  __    |    |___/    |     |
# |___| ___|    |    \ __|__   |     gsKit Open Source Project.
# ----------------------------------------------------------------------
# Copyright 2004 - Chris "Neovanglist" Gilbert <Neovanglist@LainOS.org>
# Licenced under Academic Free License version 2.0
# Review gsKit README & LICENSE files for further details.
#
# examples/basic/Makefile - Makefile for "basic" example.
#

GSKITSRC=../..

EE_BIN  = ps2gl.elf
EE_OBJS = main.o text_stuff.o test_box.o test_performance.o
EE_LIBS = -lps2glut -lps2gl -lps2stuff -lc -lstdc++ -lg -lkernel -ldraw -lmath3d -lm -ldma -lpad
EE_CFLAGS += -g
EE_LIB_DIRS += -L../../../ps2stuff/objs_optimized_no_vu0 -L../../../ps2gl/objs_optimized_no_vu0 -L../../../ps2gl/glut/objs_optimized_no_vu0
EE_INCS += -I../../../ps2stuff/include -I../../../ps2gl/include -I../../../ps2gl/glut/include
EE_CFLAGS += -DNO_VU0_VECTORS
EE_CXXFLAGS += -DNO_VU0_VECTORS

use_cpp=true

all: $(EE_BIN)

test: all
	ps2client -h $(PS2_IP) execee host:$(EE_BIN)

reset: clean
	ps2client -h $(PS2_IP) reset

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

install: all

include ../../Makefile.pref
include ../Makefile.global
