#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>
#include <unistd.h>
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
    yap_log("handle for: %s\n", abs_path);
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

static char* yap_component_so_path(const char* yap_home, const char* component_name){
    char* underscored = strus_copy((char*)component_name);
    for (char* p = underscored; *p; p++)
        if (*p == '-') *p = '_';
    char* path = strus_newf("%s/components/%s/lib%s.so", yap_home, component_name, underscored);
    free(underscored);
    return path;
}

static void yap_describe_component_flags_section(FILE* out, const char* yap_home, const char* role, const char* component_name, char flag_letter){
    char* path = yap_component_so_path(yap_home, component_name);
    void* handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    free(path);

    fprintf(out, "  " aesc_bold_on aesc_cyan "%s" aesc_reset " (-%c<FLAG>, component: " aesc_yellow "%s" aesc_reset "):\n",
        role, flag_letter, component_name);
    if (!handle){
        fprintf(out, "    " aesc_red "could not load component" aesc_reset "\n\n");
        return;
    }
    dlerror();
    yap_describe_flags_fn describe = (yap_describe_flags_fn)dlsym(handle, "yap_describe_flags");
    dlerror();

    int count = 0;
    const yap_flag_desc* flags = describe ? describe(&count) : NULL;
    if (!flags || count == 0){
        fprintf(out, "    (no flags documented)\n\n");
    } else {
        for (int i = 0; i < count; i++)
            fprintf(out, "    " aesc_green "-%c%s" aesc_reset "\t%s\n", flag_letter, flags[i].flag, flags[i].description);
        fprintf(out, "\n");
    }
    dlclose(handle);
}

static void yap_describe_all_component_flags(FILE* out, yap_args* args){
    char* yap_home = yap_get_yap_home_path();
    fprintf(out, aesc_bold_on "Component flags" aesc_reset " (resolved by each component; pick a component with -s, e.g. -sback=yap-c):\n\n");
    yap_describe_component_flags_section(out, yap_home, "Backend", args->backend_component, 'b');
    yap_describe_component_flags_section(out, yap_home, "Frontend", args->frontend_component, 'f');
    free(yap_home);
}

int compile(yap_args args){
    yap_log("YAP_HAS_VALGRIND: %d", YAP_HAS_VALGRIND);
    yap_log("Source files count: %ld", darr_len(args.extra));
    //Chose front
    yap_compiler compiler = (yap_compiler){0};
    compiler.args = &args;

    //Load compiler modules (paths relative to yap executable, not CWD)
    char* _yh = yap_get_yap_home_path();
    char* _ts  = yap_component_so_path(_yh, args.frontend_component);
    char* _c   = yap_component_so_path(_yh, args.backend_component);
    char* _sem = yap_component_so_path(_yh, args.semantic_component);
    yap_compiler_load_frontend_component(&compiler, _ts, args.frontend_component);
    yap_compiler_load_backend_component(&compiler, _c, args.backend_component);
    yap_compiler_load_semantic_component(&compiler, _sem, args.semantic_component);
    free(_ts); free(_c); free(_sem); free(_yh);

    yap_ctx* ctx = yap_ctx_new();
    //Callbacks from loaded components
    ctx->print_error = compiler.frontend.print_error;
    ctx->gen_decl = compiler.backend.gen_decl;
    ctx->ensure_symbol = compiler.backend.ensure_symbol;
    ctx->set_macro_name = compiler.backend.set_macro_name;
    ctx->set_macro_loc = compiler.backend.set_macro_loc;
    ctx->pop_macro_loc = compiler.backend.pop_macro_loc;
    ctx->args = compiler.args;

    //Module lookup paths
    char* yap_home = yap_get_yap_home_path();
    char* modules_path = strus_newf("%s/modules", yap_home);
    darr_push(ctx->module_lookup_paths, modules_path);
    free(yap_home);

    //Phase 1: Parsing (includes import resolution and building the source tree)
    ctx = compiler.frontend.parse(ctx, args);
    yap_quit_if_errors(ctx, compiler);

    //Print source tree for debugging
#ifdef YAP_DEBUG
    yap_ctx_print_source_tree(ctx);
#endif

    //Phase 0: Module declaration resolution (find and validate the module decl across all sources)
    yap_resolve_module_decl(ctx);
    yap_quit_if_errors(ctx, compiler);

    //Phase 2: Backend init (sets up TCC state, module contexts, etc.)
    if (compiler.backend.init)
        compiler.backend.init(ctx);
    yap_quit_if_errors(ctx, compiler);

    ctx = compiler.semantic.build(ctx, args);
    yap_quit_if_errors(ctx, compiler);

    //Phase 3: Emission (write files, invoke gcc)
    if (compiler.backend.emit){
        ctx = compiler.backend.emit(ctx);
        yap_quit_if_errors(ctx, compiler);
    }

    //Handle possible errors
    yap_quit_if_errors(ctx, compiler);
    int result = args.run ? ctx->run_exit_code : 0;

    //Cleanup
    yap_log("Freeing state and closing handles...");
    yap_log("Context allocated %u bytes in arena", quake_allocated_sz(&ctx->arena));
    yap_log("Freeing what remains of the context...\n\n");
    if (compiler.backend.free)
        compiler.backend.free(ctx);
    yap_ctx_free(*ctx);
    free(ctx);
    yap_free_compiler(compiler);
    #if defined(YAP_DEBUG) && YAP_HAS_VALGRIND
        VALGRIND_DO_LEAK_CHECK;
    #endif
    yap_free_compiler_handles(compiler);
    return result;
}

