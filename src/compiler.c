#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <argp.h>
#include "yap/all.h"

#ifdef YAP_DEBUG
    #include <valgrind/memcheck.h>
    #include <valgrind/valgrind.h>
#endif

void* yap_get_handle(const char* path){
    char* abs_path = yap_resolve_path(path);
    printf("handle for: %s\n", abs_path);
    void *handle = dlopen(abs_path, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
    // Clear any existing errors
    dlerror();
    free(abs_path);
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
    printf("Source files count: %ld\n", darr_len(args.extra));
    //Chose front
    yap_compiler compiler = (yap_compiler){
        .front = (yap_compiler_front){}
    };

    const char front_name[] = "ts_yap";
    void* front_handle = yap_get_handle("./modules/yap-ts/libyap_ts.so");
    printf("libyap_ts.so loaded at %p\n", front_handle);
    printf("libyap_ts.so loaded at %p\n", front_handle);
    void* sym = dlsym(front_handle, "yap_parse");
    printf("yap_parse symbol at %p\n", sym);


    compiler.front.parse = load_func_dynamically(front_handle, front_name, yap_parse_fn, "yap_parse");
    //do stuff here
    yap_state* state = compiler.front.parse(args);
    
    yap_free_state(state);
    
    //This will force resolving debug symbols before closing the handle!
    //Very important for valgrind to work correctly! :^)
    //tldr; multiple hours wasted learning this
    #ifdef YAP_DEBUG
        VALGRIND_DO_LEAK_CHECK;
    #endif
    // end of stuff here
    yap_close_handle(front_handle);
}

static error_t parse_args(int key, char *arg, struct argp_state *state) {
    yap_args* args = state->input;

    switch(key) {
    case 'o':
        args->output_file = arg;
        break;
    case 'c':
        args->command = "cflags";
        break;
    case 'm':
        args->command = "modules_dir";
        break;
    case 'i':
        args->command = "install";
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num >= 1)
            argp_usage(state);
        darr_push(char*, args->extra, arg);
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
    {"cflags", 'c', NULL, 0, "Output cflags for modules.", 0},
    {"modules", 'm', NULL, 0, "Output modules path.", 1},
    {"output", 'o', "OUTPUT_FILE", 0, "The path to the result file.", 1},
    {"install", 'i', NULL, 0, "Install modules (list module directories to be installed)", 2},
    {0}
};

static char doc[] = "The tool for yap programming language.";
static char args_doc[] = "source file(s)";

static struct argp argp = { options, parse_args, args_doc, doc };

char* yap_get_yap_home_path(){
        char* exec_path = yap_get_self_path();
        char* resolved_yap_exec = yap_resolve_path(exec_path);
        char* yap_home_dir = yap_get_parent_dir(resolved_yap_exec);
        free(resolved_yap_exec);
        free(exec_path);
        return yap_home_dir;
}

int main(int argc, char** argv) {
    //Parse args
    yap_args args = (yap_args){
      .output_file = "a",
      .extra = darr_new(char*, 16),
      .command = "compile"
    };

    int res = argp_parse(&argp, argc, argv, 0, 0, &args);
    if (res){
        fprintf(stderr, "Error while resolving arguments using argp.\n");
        return 1;
    }
    // printf("Command: %s\n", args.command);

    //Switch on command
    strus_switch(args.command, "compile"){
        //do compile stuff here
        if (!darr_empty(args.extra)){
            compile(args);
        }else{
            printf("No sources to compile!\n");
        }
    }
    char* yhd;
    strus_switch(args.command, "cflags"){
        yhd = yap_get_yap_home_path();
        printf("-I%s/include/ -L%s/lib/\n", yhd, yhd);
        free(yhd);
    }strus_case(args.command, "modules_dir"){
        yhd = yap_get_yap_home_path();
        printf("%s/modules/\n", yhd);
        free(yhd);
    }strus_case(args.command, "install"){
        printf("Installing...\n");
    }
    yap_free_args(args);
    return 0;
}

