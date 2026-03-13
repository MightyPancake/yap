CC := gcc
CFLAGS := -Wall
CYAN := [96m
PURPLE := [94m
GREEN := [92m
RESET := [0m

debug ?= false
ifeq ($(debug),true)
    CFLAGS += -g -O1 -fno-omit-frame-pointer -DYAP_DEBUG
endif

log := $(debug)
ifeq ($(log),true)
    CFLAGS += -DYAP_LOG
endif

YAP_PATH := $(shell pwd)
export PATH := $(YAP_PATH):$(PATH)

YAP_SHARED_FLAGS := -I./include -L./lib $(CFLAGS)

#Makes sure the compiler looks for yap.so in the lib dir
YAP_COMPILER_LINKER_FLAGS := -Wl,-rpath,$(YAP_PATH)/lib
YAP_COMPILER_FLAGS := $(YAP_SHARED_FLAGS) ./src/*.c -rdynamic -lyap -o yap $(YAP_COMPILER_LINKER_FLAGS)

YAP_LIB_FLAGS := $(YAP_SHARED_FLAGS) ./src/lib/*.c

RM := rm -fr
CP := cp -r
MV := mv

.PHONY: default compiler lib test path workflow

default: all

all: lib compiler

compiler:
	@echo $(PURPLE)Building yap compiler$(RESET)
	@echo $(CYAN)"debug: $(debug)"$(RESET)
	$(CC) $(YAP_COMPILER_FLAGS) 
	@echo $(GREEN)Done!$(RESET)

clean:
	@$(RM) yap
	@find ./lib -mindepth 1 ! -name 'libs.md' -delete

static_lib:
	@echo $(PURPLE)Building yap lib$(RESET)
	$(CC) -c $(YAP_LIB_FLAGS)
	ar rcs libyap.a ./*.o
	$(MV) libyap.a ./lib
	$(RM) ./*.o
	@echo $(GREEN)Done!$(RESET)

# @$(CC) -fPIC -shared -o ./lib/libyap.so $(YAP_LIB_FLAGS)
lib:
	@echo $(PURPLE)Building libyap.so$(RESET)
	$(CC) -fPIC -c $(YAP_LIB_FLAGS)
	$(CC) -shared -o ./lib/libyap.so ./*.o
	$(RM) ./*.o
	@echo $(GREEN)Done!$(RESET)

test:
	@make debug=true
	@yap -c
	@yap -m
	valgrind \
    --track-origins=yes \
    --suppressions=valgrind_suppressions.supp \
    ./yap examples/test.yap

submodules:
	git submodule update --init --recursive --remote
	@make utils
	@make yap_ts debug=$(debug)

path:
	@echo $(PURPLE)Adding yap to PATH using pathman$(RESET)
	@if command -v pathman >/dev/null 2>&1; then \
		pathman add "$(YAP_PATH)"; \
	elif [ -n "$$GITHUB_PATH" ]; then \
		echo "$(YAP_PATH)" >> "$$GITHUB_PATH"; \
		echo $(GREEN)Added yap to GitHub Actions PATH$(RESET); \
	else \
		echo $(CYAN)pathman not found; PATH is exported for this make process$(RESET); \
	fi

workflow:
	@make submodules
	@make path
	@make test

utils:
	@cd ./include/utils && make clean && make

yap_ts: all
	@cd ./modules/yap-ts && make clean && make debug=$(debug)
