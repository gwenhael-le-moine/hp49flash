# Simple Makefile

# The compiler
CC=gcc

# use GTK? If no, comment.
USE_GTK=-DUSE_GTK

# and the options
CFLAGS=-O2 -Wall $(USE_GTK)

# What to do for all
all: upgrade

# Compile the parts
upgrade: unpak.o hpserial.o upgrade.o
ifeq "$(USE_GTK)" "-DUSE_GTK"
	$(CC) $(CFLAGS) unpak.o hpserial.o upgrade.o -o upgrade `gtk-config --libs`
endif

upgrade.o: upgrade.c
ifeq "$(USE_GTK)" "-DUSE_GTK"
	$(CC) $(CFLAGS) `gtk-config --cflags` -c upgrade.c -o upgrade.o
endif

hpserial.o: hpserial.c
ifeq "$(USE_GTK)" "-DUSE_GTK"
	$(CC) $(CFLAGS) `gtk-config --cflags` -c hpserial.c -o hpserial.o
endif


# remove everything but the source
clean:
	rm -f *.o upgrade
