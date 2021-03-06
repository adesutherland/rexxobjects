# *************************************************************************
# A B O U T   T H I S   W O R K  -   S H E L L S P A W N
# *************************************************************************
# Work Name   : ShellSpawn
# Description : This provides a simple interface to spawn a process
#             : with redirected input and output
# Copyright   : Copyright (C) 2008 Adrian Sutherland
# *************************************************************************
# A B O U T   T H I S   F I L E
# *************************************************************************
# File Name   : Makefile.mingw
# Description : Makefile for building shellspawn C library for Windows
#             : (mingw)
# *************************************************************************
# L I C E N S E
# *************************************************************************
# This program is free software: you can redistribute it and/or modify
# it under the terms of version 3 of the GNU General Public License as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# For the avoidance of doubt:
# - Version 3 of the license (i.e. not earlier nor later versions) apply.
# - a copy of the license text should be in the "license" directory of the
#   source distribution.
# - Requests for use under other licenses will be treated sympathetically,
#   please see contact details.
# *************************************************************************
# C O N T A C T   D E T A I L S
# *************************************************************************
# E-mail      : adrian@sutherlandonline.org
#             : adrian@open-bpm.org
#             : sutherland@users.sourceforge.net
# Web         : http://rexxobjects.sourceforge.net/
#             : www.open-bpm.org
# Telephone   : Please e-mail for details
# Postal      : UK - Please e-mail for details
# *************************************************************************

SRCDIR = src
DESTDIR = windows
CPP = g++
AR = ar cr
RANLIB = ranlib
DEBUG = N

#---------------------------------------------------------------------
# You should not have to change anything below here...
#---------------------------------------------------------------------

ifeq ($(DEBUG),Y)
DISTDIR = ../dist/Debug
OPTIMIZE = -g
else
DISTDIR = ../dist/Release
OPTIMIZE = -O3 -DNDEBUG -s
endif

COPT = $(OPTIMIZE) -I$(SRCDIR) -DWIN32

#FLTKOPT = -DFLTK $(shell fltk2-config --cxxflags)
#FLTKLIB = $(shell fltk2-config --ldflags)

all: makedir $(DESTDIR)/libshellspawn.a $(DESTDIR)/test/shelltest $(DESTDIR)/test/testclient $(DESTDIR)/test/noconsoletest

$(DESTDIR)/libshellspawn.a: $(DESTDIR)/win32shell.o $(SRCDIR)/shellspawn.h
	$(AR) $@ $(DESTDIR)/win32shell.o
	$(RANLIB) $@
	cp $(SRCDIR)/shellspawn.h $(DESTDIR)

$(DESTDIR)/test/shelltest: $(DESTDIR)/test/shelltest.o $(DESTDIR)/libshellspawn.a
	$(CPP) $(COPT) -o $@ $(DESTDIR)/test/shelltest.o -L$(DESTDIR) -lshellspawn

$(DESTDIR)/win32shell.o : $(SRCDIR)/win32shell.cpp $(SRCDIR)/shellspawn.h
	$(CPP) $(COPT) -c -o $@ $(SRCDIR)/win32shell.cpp

$(DESTDIR)/test/noconsoletest: $(DESTDIR)/test/noconsoletest.o $(DESTDIR)/libshellspawn.a
	$(CPP) $(COPT) $(FLTKOPT) -o $@ $(DESTDIR)/test/noconsoletest.o -L$(DESTDIR) -lshellspawn $(FLTKLIB)

$(DESTDIR)/test/testclient: $(DESTDIR)/test/testclient.o
	$(CPP) $(COPT) -o $@ $(DESTDIR)/test/testclient.o

$(DESTDIR)/test/shelltest.o : $(SRCDIR)/test/shelltest.cpp $(DESTDIR)/shellspawn.h
	$(CPP) $(COPT) -c -o $@ $(SRCDIR)/test/shelltest.cpp
	cp $(SRCDIR)/test/input.txt $(DESTDIR)/test

$(DESTDIR)/test/noconsoletest.o : $(SRCDIR)/test/noconsoletest.cpp $(DESTDIR)/shellspawn.h
	$(CPP) $(COPT) $(FLTKOPT) $(FLTKINC) -c -o $@ $(SRCDIR)/test/noconsoletest.cpp

$(DESTDIR)/test/testclient.o : $(SRCDIR)/test/testclient.cpp
	$(CPP) $(COPT) -c -o $@ $(SRCDIR)/test/testclient.cpp

makedir:
	@if [ ! -d $(DESTDIR) ]; then \
	mkdir -p $(DESTDIR); \
	fi
	@if [ ! -d $(DESTDIR)/test ]; then \
	mkdir -p $(DESTDIR)/test; \
	fi

clean:
	rm $(DESTDIR)/*
	rm $(DESTDIR)/test/*

dist:
	@if [ ! -d ../dist ]; then \
	mkdir ../dist; \
	fi
	@if [ ! -d $(DISTDIR) ]; then \
	mkdir -p $(DISTDIR); \
	fi
	cp $(DESTDIR)/*.a $(DISTDIR)
	cp $(DESTDIR)/*.h $(DISTDIR)
