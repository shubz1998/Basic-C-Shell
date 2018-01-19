# Simple C Shell Makefile

CC = gcc
CFLAGS  = -Wall -g
OBJ = myshell.o

all: myshell

myshell: $(OBJ)
	$(CC) $(CFLAGS) -o myshell $(OBJ) 

%.o: %.c
	$(CC) $(CFLAGS) -c $<
