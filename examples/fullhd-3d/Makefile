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

EE_BIN  = fullhd-3d.elf
EE_OBJS = main.o
EE_LIBS = -L$(GSKITSRC)/lib -L$(PS2SDK)/ee/lib -lsupc++ -lgskit -ldmakit -lc -lkernel -lmath3d -lm -ldraw
EE_INCS = -I$(GSKITSRC)/ee/gs/include -I$(GSKITSRC)/ee/dma/include

all: $(EE_BIN)

test: all
	ps2client -h $(PS2_IP) -t 1 execee host:$(EE_BIN)

reset: clean
	ps2client -h $(PS2_IP) reset

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

install: all

include ../../Makefile.pref
#include ../Makefile.global
include $(GSKITSRC)/ee/Rules.make
