#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <clang-c/Index.h>
#include "yap/all.h"
#include "yap/bindgen.h"

int yap_gen_c_bind(yap_args args) {
    if (!args.gen_c_bind_header || !args.gen_c_bind_header[0]) {
        fprintf(stderr, "Error: --gen-c-bind requires a header argument\n");
        return 1;
    }
    const char *header = args.gen_c_bind_header;
    const char *outdir = args.output_file ? args.output_file : "binds";
    yap_log("gen-c-bind: header=%s  output_dir=%s", header, outdir);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", outdir);
    if (system(cmd) != 0) { fprintf(stderr, "Error: failed to create directory\n"); return 1; }

    FILE *tmp = fopen("/tmp/yap_bindgen_input.c", "w");
    if (!tmp) { return 1; }
    if (header[0] == '<') fprintf(tmp, "#include %s\n", header);
    else fprintf(tmp, "#include \"%s\"\n", header);
    fclose(tmp);

    FILE *out = fopen("/tmp/yap_bindgen_output.yap", "w");
    if (!out) { unlink("/tmp/yap_bindgen_input.c"); return 1; }

    yap_ctx *ctx = yap_ctx_new();

    const char *glibc_inc = NULL;
    char glibc_arg[512];
    FILE *fp = popen("ls -d /nix/store/*-glibc-*-dev/include 2>/dev/null | head -1", "r");
    if (fp) { if (fgets(glibc_arg, sizeof(glibc_arg), fp)) { glibc_arg[strcspn(glibc_arg, "\n")] = 0; glibc_inc = glibc_arg; } pclose(fp); }

    const char *full_argv[4]; int full_argc = 0;
    full_argv[full_argc++] = "-x"; full_argv[full_argc++] = "c";
    if (glibc_inc) { full_argv[full_argc++] = "-isystem"; full_argv[full_argc++] = glibc_inc; }

    yap_bindgen_import(ctx, "/tmp/yap_bindgen_input.c", full_argc, full_argv);

    fprintf(out, "// C bindings from %s\n\n", header);

    /* types */
    for (size_t i = 0; i < darr_len(ctx->types); i++) {
        yap_type *typ = &ctx->types[i];
        if (typ->kind == yap_type_struct) {
            fprintf(out, "struct %s {\n", typ->structure.name ? typ->structure.name : "(anon)");
            for (size_t j = 0; j < darr_len(typ->structure.fields); j++) {
                char *tstr = yap_ctx_type_id_to_string(ctx, typ->structure.fields[j].type);
                fprintf(out, "    %s: %s,\n", typ->structure.fields[j].name ? typ->structure.fields[j].name : "_", tstr);
                free(tstr);
            }
            fprintf(out, "}\n\n");
        } else if (typ->kind == yap_type_union) {
            fprintf(out, "union %s {\n", typ->uni.name ? typ->uni.name : "(anon)");
            for (size_t j = 0; j < darr_len(typ->uni.variants); j++) {
                char *tstr = yap_ctx_type_id_to_string(ctx, typ->uni.variants[j].type);
                fprintf(out, "    %s: %s,\n", typ->uni.variants[j].name ? typ->uni.variants[j].name : "_", tstr);
                free(tstr);
            }
            fprintf(out, "}\n\n");
        } else if (typ->kind == yap_type_enum) {
            fprintf(out, "enum %s {\n", typ->enumeration.name ? typ->enumeration.name : "(anon)");
            for (size_t j = 0; j < darr_len(typ->enumeration.variants); j++)
                fprintf(out, "    %s,\n", typ->enumeration.variants[j].name);
            fprintf(out, "}\n\n");
        }
    }

    /* functions */
    for (size_t i = 0; i < darr_len(ctx->semantic_decls); i++) {
        yap_decl *d = &ctx->semantic_decls[i];
        if (d->kind == yap_decl_func && d->func_decl.body.kind == yap_block_none) {
            char *ret = yap_ctx_type_id_to_string(ctx, d->func_decl.ret_typ);
            fprintf(out, "%s fn %s(", ret, d->func_decl.name); free(ret);
            for (size_t j = 0; j < darr_len(d->func_decl.args); j++) {
                if (j > 0) fprintf(out, ", ");
                yap_func_arg *arg = &d->func_decl.args[j];
                char *tstr = yap_ctx_type_id_to_string(ctx, arg->type);
                if (arg->name && arg->name[0]) fprintf(out, "%s: %s", arg->name, tstr);
                else fprintf(out, "%s", tstr);
                free(tstr);
            }
            ret = yap_ctx_type_id_to_string(ctx, d->func_decl.ret_typ);
            fprintf(out, ") %s;\n", ret); free(ret);
        }
    }
    fclose(out);

    snprintf(cmd, sizeof(cmd), "mv /tmp/yap_bindgen_output.yap %s/binds.yap", outdir);
    int sysret = system(cmd); (void)sysret;
    yap_ctx_free(*ctx); free(ctx); unlink("/tmp/yap_bindgen_input.c");
    printf("Generated %s/binds.yap\n", outdir);
    return 0;
}

/* ===== type-graph walker ===== */

