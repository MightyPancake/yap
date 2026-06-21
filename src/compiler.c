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

#define yap_quit_if_errors(ctx, compiler) if (yap_ctx_dispatch_errors(ctx)) return yap_early_compile_error_return(compiler, ctx, 1)

int compile(yap_args args){
    yap_log("YAP_HAS_VALGRIND: %d", YAP_HAS_VALGRIND);
    yap_log("Source files count: %ld", darr_len(args.extra));
    //Chose front
    yap_compiler compiler = (yap_compiler){0};

    //Load compiler modules
    // yap_compiler_load_macro_eval_module(&compiler, "./components/yap-macro/libyap_macro.so", "yap-c");
    yap_compiler_load_frontend_component(&compiler, "./components/yap-ts/libyap_ts.so", "yap-ts");
    yap_compiler_load_backend_component(&compiler, "./components/yap-c/libyap_c.so", "yap-c");
    yap_compiler_load_macro_component(&compiler, "./components/yap-c/libyap_c.so", "yap-c");
    // Load internal semantic module (provides yap_build)
    yap_compiler_load_internal_component(&compiler, "./components/yap-semantic/libyap_semantic.so", "yap-semantic");


    // void* front_handle = yap_get_handle("./components/yap-ts/libyap_ts.so");
    // void* back_handle = yap_get_handle("./components/yap-c/libyap_c.so");
    // yap_log("libyap_ts.so loaded at %p\n", front_handle);
    // void* sym = dlsym(front_handle, "yap_parse");
    // yap_log("yap_parse symbol at %p\n", sym);

    //Macro expansion function
    // TODO
    // compiler.front_module.macro_eval = load_func_dynamically(front_handle, front_name, yap_expand_macros_fn, "yap_eval_macro");

    //Function to print errors
    
    //Do the compilation procces here
    //Step 0: Create a context and attach callbacks
    //Fresh context
    yap_ctx* ctx = yap_ctx_new();
    //Printing errors callback
    ctx->print_error = compiler.frontend.print_error;
    
    //Phase 1: Parsing
    ctx = compiler.frontend.parse(ctx, args);
    yap_quit_if_errors(ctx, compiler);

    //Print source tree for debugging
    yap_ctx_print_source_tree(ctx);

    //Phase 2: Macro registration
    ctx = compiler.macro.register_macros(ctx);
    yap_quit_if_errors(ctx, compiler);

    //Phase X: Semantic analysis and macro expansion

    //Phase Y: Building result tree
    ctx = compiler.internal.build(ctx, args);
    yap_quit_if_errors(ctx, compiler);

    //Phase Z: Codegen and emission
    ctx = compiler.backend.codegen(ctx);
    yap_quit_if_errors(ctx, compiler);

    // //TODO: Change this, right now we check semantic run because macros are not there yet
    // ctx = compiler.internal_module.build(ctx, args);
    // if (yap_ctx_dispatch_errors(ctx)) return yap_early_compile_error_return(compiler, ctx, 1);

    //Step 2: Semantic analysis of macros
    //Step 3: Execute macros
    //Step 4: Semantic analysis of expanded code
    //Step 5: Codegen and emition
    //TODO: Implement codegen and emition lol
    //compiler.back_module.codegen(ctx);

    //Handle possible errors
    yap_quit_if_errors(ctx, compiler);
    int result = 0;

    //Cleanup
    yap_log("Freeing state and closing handles...");
    yap_log("Context allocated %u bytes in arena", quake_allocated_sz(&ctx->arena));
    yap_log("Freeing what remains of the context...\n\n");
    yap_ctx_free(*ctx);
    free(ctx);
    yap_free_args(args);
    #if defined(YAP_DEBUG) && YAP_HAS_VALGRIND
        VALGRIND_DO_LEAK_CHECK;
    #endif
    yap_free_compiler(compiler);
    return result;
}

int yap_early_compile_error_return(yap_compiler compiler, yap_ctx* ctx, int error_code){
    yap_free_compiler(compiler);
    yap_ctx_free(*ctx);
    free(ctx);
    return error_code;
}

void yap_free_compiler(yap_compiler compiler){
    yap_close_handle(compiler.frontend_handle);
    yap_close_handle(compiler.backend_handle);
    yap_close_handle(compiler.internal_handle);
    yap_close_handle(compiler.macro_handle);
}

void yap_compiler_load_macro_component(yap_compiler* compiler, const char* path, const char* name){
    compiler->macro_handle = yap_get_handle(path);
    compiler->macro.register_macros = load_func_dynamically(compiler->macro_handle, name, yap_register_macros_fn, "yap_register_macros");
    // compiler->macro.macro_eval = load_func_dynamically(compiler->macro_handle, name, yap_macro_eval_fn, "yap_eval_macro");
}

void yap_compiler_load_frontend_component(yap_compiler* compiler, const char* path, const char* name){
    compiler->frontend_handle = yap_get_handle(path);
    compiler->frontend.parse = load_func_dynamically(compiler->frontend_handle, name, yap_parse_fn, "yap_parse");
    compiler->frontend.print_error = load_func_dynamically(compiler->frontend_handle, name, yap_print_error_fn, "yap_print_error");
}

void yap_compiler_load_internal_component(yap_compiler* compiler, const char* path, const char* name){
    compiler->internal_handle = yap_get_handle(path);
    compiler->internal.build = load_func_dynamically(compiler->internal_handle, name, yap_build_fn, "yap_build");
}

void yap_compiler_load_backend_component(yap_compiler* compiler, const char* path, const char* name){
    compiler->backend_handle = yap_get_handle(path);
    compiler->backend.codegen = load_func_dynamically(compiler->backend_handle, name, yap_codegen_fn, "yap_gen_code");
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
        args->command = "components_dir";
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
    {"cflags", 'c', NULL, 0, "Output cflags for components.", 0},
    {"components", 'm', NULL, 0, "Output components path.", 1},
    {"output", 'o', "OUTPUT_FILE", 0, "The path to the result file.", 1},
    {"install", 'i', NULL, 0, "Install components (list component directories to be installed)", 2},
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
            //compile will free args, so we don't need to do it here
        }else{
            printf("No sources to compile!\n");
            result = 1;
            yap_free_args(args);
        }
    }
    char* yhd;
    strus_switch(args.command, "cflags"){
        yhd = yap_get_yap_home_path();
        printf("-I%s/include/ -L%s/lib/\n", yhd, yhd);
        free(yhd);
        yap_free_args(args);
    }strus_case(args.command, "components_dir"){
        yhd = yap_get_yap_home_path();
        printf("%s/components/\n", yhd);
        free(yhd);
        yap_free_args(args);
    }strus_case(args.command, "install"){
        printf("Installing...\n");
        yap_free_args(args);
    }
    return result;
}

