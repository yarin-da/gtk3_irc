CC = gcc
SRC_DIR = src
INC_DIR = inc
OBJ_DIR = obj
# -DNET_DEBUG to add network debugging messages 
CFLAGS = -c -Wall -I$(INC_DIR) -g #-DNET_DEBUG
OUT = irc
PACKAGE = $(shell pkg-config --cflags gtk+-3.0)
LDLIBS = $(shell pkg-config --libs gtk+-3.0 gmodule-2.0)

OBJS = $(OBJ_DIR)/main.o $(OBJ_DIR)/common.o $(OBJ_DIR)/net.o\
	   $(OBJ_DIR)/serverinput.o $(OBJ_DIR)/ui.o $(OBJ_DIR)/userinput.o\
	   $(OBJ_DIR)/context.o $(OBJ_DIR)/chanlist.o $(OBJ_DIR)/serveroutput.o\

main: build

build: $(OBJS)
	$(CC) $(OBJS) -o $(OUT) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(PACKAGE) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) $(OUT) $(OBJS)
