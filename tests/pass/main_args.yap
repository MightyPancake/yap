// main's C entry point is always called as (argc, argv); when main declares
// a single 'byte@[] args' parameter, the backend binds it from argc/argv
// (see yap_gen_func_decl in codegen.c). argv[0] (the program path) is always
// present, even with no user-supplied args, so this is testable without the
// harness passing any extra CLI args.
i32 fn main(byte@[] args) {
    i32 code = 0;

    if (args.len == 0) { code = code + 1; }
    if (args:[0]:[0] == 0) { code = code + 1; }

    ret code;
}