static char *cx_usr(CXCursor c) {
  CXString s = clang_getCursorUSR(c); const char *u = clang_getCString(s);
  char *r = u ? strdup(u) : NULL; clang_disposeString(s); return r;
}

static yap_type_id process_type(yap_ctx *ctx, CXType t);

static enum CXChildVisitResult sf_visitor(CXCursor c, CXCursor parent, CXClientData cd) {
  (void)parent;
  struct { yap_struct_type *st; yap_ctx *ctx; } *data = (void*)cd;
  if (clang_getCursorKind(c) == CXCursor_FieldDecl) {
    CXType ft = clang_getCursorType(c);
    yap_type_id fid = process_type(data->ctx, ft);
    CXString ns = clang_getCursorSpelling(c);
    yap_struct_field sf = { .kind = yap_struct_field_valid,
      .name = strdup(clang_getCString(ns)), .type = fid, .default_value = NULL };
    clang_disposeString(ns); darr_push(data->st->fields, sf);
  }
  return CXChildVisit_Continue;
}

static enum CXChildVisitResult ef_visitor(CXCursor c, CXCursor parent, CXClientData cd) {
  (void)parent; yap_enum_type *et = cd;
  if (clang_getCursorKind(c) == CXCursor_EnumConstantDecl) {
    CXString ns = clang_getCursorSpelling(c);
    yap_enum_variant ev = { .name = strdup(clang_getCString(ns)), .value = NULL };
    clang_disposeString(ns); darr_push(et->variants, ev);
  }
  return CXChildVisit_Continue;
}

static yap_type_id process_type(yap_ctx *ctx, CXType t) {
  CXType canon = clang_getCanonicalType(t);
  switch (canon.kind) {
    case CXType_Void:  return ctx->void_type_id;
    case CXType_Bool:  return ctx->bool_type_id;
    case CXType_Int: case CXType_UInt: case CXType_Short: case CXType_UShort:
      return ctx->int_type_id;
    case CXType_Long: case CXType_ULong: case CXType_LongLong: case CXType_ULongLong:
      { yap_type_id id = yap_ctx_get_type_id_by_name(ctx, "i64"); return id ? id : ctx->internal_error_type_id; }
    case CXType_Float: return ctx->float_type_id;
    case CXType_Double: case CXType_LongDouble:
      { yap_type_id id = yap_ctx_get_type_id_by_name(ctx, "f64"); return id ? id : ctx->internal_error_type_id; }
    case CXType_Char_S: case CXType_Char_U: case CXType_SChar: case CXType_UChar:
      { yap_type_id id = yap_ctx_get_type_id_by_name(ctx, "byte"); return id ? id : ctx->internal_error_type_id; }
    case CXType_Pointer: {
      yap_type_id pointee = process_type(ctx, clang_getPointeeType(canon));
      yap_type ptr = { .kind = yap_type_ptr, .pointer_type = pointee };
      return yap_ctx_insert_type_if_not_exists(ctx, ptr);
    }
    case CXType_ConstantArray: case CXType_IncompleteArray: {
      yap_type_id elt = process_type(ctx, clang_getArrayElementType(canon));
      yap_type ptr = { .kind = yap_type_ptr, .pointer_type = elt };
      return yap_ctx_insert_type_if_not_exists(ctx, ptr);
    }
    case CXType_FunctionProto: {
      yap_type_id ret = process_type(ctx, clang_getResultType(canon));
      int n = clang_getNumArgTypes(canon);
      yap_fn_type fn = { .return_type = ret, .args = darr_new(yap_type_id) };
      for (int i = 0; i < n; i++) darr_push(fn.args, process_type(ctx, clang_getArgType(canon, i)));
      yap_type ft = { .kind = yap_type_func, .func = fn };
      return yap_ctx_insert_type_if_not_exists(ctx, ft);
    }
    case CXType_Record: {
      CXCursor decl = clang_getTypeDeclaration(canon);
      CXString ns = clang_getCursorSpelling(decl);
      const char *cname = clang_getCString(ns);
      bool is_union = (clang_getCursorKind(decl) == CXCursor_UnionDecl);
      const char *tn = (cname && *cname) ? cname : NULL;
      if (!tn) { clang_disposeString(ns); CXString sp = clang_getTypeSpelling(t); tn = clang_getCString(sp);
        if (!tn || !*tn) { clang_disposeString(sp); return ctx->internal_error_type_id; }
        ns = sp; }
      // Sanitize: filter out anonymous/file-path type names
      if (strstr(tn, "/nix/store/") || strstr(tn, "(unnamed") || strstr(tn, "unnamed at"))
        tn = "anon";
      // Check by clang name first then by mangle name
      yap_type_id existing = yap_ctx_get_type_id_by_name(ctx, (char*)tn);
      if (existing) { clang_disposeString(ns); return existing; }

      // Store opaque FIRST to break recursion. Use yap_ctx_push_named_type
      // so the clang name (e.g. "_IO_FILE") is also registered.
      yap_type opaque_ty = is_union
        ? (yap_type){ .kind = yap_type_union, .uni = { .name = strdup(tn), .variants = darr_new(yap_struct_field) } }
        : (yap_type){ .kind = yap_type_struct, .structure = { .name = strdup(tn), .fields = darr_new(yap_struct_field) } };
      yap_type_id opaque_id = yap_ctx_push_named_type(ctx, (char*)tn, NULL, opaque_ty);

      // Now walk fields — self-references will find the registered name
      if (clang_isCursorDefinition(decl)) {
        if (is_union) {
          yap_union_type *ut = &ctx->types[opaque_id].uni;
          ut->variants = darr_new(yap_struct_field);
          struct { yap_struct_type *st; yap_ctx *ctx; } fd = {(yap_struct_type*)ut, ctx};
          clang_visitChildren(decl, sf_visitor, &fd);
        } else {
          yap_struct_type *st = &ctx->types[opaque_id].structure;
          st->fields = darr_new(yap_struct_field);
          struct { yap_struct_type *st; yap_ctx *ctx; } fd = {st, ctx};
          clang_visitChildren(decl, sf_visitor, &fd);
        }
      }
      clang_disposeString(ns);
      return opaque_id;
    }
    case CXType_Enum: {
      CXCursor decl = clang_getTypeDeclaration(canon); CXString ns = clang_getCursorSpelling(decl);
      const char *cname = clang_getCString(ns);
      if (!cname || !*cname) { clang_disposeString(ns); return ctx->internal_error_type_id; }
      yap_type_id ex = yap_ctx_get_type_id_by_name(ctx, (char*)cname);
      if (ex) { clang_disposeString(ns); return ex; }
      yap_enum_type et = { .name = strdup(cname), .variants = darr_new(yap_enum_variant) };
      if (clang_isCursorDefinition(decl)) clang_visitChildren(decl, ef_visitor, &et);
      yap_type ty = { .kind = yap_type_enum, .enumeration = et }; clang_disposeString(ns);
      return yap_ctx_insert_type_if_not_exists(ctx, ty);
    }
    case CXType_Typedef: return process_type(ctx, canon);
    default: return ctx->internal_error_type_id;
  }
}

