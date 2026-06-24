/** @file bindgen.c
 *  C binding import via libclang.
 *
 *  Walk an already-parsed Clang translation unit, extract exported
 *  functions, close over all reachable canonical types, and push
 *  yap_decl_extern_func entries into ctx.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <clang-c/Index.h>
#include "yap/all.h"
#include "yap/bindgen.h"

// ---------------------------------------------------------------------------
// --gen-c-bind CLI command: parse a C header, write binds.yap to outdir.
// ---------------------------------------------------------------------------

int yap_gen_c_bind(yap_args args) {
    if (!args.gen_c_bind_header || !args.gen_c_bind_header[0]) {
        fprintf(stderr, "Error: --gen-c-bind requires a header argument (e.g. \"<stdio.h>\")\n");
        return 1;
    }
    const char *header = args.gen_c_bind_header;
    const char *outdir = args.output_file ? args.output_file : "binds";
    yap_log("gen-c-bind: header=%s  output_dir=%s", header, outdir);

    // 1. Create output directory
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", outdir);
    if (system(cmd) != 0) {
        fprintf(stderr, "Error: failed to create directory '%s'\n", outdir);
        return 1;
    }

    // 2. Write a temp .c file that #includes the header (so libclang can parse it)
    FILE *tmp = fopen("/tmp/yap_bindgen_input.c", "w");
    if (!tmp) {
        fprintf(stderr, "Error: failed to create temp file\n");
        return 1;
    }
    if (header[0] == '<')
        fprintf(tmp, "#include %s\n", header);
    else
        fprintf(tmp, "#include \"%s\"\n", header);
    fclose(tmp);

    // 3. Parse with libclang and generate bindings
    FILE *out = fopen("/tmp/yap_bindgen_output.yap", "w");
    if (!out) {
        fprintf(stderr, "Error: failed to create output temp file\n");
        unlink("/tmp/yap_bindgen_input.c");
        return 1;
    }

    // Create a fresh ctx (bindgen needs it for the type table)
    yap_ctx *ctx = yap_ctx_new();

    // On NixOS, clang needs help finding system headers
    const char *glibc_inc = NULL;
    char glibc_arg[512];
    FILE *fp = popen("ls -d /nix/store/*-glibc-*-dev/include 2>/dev/null | head -1", "r");
    if (fp) {
        if (fgets(glibc_arg, sizeof(glibc_arg), fp)) {
            glibc_arg[strcspn(glibc_arg, "\n")] = 0;
            glibc_inc = glibc_arg;
        }
        pclose(fp);
    }

    const char *full_argv[4];
    int full_argc = 0;
    full_argv[full_argc++] = "-x";
    full_argv[full_argc++] = "c";
    if (glibc_inc) {
        full_argv[full_argc++] = "-isystem";
        full_argv[full_argc++] = glibc_inc;
    }

    yap_bindgen_import(ctx, "/tmp/yap_bindgen_input.c", full_argc, full_argv);

    // 4. Write the generated decls to the output .yap file
    fprintf(out, "// C bindings generated from %s\n// (bindgen not yet fully implemented)\n\n", header);
    for (size_t i = 0; i < darr_len(ctx->semantic_decls); i++) {
        yap_decl *d = &ctx->semantic_decls[i];
        if (d->kind == yap_decl_func && d->func_decl.body.kind == yap_block_error) {
            fprintf(out, "extern fn %s(...);  // imported\n", d->func_decl.name);
        }
    }
    fclose(out);

    // 5. Move output file to the target directory
    snprintf(cmd, sizeof(cmd), "mv /tmp/yap_bindgen_output.yap %s/binds.yap", outdir);
    int sysret = system(cmd); (void)sysret;

    // Cleanup
    yap_ctx_free(*ctx);
    free(ctx);
    unlink("/tmp/yap_bindgen_input.c");

    printf("Generated %s/binds.yap\n", outdir);
    return 0;
}

// ---------------------------------------------------------------------------
// internal helpers
// ---------------------------------------------------------------------------

/** Return a string copy of the CXType's spelling (caller must free). */
static char *cx_spelling(CXType t) {
  CXString s = clang_getTypeSpelling(t);
  const char *c = clang_getCString(s);
  char *r = c ? strdup(c) : NULL;
  clang_disposeString(s);
  return r;
}

