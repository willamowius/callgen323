#
# Makefile
#
# Make file for OpenH323 Call Generator
#

PROG		= callgen323
SOURCES		= main.cxx

ifndef OPENH323DIR
OPENH323DIR=$(HOME)/openh323
endif

include $(OPENH323DIR)/openh323u.mak

# dependencies
$(OBJDIR)/main.o: main.h