/* ===== function importer ===== */

static void push_imported_func(yap_ctx *ctx, CXCursor c) {
  CXString cs = clang_getCursorSpelling(c); const char *fname = clang_getCString(cs);
  if (!fname) { clang_disposeString(cs); return; }
  CXType ret_t = clang_getCursorResultType(c); yap_type_id ret_id = process_type(ctx, ret_t);
  darr(yap_func_arg) args = darr_new(yap_func_arg);
  int n = clang_Cursor_getNumArguments(c);
  for (int i = 0; i < n; i++) {
    CXCursor param = clang_Cursor_getArgument(c, i);
    CXType pt = clang_getCursorType(param); yap_type_id pid = process_type(ctx, pt);
    CXString ps = clang_getCursorSpelling(param); const char *pn = clang_getCString(ps);
    yap_func_arg arg = { .kind = yap_func_arg_valid, .name = pn ? strdup(pn) : NULL, .type = pid };
    darr_push(args, arg); clang_disposeString(ps);
  }
  yap_decl decl = { .kind = yap_decl_func,
    .func_decl = { .name = strdup(fname), .args = args, .ret_typ = ret_id,
                    .body = { .kind = yap_block_none } },
    .loc = (yap_loc){0}, .range = (yap_code_range){0} };
  darr_push(ctx->semantic_decls, decl); clang_disposeString(cs);
}

/* ===== walker + public API ===== */

static enum CXChildVisitResult bindgen_visitor(CXCursor c, CXCursor parent, CXClientData cd) {
  (void)parent; yap_ctx *ctx = (void*)cd;
  enum CXCursorKind k = clang_getCursorKind(c);
  if (k == CXCursor_FunctionDecl) {
    enum CXLinkageKind lk = clang_getCursorLinkage(c);
    if (lk == CXLinkage_External || lk == CXLinkage_UniqueExternal) push_imported_func(ctx, c);
  }
  if (k == CXCursor_StructDecl || k == CXCursor_UnionDecl || k == CXCursor_EnumDecl) {
    CXString ns = clang_getCursorSpelling(c);
    if (clang_getCString(ns) && *clang_getCString(ns)) process_type(ctx, clang_getCursorType(c));
    clang_disposeString(ns);
  }
  if (k == CXCursor_TypedefDecl) process_type(ctx, clang_getTypedefDeclUnderlyingType(c));
  return CXChildVisit_Recurse;
}

void yap_bindgen_import(yap_ctx *ctx, const char *header_path, int clang_argc, const char **clang_argv) {
  if (!ctx || !header_path) return;
  CXIndex idx = clang_createIndex(0, 0);
  CXTranslationUnit tu = clang_parseTranslationUnit(idx, header_path, clang_argv, clang_argc, NULL, 0,
      CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_DetailedPreprocessingRecord);
  if (!tu) { yap_log("bindgen: failed to parse %s", header_path); clang_disposeIndex(idx); return; }
  CXCursor root = clang_getTranslationUnitCursor(tu);
  clang_visitChildren(root, bindgen_visitor, ctx);
  clang_disposeTranslationUnit(tu); clang_disposeIndex(idx);
}