int yap_early_compile_error_return(yap_compiler compiler, yap_ctx* ctx, int error_code){
    if (compiler.backend.free)
        compiler.backend.free(ctx);
    yap_free_compiler(compiler);
    yap_free_compiler_handles(compiler);
    yap_ctx_free(*ctx);
    free(ctx);
    return error_code;
}

void yap_free_compiler(yap_compiler compiler){
    if (compiler.args) yap_free_args(*compiler.args);
}

void yap_free_compiler_handles(yap_compiler compiler){
    yap_close_handle(compiler.frontend_handle);
    yap_close_handle(compiler.backend_handle);
    yap_close_handle(compiler.semantic_handle);
}

void yap_compiler_load_incremental_component(yap_compiler* compiler, const char* path, const char* name){
    (void)compiler; (void)path; (void)name;
    // Placeholder for future incremental expansion callbacks
}

void yap_compiler_load_frontend_component(yap_compiler* compiler, const char* path, const char* name){
    compiler->frontend_handle = yap_get_handle(path);
    compiler->frontend.parse = load_func_dynamically(compiler->frontend_handle, name, yap_parse_fn, "yap_parse");
    compiler->frontend.print_error = load_func_dynamically(compiler->frontend_handle, name, yap_print_error_fn, "yap_print_error");
}

void yap_compiler_load_backend_component(yap_compiler* compiler, const char* path, const char* name){
    compiler->backend_handle = yap_get_handle(path);
    compiler->backend.init = load_func_dynamically(compiler->backend_handle, name, yap_backend_init_fn, "yap_backend_init");
    compiler->backend.free = load_func_dynamically(compiler->backend_handle, name, yap_backend_free_fn, "yap_backend_free");
    compiler->backend.gen_decl = load_func_dynamically(compiler->backend_handle, name, yap_gen_decl_fn, "yap_gen_decl");
    compiler->backend.emit = load_func_dynamically(compiler->backend_handle, name, yap_emit_fn, "yap_emit");
    compiler->backend.ensure_symbol = load_func_dynamically(compiler->backend_handle, name, yap_ensure_symbol_fn, "yap_c_ensure_symbol");
    compiler->backend.set_macro_name = load_func_dynamically(compiler->backend_handle, name, yap_set_macro_name_fn, "yap_c_set_macro_name");
    compiler->backend.set_macro_loc = load_func_dynamically(compiler->backend_handle, name, yap_set_macro_loc_fn, "yap_c_set_macro_loc");
    compiler->backend.pop_macro_loc = load_func_dynamically(compiler->backend_handle, name, yap_pop_macro_loc_fn, "yap_c_pop_macro_loc");
}

void yap_compiler_load_semantic_component(yap_compiler* compiler, const char* path, const char* name){
    compiler->semantic_handle = yap_get_handle(path);
    compiler->semantic.build = load_func_dynamically(compiler->semantic_handle, name, yap_build_fn, "yap_build");
}

#define OPT_COMPONENT_FLAGS 0x1000

