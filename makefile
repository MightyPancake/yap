CC := gcc
CFLAGS := -Wall -Wextra
CYAN := [96m
PURPLE := [94m
GREEN := [92m
RED := [91m
RESET := [0m

debug ?= false
ifeq ($(debug),true)
    CFLAGS += -g -O1 -fno-omit-frame-pointer -DYAP_DEBUG
endif

show_test_output ?= false
detailed ?= false

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

.PHONY: default compiler lib test test_file path workflow

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

# Smoke-test: builds bindgen_smoke, runs it (default: synthetic C header).
#   make bindgen_smoke                             # synthetic header
#   make bindgen_smoke bindgen_header="<stdio.h>"   # real system header
# Binary kept at /tmp/bindgen_smoke for later manual runs.
bindgen_smoke:
	@echo $(PURPLE)Building bindgen smoke test$(RESET)
	@BDG_INC="$$(pkg-config --cflags-only-I clang 2>/dev/null || true)"; \
	BDG_INC=$${BDG_INC#-I}; \
	BDG_LIB="$$(pkg-config --variable=libdir clang 2>/dev/null || true)"; \
	[ -n "$${CLANG_INCDIR:-}" ] && BDG_INC="$$CLANG_INCDIR"; \
	[ -n "$${CLANG_LIBDIR:-}" ] && BDG_LIB="$$CLANG_LIBDIR"; \
	BDG_FLAGS=""; \
	[ -n "$$BDG_INC" ] && BDG_FLAGS="$$BDG_FLAGS -I$$BDG_INC"; \
	[ -n "$$BDG_LIB" ] && BDG_FLAGS="$$BDG_FLAGS -L$$BDG_LIB -Wl,-rpath,$$BDG_LIB"; \
	$(CC) tests/bindgen_smoke.c -o /tmp/bindgen_smoke $$BDG_FLAGS -lclang
	@echo $(CYAN)Running bindgen smoke test$(RESET)
	@BDG_LIB="$$(pkg-config --variable=libdir clang 2>/dev/null || true)"; \
	[ -n "$${CLANG_LIBDIR:-}" ] && BDG_LIB="$$CLANG_LIBDIR"; \
	[ -n "$$BDG_LIB" ] && export LD_LIBRARY_PATH="$$BDG_LIB"; \
	$(if $(bindgen_header),/tmp/bindgen_smoke '$(bindgen_header)',/tmp/bindgen_smoke)
	@echo $(GREEN)Bindgen smoke test passed!  binary: /tmp/bindgen_smoke$(RESET)

test:
	@make debug=true
	@make yap_ts debug=true
	@make yap_c debug=true
	@make yap_semantic debug=true
	@yap -c
	@yap -m
	@echo $(CYAN)Running tests$(RESET)
	@set -e; \
	failed=0; \
	for test_file in tests/*.yap; do \
		[ -e "$$test_file" ] || { echo "No test files found in ./tests"; exit 1; }; \
		vg_log=$$(mktemp); \
		cmd_log=$$(mktemp); \
		test_failed=0; \
		leak_found=0; \
		if valgrind \
			--track-origins=yes \
			--leak-check=full \
			--error-exitcode=99 \
			--suppressions=valgrind_suppressions.supp \
			--log-file="$$vg_log" \
			./yap "$$test_file" >"$$cmd_log" 2>&1; then \
			status="$(GREEN)passed$(RESET)"; \
		else \
			status="$(RED)failed$(RESET)"; \
			test_failed=1; \
			failed=1; \
		fi; \
		leaks=""; \
		if grep -Eq 'definitely lost:[[:space:]]*[1-9][0-9]* bytes|indirectly lost:[[:space:]]*[1-9][0-9]* bytes|possibly lost:[[:space:]]*[1-9][0-9]* bytes' "$$vg_log"; then \
			leak_found=1; \
			leaks="$(RED)LEAKING!$(RESET)"; \
		fi; \
		printf '%s: %s %s\n' "$$test_file" "$$status" "$$leaks"; \
		if [ "$(show_test_output)" = "true" ]; then \
			cat "$$cmd_log"; \
		fi; \
		if { [ "$$test_failed" -eq 1 ] || [ "$$leak_found" -eq 1 ]; }; then \
			echo $(CYAN)Output for $$test_file$(RESET); \
			cat "$$cmd_log"; \
			echo $(CYAN)Valgrind output for $$test_file$(RESET); \
			cat "$$vg_log"; \
		elif [ "$(detailed)" = "true" ]; then \
			echo $(CYAN)Output for $$test_file$(RESET); \
			cat "$$cmd_log"; \
			echo $(CYAN)Valgrind output for $$test_file$(RESET); \
			cat "$$vg_log"; \
		fi; \
		rm -f "$$vg_log" "$$cmd_log"; \
	done; \
	test $$failed -eq 0
	@echo $(GREEN)Tests passed!$(RESET)

rerun:
	@make debug=true
	@make yap_ts debug=true
	@make yap_c debug=true
	@make yap_semantic debug=true
	@make run test=$(test)

run:
	@[ -n "$(test)" ] || { echo "Usage: make run test=<name>.yap"; exit 1; }
	valgrind \
		--track-origins=yes \
		--leak-check=full \
		--error-exitcode=99 \
		--suppressions=valgrind_suppressions.supp \
		./yap tests/$(test).yap

submodules:
	git submodule update --init --recursive
	@make utils
	@make yap_ts debug=$(debug)
	@make yap_c debug=$(debug)
	@make yap_semantic debug=$(debug)

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
	@make test detailed=true

utils:
	@cd ./include/utils && make clean && make

yap_ts: all
	@cd ./components/yap-ts && make debug=$(debug)

yap_c: all
	@cd ./components/yap-c && make debug=$(debug)

yap_semantic: all
	@cd ./components/yap-semantic && make debug=$(debug)
