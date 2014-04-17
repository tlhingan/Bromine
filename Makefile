#***********************************
# This makefile for use with the MercuryAPI
# and compiling the magicwrapper used within python. 
# Author: chriszaal@sait.ca
# Date: October 1, 2013
#***********************************

#Variables for use in the makefile
#Set gcc as the compiler to use for this make
CC=gcc

#Set CFLAGS for compilation to include the three folder locations of library include files
#Also set for all warnings 
CFLAGS=-Wall -c -fPIC 
#-I../../ -I../../lib/LTK/LTKC/Library -I../../lib/LTK/LTKC/Library/LLRP.org

#Set LDFLAGS for linking. Show all warnings and include library locations
LDFLAGS=-Wall 
#-I../../ -I../../lib/LTK/LTKC/Library -I../../lib/LTK/LTKC/Library/LLRP.org 

#Set LIBS to equal the file locations of where the libraries are actually located 
LIBS=-lmercuryapi -lltkc -lltkctm -lpthread
#-L. -L../../ -L../../lib/LTK/LTKC/Library -L../../lib/LTK/LTKC/Library/LLRP.org -L. -lmercuryapi -lltkc -lltkctm -lpthread

build:
	make reader
	make clean    

reader: TMReader.c socket_comms.o
	$(CC) $(CFLAGS) TMReader.c -o TMReader.o
	$(CC) $(LDFLAGS) TMReader.o socket_comms.o $(LIBS) -o TMReader

socket_comms.o: socket_comms.c
	gcc -c socket_comms.c -o socket_comms.o
clean:
	rm -rfv *.o libmagicwrapper.so
