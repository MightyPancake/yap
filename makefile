CC := gcc
CFLAGS := -Wall -g
CYAN := [96m
PURPLE := [94m
GREEN := [92m
RESET := [0m

debug ?= false
log := $(debug)

YAP_PATH := $(pwd)

ifeq ($(log),true)
    CFLAGS += -DYAP_LOG
endif

YAP_SHARED_FLAGS := -I./include -L./lib

YAP_COMPILER_FLAGS := $(YAP_SHARED_FLAGS) ./src/*.c -lyap -o yap

YAP_LIB_FLAGS := $(YAP_SHARED_FLAGS) ./src/lib/*.c

RM := rm -fr
CP := cp -r
MV := mv

.PHONY: default compiler lib test path

default: all

all: lib compiler path

compiler:
	@echo $(PURPLE)Building yap compiler$(RESET)
	@echo $(CYAN)"debug: $(debug)"$(RESET)
	@$(CC) $(YAP_COMPILER_FLAGS) 
	@echo $(GREEN)Done!$(RESET)

clean:
	@$(RM) yap
	@$(RM) ./lib/*

lib:
	@echo $(PURPLE)Building yap lib$(RESET)
	@$(CC) -c $(YAP_LIB_FLAGS)
	@ar rcs libyap.a ./*.o
	@$(MV) libyap.a ./lib
	@$(RM) ./*.o
	@echo $(GREEN)Done!$(RESET)

test:
	@make debug=true
	valgrind --track-origins=yes --leak-check=full -s ./yap -c

submodules:
	git submodule update --init --recursive --remote
	@make utils

path:
	@echo $(PURPLE)Adding yap to PATH using pathman$(RESET)
	@pathman add .

utils:
	@cd ./include/utils && make clean && make
