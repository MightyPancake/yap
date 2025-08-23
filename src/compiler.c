#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <argp.h>
#include "yap/all.h"

void* yap_get_handle(const char* path){
    void *handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
    // Clear any existing errors
    dlerror();
    return handle;
}

void yap_close_handle(void* handle){
    dlclose(handle);
}

#define load_func_dynamically(HANDLE, HANDLE_NAME, T, SYM) ({\
    T DL_SYM_RES = (T)dlsym(HANDLE, SYM); \
    char *error = dlerror(); \
    if (error != NULL) { \
        fprintf(stderr, "Failed to find '"SYM"' in %s: %s\n", HANDLE_NAME, error); \
        dlclose(HANDLE); \
        exit(1); \
    } \
    DL_SYM_RES; \
    })

void compile(yap_args args){
    printf("Source files count: %ld\n", darr_len(args.source_paths));
    //Chose front
    yap_compiler compiler = (yap_compiler){
        .front = (yap_compiler_front){}
    };

    const char front_name[] = "ts_yap";
    void* front_handle = yap_get_handle("./modules/ts_yap/ts_yap.so");

    compiler.front.parse = load_func_dynamically(front_handle, front_name, yap_parse_fn, "yap_parse");
    //do stuff here
    yap_state* state = compiler.front.parse(args);
    
    yap_free_state(state);
    
    //end of stuff here
    yap_close_handle(front_handle);
}

static error_t parse_args(int key, char *arg, struct argp_state *state) {
    yap_args* args = state->input;

    switch(key) {
    case 'o':
        args->output_file = arg;
        break;
    case 'c':
        args->show_cflags = true;
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num >= 1)
            argp_usage(state);
        darr_push(char*, args->source_paths, arg);
        break;
    case ARGP_KEY_END:
        // if (state->arg_num < 1)
        // //No source provided
        // argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp_option options[] = {
    //{"long_name", 'short_name', "value_name | NULL for no value", flags, "doc string", group},
    {"output", 'o', "OUTPUT_FILE", 0, "The path to the result file.", 0},
    {"cflags", 'c', NULL, 0, "Output cflags for modules.", 1},
    {0}
};

static char doc[] = "The tool for yap programming language.";
static char args_doc[] = "source file(s)";

static struct argp argp = { options, parse_args, args_doc, doc };

int main(int argc, char** argv) {
    //Parse args
    yap_args args = (yap_args){
      .output_file = "a",
      .source_paths = darr_new(char*, 16),
      .show_cflags = false
    };

    int res = argp_parse(&argp, argc, argv, 0, 0, &args);
    if (res){
        fprintf(stderr, "Error while resolving arguments using argp.\n");
        return 1;
    }
    if (!darr_empty(args.source_paths)){
        compile(args);
    }
    if (args.show_cflags){
        char* resolved_yap_home = yap_resolve_path(".");
        char* rsm = resolved_yap_home;
        printf("-I%s/include/ -L%s/lib/\n", rsm, rsm);
        free(resolved_yap_home);
    }
    yap_free_args(args);
    return 0;
}

