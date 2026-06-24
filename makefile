CC := gcc
CFLAGS := -Wall -Wextra
CYAN := [96m
PURPLE := [94m
GREEN := [92m
RED := [91m
RESET := [0m

debug ?= true
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

# Resolve libclang include/lib dirs once.
# On Nix: CLANG_INCDIR/CLANG_LIBDIR come from shellHook.
# On Ubuntu: libclang-18-dev puts files under /usr/lib/llvm-18/.
CLANG_INCDIR := $(or $(CLANG_INCDIR),$(shell [ -d /usr/lib/llvm-18/include ] && echo /usr/lib/llvm-18/include))
CLANG_LIBDIR := $(or $(CLANG_LIBDIR),$(shell [ -d /usr/lib/llvm-18/lib     ] && echo /usr/lib/llvm-18/lib))
ifneq ($(CLANG_INCDIR),)
  CLANG_CFLAGS := -I$(CLANG_INCDIR)
endif
ifneq ($(CLANG_LIBDIR),)
  CLANG_LDFLAGS := -L$(CLANG_LIBDIR) -Wl,-rpath,$(CLANG_LIBDIR)
endif
CLANG_LIBS := -lclang

# Make these available to lib build (src/lib/bindgen.c needs clang-c/Index.h)
YAP_SHARED_FLAGS += $(CLANG_CFLAGS)

#Makes sure the compiler looks for yap.so in the lib dir
YAP_COMPILER_LINKER_FLAGS := -Wl,-rpath,$(YAP_PATH)/lib
YAP_COMPILER_FLAGS := $(YAP_SHARED_FLAGS) ./src/*.c -rdynamic -lyap $(CLANG_LDFLAGS) $(CLANG_LIBS) -o yap $(YAP_COMPILER_LINKER_FLAGS)

YAP_LIB_FLAGS := $(YAP_SHARED_FLAGS) ./src/lib/*.c

RM := rm -fr
CP := cp -r
MV := mv

.PHONY: default compiler lib test test_file path workflow bindings hello

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
	$(CC) -shared -o ./lib/libyap.so ./*.o $(CLANG_LDFLAGS) $(CLANG_LIBS)
	$(RM) ./*.o
	@echo $(GREEN)Done!$(RESET)

# Smoke-test: builds bindgen_smoke, runs it (default: synthetic C header).
#   make bindgen_smoke                             # synthetic header
#   make bindgen_smoke bindgen_header="<stdio.h>"   # real system header
# On Nix: CLANG_INCDIR/CLANG_LIBDIR come from shellHook.
# On Ubuntu: clang package puts headers/libs in system paths.
bindings: lib compiler
	@echo $(PURPLE)Generating C bindings$(RESET)
	@mkdir -p modules/io
	./yap --gen-c-bind '<stdio.h>' -o modules/io/binds.yap
	@echo $(GREEN)Done!$(RESET)

hello: bindings
	@echo $(PURPLE)Building and running hello world$(RESET)
	./yap -o hello examples/hello.yap
	./hello
	@echo $(GREEN)Done!$(RESET)

bindgen_smoke:
	@echo $(PURPLE)Building bindgen smoke test$(RESET)
	$(CC) tests/bindgen_smoke.c -o /tmp/bindgen_smoke $(CLANG_CFLAGS) $(CLANG_LDFLAGS) -lclang
	@echo $(CYAN)Running bindgen smoke test$(RESET)
	@$(if $(CLANG_LIBDIR),LD_LIBRARY_PATH="$(CLANG_LIBDIR)" ,)/tmp/bindgen_smoke $(if $(bindgen_header),'$(bindgen_header)',)
	@echo $(GREEN)Bindgen smoke test passed!  binary: /tmp/bindgen_smoke$(RESET)

test: lib compiler
	@make yap_ts debug=$(debug)
	@make yap_c debug=$(debug)
	@make yap_semantic debug=$(debug)
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

rerun: lib compiler
	@make yap_ts debug=$(debug)
	@make yap_c debug=$(debug)
	@make yap_semantic debug=$(debug)
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
