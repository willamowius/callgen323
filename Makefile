#
# Makefile
#
# Makefile for H.323 Call Generator
#

PROG		= callgen323
SOURCES		= main.cxx

ifndef OPENH323DIR
#OPENH323DIR=$(HOME)/h323plus
OPENH323DIR=$(HOME)/tmp/2.2/h323plus
endif

include $(OPENH323DIR)/openh323u.mak

STDCCFLAGS += -Wno-unused-variable

# dependencies
$(OBJDIR)/main.o: main.h version.h

