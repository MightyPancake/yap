#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <argp.h>
#include "yap/all.h"
#include "yap/types.h"

#ifdef YAP_DEBUG
    #if defined(__has_include)
        #if __has_include(<valgrind/memcheck.h>) && __has_include(<valgrind/valgrind.h>)
            #include <valgrind/memcheck.h>
            #include <valgrind/valgrind.h>
            #define YAP_HAS_VALGRIND 1
        #endif
    #endif
#endif

#ifndef YAP_HAS_VALGRIND
    #define YAP_HAS_VALGRIND 0
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
    if (handle) dlclose(handle);
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

int compile(yap_args args){
    yap_log("Source files count: %ld", darr_len(args.extra));
    //Chose front
    yap_compiler compiler = (yap_compiler){
        .back_handle = NULL,
        .macro_eval_handle = NULL,
        .front_handle = NULL,
    };

    //Load compiler modules
    // yap_compiler_load_macro_eval_module(&compiler, "./modules/yap-macro/libyap_macro.so", "yap-c");
    yap_compiler_load_front_module(&compiler, "./modules/yap-ts/libyap_ts.so", "yap-ts");
    yap_compiler_load_back_module(&compiler, "./modules/yap-c/libyap_c.so", "yap-c");


    // void* front_handle = yap_get_handle("./modules/yap-ts/libyap_ts.so");
    // void* back_handle = yap_get_handle("./modules/yap-c/libyap_c.so");
    // yap_log("libyap_ts.so loaded at %p\n", front_handle);
    // void* sym = dlsym(front_handle, "yap_parse");
    // yap_log("yap_parse symbol at %p\n", sym);

    //Macro expansion function
    // TODO
    // compiler.front_module.macro_eval = load_func_dynamically(front_handle, front_name, yap_expand_macros_fn, "yap_eval_macro");

    //Function to print errors
    
    //Do the compilation procces here
    //Step 1: Create a context
    yap_ctx* ctx = yap_ctx_new();
    ctx->print_error = compiler.front_module.print_error;

    //Step 2: Attach macro eval function to context so it can be used during parsing

    
    //Step 3: Parse the source files and fill the context with the results
    ctx = compiler.front_module.parse(ctx, args);
    //at this point, ctx should be filled with sources, source codes, errors and types. We can check for errors and print them if needed.

    //Step 4: Codegen and emition
    //TODO: Implement codegen and emition lol
    compiler.back_module.codegen(ctx);

    //Handle possible errors
    for_darr(i, err, ctx->errors){
        compiler.front_module.print_error(err);
    }
    int result = darr_len(ctx->errors) ? 1 : 0;
    
    yap_log("Parsing finished with %ld error(s)", darr_len(ctx->errors));

    //Cleanup
    yap_log("Freeing state and closing handles...");
    yap_log("Context allocated %u bytes in arena", quake_allocated_sz(&ctx->arena));
    yap_log("Freeing what remains of the context...\n\n");
    yap_ctx_free(*ctx);
    free(ctx);
    
    //This will force resolving debug symbols before closing the handle!
    //Very important for valgrind to work correctly! :^)
    //tldr; multiple hours wasted learning this
    #if defined(YAP_DEBUG) && YAP_HAS_VALGRIND
        VALGRIND_DO_LEAK_CHECK;
    #endif
    // end of stuff here
    yap_close_handle(compiler.front_handle);
    yap_close_handle(compiler.back_handle);
    yap_close_handle(compiler.macro_eval_handle);
    return result;
}

void yap_compiler_load_macro_eval_module(yap_compiler* compiler, const char* path, const char* name){
    (void)name;
    compiler->macro_eval_handle = yap_get_handle(path);
    compiler->macro_eval_module.macro_eval = load_func_dynamically(compiler->macro_eval_handle, name, yap_macro_eval_fn, "yap_eval_macro");
}

void yap_compiler_load_front_module(yap_compiler* compiler, const char* path, const char* name){
    compiler->front_handle = yap_get_handle(path);
    compiler->front_module.parse = load_func_dynamically(compiler->front_handle, name, yap_parse_fn, "yap_parse");
    compiler->front_module.print_error = load_func_dynamically(compiler->front_handle, name, yap_print_error_fn, "yap_print_error");
}

void yap_compiler_load_back_module(yap_compiler* compiler, const char* path, const char* name){
    (void)name;
    compiler->back_handle = yap_get_handle(path);
    compiler->back_module.codegen = load_func_dynamically(compiler->back_handle, name, yap_codegen_fn, "yap_gen_code");
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
        darr_push(args->extra, arg);
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

static struct argp argp = { options, parse_args, args_doc, doc, .children=NULL};

char* yap_get_yap_home_path(){
        char* exec_path = yap_get_self_path();
        char* resolved_yap_exec = yap_resolve_path(exec_path);
        char* yap_home_dir = yap_get_parent_dir(resolved_yap_exec);
        free(resolved_yap_exec);
        free(exec_path);
        return yap_home_dir;
}

int main(int argc, char** argv) {
    int result = 0;
    //Parse args
    yap_args args = (yap_args){
      .output_file = "a",
            .extra = darr_new(char*),
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
        if (darr_len(args.extra) > 0){
            result = compile(args);
        }else{
            printf("No sources to compile!\n");
            result = 1;
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
    return result;
}