// TODO: scaffolding for the full algorithm — unused until process_type is implemented
/** Return a string copy of a cursor's USR (caller must free). */
__attribute__((unused))
static char *cx_usr(CXCursor c) {
  CXString s = clang_getCursorUSR(c);
  const char *u = clang_getCString(s);
  char *r = u ? strdup(u) : NULL;
  clang_disposeString(s);
  return r;
}

/** Simple set of canonical-type USR strings so we don't visit twice. */
typedef struct {
  char   **usrs;
  size_t   len;
  size_t   cap;
} usr_set;

__attribute__((unused))
static bool usr_set_has(usr_set *s, const char *usr) {
  for (size_t i = 0; i < s->len; i++)
    if (!strcmp(s->usrs[i], usr)) return true;
  return false;
}

__attribute__((unused))
static void usr_set_add(usr_set *s, char *usr) {
  if (s->len == s->cap) {
    s->cap = s->cap ? s->cap * 2 : 8;
    s->usrs = realloc(s->usrs, s->cap * sizeof(char *));
  }
  s->usrs[s->len++] = usr;
}

__attribute__((unused))
static void usr_set_free(usr_set *s) {
  for (size_t i = 0; i < s->len; i++) free(s->usrs[i]);
  free(s->usrs);
  *s = (usr_set){0};
}

// ---------------------------------------------------------------------------
// walker helpers  (forwards — full implementation in later commit)
// ---------------------------------------------------------------------------

static void collect_exported(CXTranslationUnit tu, CXCursor cursor,
                             yap_ctx *ctx, usr_set *seen);
// TODO: implement process_type — the core type-graph walker
#if 0
__attribute__((unused))
static void process_type(CXType t, yap_ctx *ctx, usr_set *seen);
#endif

// ---------------------------------------------------------------------------
// public API
// ---------------------------------------------------------------------------

void yap_bindgen_import(yap_ctx *ctx,
                        const char *header_path,
                        int clang_argc,
                        const char **clang_argv) {
  if (!ctx || !header_path) return;

  CXIndex idx = clang_createIndex(0, 0);
  CXTranslationUnit tu = clang_parseTranslationUnit(
      idx, header_path, clang_argv, clang_argc, NULL, 0,
      CXTranslationUnit_SkipFunctionBodies |
      CXTranslationUnit_DetailedPreprocessingRecord);

  if (!tu) {
    yap_log("bindgen: failed to parse %s", header_path);
    clang_disposeIndex(idx);
    return;
  }

  unsigned diags = clang_getNumDiagnostics(tu);
  for (unsigned i = 0; i < diags; i++) {
    CXDiagnostic d = clang_getDiagnostic(tu, i);
    CXString ds = clang_formatDiagnostic(d, clang_defaultDiagnosticDisplayOptions());
    yap_log("bindgen diag: %s", clang_getCString(ds));
    clang_disposeString(ds);
    clang_disposeDiagnostic(d);
  }

  usr_set seen = {0};
  CXCursor root = clang_getTranslationUnitCursor(tu);
  collect_exported(tu, root, ctx, &seen);
  usr_set_free(&seen);

  clang_disposeTranslationUnit(tu);
  clang_disposeIndex(idx);
}

// ---------------------------------------------------------------------------
// walker stubs (just logs what it finds for now)
// ---------------------------------------------------------------------------

/** Visit every top-level child, looking for FunctionDecl with external linkage. */
static enum CXChildVisitResult bindgen_visitor(CXCursor c, CXCursor parent, CXClientData cd) {
  (void)parent;
  (void)cd;
  if (clang_getCursorKind(c) == CXCursor_FunctionDecl) {
    enum CXLinkageKind lk = clang_getCursorLinkage(c);
    if (lk == CXLinkage_External || lk == CXLinkage_UniqueExternal) {
      char *spelling = cx_spelling(clang_getCursorResultType(c));
      CXString cs = clang_getCursorSpelling(c);
      const char *fname = clang_getCString(cs);
      yap_log("found exported function: %s  (returns %s)",
              fname ? fname : "(anon)", spelling ? spelling : "(null)");
      clang_disposeString(cs);
      free(spelling);
    }
  }
  return CXChildVisit_Recurse;
}

static void collect_exported(CXTranslationUnit tu, CXCursor cursor,
                             yap_ctx *ctx, usr_set *seen) {
  struct { CXTranslationUnit tu; yap_ctx *ctx; usr_set *seen; }
      data = {tu, ctx, seen};
  clang_visitChildren(cursor, bindgen_visitor, &data);
}