static error_t parse_args(int key, char *arg, struct argp_state *state) {
    yap_args* args = state->input;

    switch(key) {
    case OPT_COMPONENT_FLAGS:
        args->command = "component_flags";
        break;
    case 'o':
        args->output_file = arg;
        break;
    case 'c':
        args->command = "cflags";
        break;
    case 'C':
        args->command = "components_dir";
        break;
    case 'g':
        args->command = "gen_c_bind";
        args->gen_c_bind_header = arg;
        break;
    case 'p':
        args->gen_c_bind_prefix = arg;
        break;
    case 'r':
        args->run = true;
        break;
    case 'b':
        darr_push(args->backend_flags, arg);
        break;
    case 'f':
        darr_push(args->frontend_flags, arg);
        break;
    case 'h':
        args->command = "help";
        break;
    case 's': {
        char* eq = strchr(arg, '=');
        if (!eq || eq == arg || eq[1] == '\0'){
            argp_error(state, "Invalid -s value '%s' (expected -s<component>=<name>, e.g. -sback=yap-c)", arg);
            break;
        }
        size_t key_len = (size_t)(eq - arg);
        char* name = eq + 1;
        if (key_len == 4 && strncmp(arg, "back", 4) == 0)
            args->backend_component = name;
        else if (key_len == 5 && strncmp(arg, "front", 5) == 0)
            args->frontend_component = name;
        else if (key_len == 3 && strncmp(arg, "sem", 3) == 0)
            args->semantic_component = name;
        else
            argp_error(state, "Unknown component selector '-s%.*s' (expected back, front, or sem)", (int)key_len, arg);
        break;
    }
    case ARGP_KEY_ARG:
        if (state->arg_num >= 1)
            argp_usage(state);
        darr_push(args->extra, arg);
        break;
    case ARGP_KEY_END:
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp_option options[] = {
    //{"long_name", 'short_name', "value_name | NULL for no value", flags, "doc string", group},
    {"cflags", 'c', NULL, 0, "Output cflags for components.", 0},
    {"components", 'C', NULL, 0, "Output components path.", 1},
    {"output", 'o', "OUTPUT_FILE", 0, "The path to the result file.", 1},
    {"gen-c-bind", 'g', "HEADER", 0, "Generate C bindings from a header (e.g. \"<stdio.h>\").  -o sets the output directory name.", 3},
    {"prefix", 'p', "PREFIX", 0, "Symbol prefix for generated wrapper library (e.g. \"yap_io_\").", 3},
    {"run", 'r', NULL, 0, "Compile and immediately run the resulting program (executed in-memory via TCC).", 1},
    {"backend-flag", 'b', "FLAG", 0, "Raw flag forwarded to the backend component, e.g. -bO2 for optimization level, -bc to stop after emitting C (copied to ./out), -bcc=clang to pick the C compiler (gcc, clang, tcc supported), -bf=-Wall to forward a raw flag to that compiler. Resolved by the backend, not the core compiler.", 1},
    {"frontend-flag", 'f', "FLAG", 0, "Raw flag forwarded to the frontend component. Resolved by the frontend.", 1},
    {"select-component", 's', "COMPONENT=NAME", 0, "Select which directory under components/ implements a compiler stage, e.g. -sback=yap-c, -sfront=yap-ts, -ssem=yap-semantic.", 1},
    {"component-flags", OPT_COMPONENT_FLAGS, NULL, 0, "List the raw -b/-f flags supported by the currently selected components (also shown under --help).", 1},
    {"help", 'h', NULL, 0, "Give this help list.", 4},
    {0}
};

static char doc[] = "The tool for yap programming language.";
static char args_doc[] = "source file(s)";

static void yap_print_help(yap_args* args){
    printf(aesc_bold_on "Usage:" aesc_reset " yap [OPTION...] %s\n\n", args_doc);
    printf("%s\n\n", doc);

    for (int i = 0; options[i].name != NULL || options[i].key != 0; i++){
        const struct argp_option* opt = &options[i];
        bool has_short = opt->key > 0 && opt->key < 128 && isprint(opt->key);

        printf("  " aesc_bold_on aesc_cyan);
        if (has_short)
            printf("-%c, --%s", opt->key, opt->name);
        else
            printf("    --%s", opt->name);
        if (opt->arg)
            printf("=%s", opt->arg);
        printf(aesc_reset "\n");

        if (opt->doc && opt->doc[0])
            printf("      %s\n", opt->doc);
        printf("\n");
    }

    yap_describe_all_component_flags(stdout, args);
}

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

    bool run_subcommand = false;
    if (argc > 1 && strcmp(argv[1], "run") == 0){
        run_subcommand = true;
        for (int i = 1; i < argc - 1; i++) argv[i] = argv[i + 1];
        argc--;
    }

    //Parse args
    yap_args args = (yap_args){
      .output_file = "a.out",
      .extra = darr_new(char*),
      .command = "compile",
      .run = run_subcommand,
      .backend_flags = darr_new(char*),
      .frontend_flags = darr_new(char*),
      .frontend_component = "yap-ts",
      .backend_component = "yap-c",
      .semantic_component = "yap-semantic"
    };

    int res = argp_parse(&argp, argc, argv, ARGP_NO_HELP, 0, &args);
    if (res){
        fprintf(stderr, "Error while resolving arguments using argp.\n");
        yap_free_args(args);
        return 1;
    }
    // printf("Command: %s\n", args.command);
    //Switch on command
    strus_switch(args.command, "compile"){
        if (darr_len(args.extra) > 0){
            result = compile(args);
            // args freed by yap_free_compiler via compiler.args
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
    }strus_case(args.command, "gen_c_bind"){
        result = yap_gen_c_bind(args);
        yap_free_args(args);
    }strus_case(args.command, "component_flags"){
        yap_describe_all_component_flags(stdout, &args);
        yap_free_args(args);
    }strus_case(args.command, "help"){
        yap_print_help(&args);
        yap_free_args(args);
    }
    return result;
}

