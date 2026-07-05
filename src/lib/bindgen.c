#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <clang-c/Index.h>
#include "yap/all.h"
#include "yap/bindgen.h"

/* Forward decl: inlines anonymous struct/union types in field output */
static void print_type_inline(yap_ctx *ctx, FILE *out, yap_type_id id);

typedef struct {
    char *c_name;
    char *c_ret_spelling;
    darr(char*) c_param_types;
    darr(char*) c_param_names;
} bindgen_wrapper_entry;

static darr(bindgen_wrapper_entry) s_wrapper_entries = NULL;

static bool is_reserved_name(const char *name) {
    return name && name[0] == '_' && name[1] == '_';
}

static bool type_uses_reserved_r(yap_ctx *ctx, yap_type_id id, int depth) {
    if (depth > 16) return false;
    yap_type *t = yap_ctx_get_type(ctx, id);
    if (!t) return false;
    if (t->kind == yap_type_struct) {
        if (t->structure.name && is_reserved_name(t->structure.name)) return true;
        for (size_t i = 0; t->structure.fields && i < darr_len(t->structure.fields); i++)
            if (type_uses_reserved_r(ctx, t->structure.fields[i].type, depth + 1)) return true;
        return false;
    }
    if (t->kind == yap_type_union) {
        if (t->uni.name && is_reserved_name(t->uni.name)) return true;
        for (size_t i = 0; t->uni.variants && i < darr_len(t->uni.variants); i++)
            if (type_uses_reserved_r(ctx, t->uni.variants[i].type, depth + 1)) return true;
        return false;
    }
    if (t->kind == yap_type_enum && t->enumeration.name) return is_reserved_name(t->enumeration.name);
    if (t->kind == yap_type_ptr) return type_uses_reserved_r(ctx, t->pointer_type, depth + 1);
    return false;
}

static bool type_uses_reserved(yap_ctx *ctx, yap_type_id id) {
    return type_uses_reserved_r(ctx, id, 0);
}

int yap_gen_c_bind(yap_args args) {
    if (!args.gen_c_bind_header || !args.gen_c_bind_header[0]) {
        fprintf(stderr, "Error: --gen-c-bind requires a header argument\n");
        return 1;
    }
    const char *header = args.gen_c_bind_header;
    const char *outfile = args.output_file ? args.output_file : "binds.yap";
    yap_log("gen-c-bind: header=%s  output=%s", header, outfile);

    FILE *tmp = fopen("/tmp/yap_bindgen_input.c", "w");
    if (!tmp) { return 1; }
    if (header[0] == '<') fprintf(tmp, "#include %s\n", header);
    else fprintf(tmp, "#include \"%s\"\n", header);
    fclose(tmp);

    FILE *out = fopen(outfile, "w");
    if (!out) { fprintf(stderr, "Error: failed to open '%s' for writing\n", outfile); unlink("/tmp/yap_bindgen_input.c"); return 1; }

    yap_ctx *ctx = yap_ctx_new();

    bool gen_wrapper = args.gen_c_bind_prefix && args.gen_c_bind_prefix[0];
    if (gen_wrapper) s_wrapper_entries = darr_new(bindgen_wrapper_entry);

    const char *glibc_inc = NULL;
    char glibc_arg[512];
    FILE *fp = popen("ls -d /nix/store/*-glibc-*-dev/include 2>/dev/null | head -1", "r");
    if (fp) { if (fgets(glibc_arg, sizeof(glibc_arg), fp)) { glibc_arg[strcspn(glibc_arg, "\n")] = 0; glibc_inc = glibc_arg; } pclose(fp); }

    const char *full_argv[4]; int full_argc = 0;
    full_argv[full_argc++] = "-x"; full_argv[full_argc++] = "c";
    if (glibc_inc) { full_argv[full_argc++] = "-isystem"; full_argv[full_argc++] = glibc_inc; }

    yap_bindgen_import(ctx, "/tmp/yap_bindgen_input.c", full_argc, full_argv);

    fprintf(out, "// C bindings from %s\n\n", header);

    /* forward declarations first */
    for (size_t i = 0; i < darr_len(ctx->types); i++) {
        yap_type *typ = &ctx->types[i];
        if (typ->kind == yap_type_struct && typ->structure.name && !darr_len(typ->structure.fields) && !is_reserved_name(typ->structure.name))
            fprintf(out, "type %s\n", typ->structure.name);
        else if (typ->kind == yap_type_union && typ->uni.name && !darr_len(typ->uni.variants) && !is_reserved_name(typ->uni.name))
            fprintf(out, "type %s\n", typ->uni.name);
    }
    fprintf(out, "\n");

    /* full type definitions */
    for (size_t i = 0; i < darr_len(ctx->types); i++) {
        yap_type *typ = &ctx->types[i];
        if (typ->kind == yap_type_struct) {
            if (!typ->structure.name || !darr_len(typ->structure.fields) || is_reserved_name(typ->structure.name)) continue;
            bool has_reserved = false;
            for (size_t j = 0; j < darr_len(typ->structure.fields); j++)
                if (type_uses_reserved(ctx, typ->structure.fields[j].type)) { has_reserved = true; break; }
            if (has_reserved) continue;
            fprintf(out, "struct %s {\n", typ->structure.name);
            for (size_t j = 0; j < darr_len(typ->structure.fields); j++) {
                fprintf(out, "    ");
                print_type_inline(ctx, out, typ->structure.fields[j].type);
                fprintf(out, " %s,\n", typ->structure.fields[j].name ? typ->structure.fields[j].name : "_");
            }
            fprintf(out, "}\n\n");
        } else if (typ->kind == yap_type_union) {
            if (!typ->uni.name || !darr_len(typ->uni.variants) || is_reserved_name(typ->uni.name)) continue;
            bool has_reserved_u = false;
            for (size_t j = 0; j < darr_len(typ->uni.variants); j++)
                if (type_uses_reserved(ctx, typ->uni.variants[j].type)) { has_reserved_u = true; break; }
            if (has_reserved_u) continue;
            fprintf(out, "union %s {\n", typ->uni.name);
            for (size_t j = 0; j < darr_len(typ->uni.variants); j++) {
                fprintf(out, "    ");
                print_type_inline(ctx, out, typ->uni.variants[j].type);
                fprintf(out, " %s,\n", typ->uni.variants[j].name ? typ->uni.variants[j].name : "_");
            }
            fprintf(out, "}\n\n");
        } else if (typ->kind == yap_type_enum) {
            if (!typ->enumeration.name || is_reserved_name(typ->enumeration.name)
                || strchr(typ->enumeration.name, '(') || strchr(typ->enumeration.name, '/')) continue;
            fprintf(out, "enum %s {\n", typ->enumeration.name);
            for (size_t j = 0; j < darr_len(typ->enumeration.variants); j++)
                fprintf(out, "    %s,\n", typ->enumeration.variants[j].name);
            fprintf(out, "}\n\n");
        }
    }

    /* functions */
    for (size_t i = 0; i < darr_len(ctx->semantic_decls); i++) {
        yap_decl *d = &ctx->semantic_decls[i];
        if (d->kind == yap_decl_func_def && d->func_decl.body.kind == yap_block_none) {
            if (is_reserved_name(d->func_decl.name)) continue;
            if (type_uses_reserved(ctx, d->func_decl.ret_typ)) continue;
            bool skip = false;
            for (size_t j = 0; j < darr_len(d->func_decl.args); j++)
                if (type_uses_reserved(ctx, d->func_decl.args[j].type)) { skip = true; break; }
            if (skip) continue;
            print_type_inline(ctx, out, d->func_decl.ret_typ);
            fprintf(out, " fn %s(", d->func_decl.name);
            for (size_t j = 0; j < darr_len(d->func_decl.args); j++) {
                if (j > 0) fprintf(out, ", ");
                yap_func_arg *arg = &d->func_decl.args[j];
                print_type_inline(ctx, out, arg->type);
                if (arg->name && arg->name[0]) fprintf(out, " %s", arg->name);
                else fprintf(out, " _arg%zu", j);
            }
            fprintf(out, ");\n");
        }
    }
    fclose(out);

    if (gen_wrapper) {
        const char *prefix = args.gen_c_bind_prefix;

        char outdir[PATH_MAX / 2];
        strncpy(outdir, outfile, sizeof(outdir) - 1);
        outdir[sizeof(outdir) - 1] = '\0';
        char *last_slash = strrchr(outdir, '/');
        if (last_slash) *(last_slash + 1) = '\0';
        else strcpy(outdir, "./");

        char libname[256];
        strncpy(libname, prefix, sizeof(libname) - 1);
        libname[sizeof(libname) - 1] = '\0';
        size_t plen = strlen(libname);
        if (plen > 0 && libname[plen - 1] == '_') libname[plen - 1] = '\0';

        char wrapper_path[PATH_MAX];
        snprintf(wrapper_path, sizeof(wrapper_path), "%swrapper.c", outdir);
        FILE *wf = fopen(wrapper_path, "w");
        if (!wf) { fprintf(stderr, "Error: failed to open '%s'\n", wrapper_path); goto wrapper_cleanup; }

        if (header[0] == '<') fprintf(wf, "#include %s\n\n", header);
        else fprintf(wf, "#include \"%s\"\n\n", header);

        for (size_t i = 0; i < darr_len(ctx->semantic_decls); i++) {
            yap_decl *d = &ctx->semantic_decls[i];
            if (d->kind != yap_decl_func_def || d->func_decl.body.kind != yap_block_none) continue;
            if (is_reserved_name(d->func_decl.name)) continue;
            if (type_uses_reserved(ctx, d->func_decl.ret_typ)) continue;
            bool skip = false;
            for (size_t j = 0; j < darr_len(d->func_decl.args); j++)
                if (type_uses_reserved(ctx, d->func_decl.args[j].type)) { skip = true; break; }
            if (skip) continue;

            bindgen_wrapper_entry *we = &s_wrapper_entries[i];

            bool duplicate = false;
            for (size_t k = 0; k < i; k++) {
                yap_decl *dk = &ctx->semantic_decls[k];
                if (dk->kind == yap_decl_func_def && dk->func_decl.body.kind == yap_block_none
                    && dk->func_decl.name && strcmp(dk->func_decl.name, d->func_decl.name) == 0) {
                    duplicate = true; break;
                }
            }
            if (duplicate) continue;

            bool has_fnptr = strstr(we->c_ret_spelling, "(*)") != NULL;
            for (size_t j = 0; !has_fnptr && j < darr_len(we->c_param_types); j++)
                if (strstr(we->c_param_types[j], "(*)")) has_fnptr = true;
            if (has_fnptr) {
                yap_log("bindgen: skipping wrapper for '%s' (function pointer type)", we->c_name);
                continue;
            }

            bool is_void = strcmp(we->c_ret_spelling, "void") == 0;

            fprintf(wf, "__attribute__((visibility(\"default\")))\n");
            fprintf(wf, "%s %s%s(", we->c_ret_spelling, prefix, we->c_name);
            if (darr_len(we->c_param_types) == 0) fprintf(wf, "void");
            for (size_t j = 0; j < darr_len(we->c_param_types); j++) {
                if (j > 0) fprintf(wf, ", ");
                fprintf(wf, "%s %s", we->c_param_types[j], we->c_param_names[j]);
            }
            fprintf(wf, ") {\n    %s%s(", is_void ? "" : "return ", we->c_name);
            for (size_t j = 0; j < darr_len(we->c_param_names); j++) {
                if (j > 0) fprintf(wf, ", ");
                fprintf(wf, "%s", we->c_param_names[j]);
            }
            fprintf(wf, ");\n}\n\n");
        }
        fclose(wf);
        printf("Generated %s\n", wrapper_path);

        char obj_path[PATH_MAX], so_path[PATH_MAX], a_path[PATH_MAX], cmd[PATH_MAX * 3];
        snprintf(obj_path, sizeof(obj_path), "%swrapper.o", outdir);
        snprintf(so_path, sizeof(so_path), "%slib%s.so", outdir, libname);
        snprintf(a_path, sizeof(a_path), "%slib%s.a", outdir, libname);

        snprintf(cmd, sizeof(cmd), "gcc -fPIC -fvisibility=hidden -c \"%s\" -o \"%s\"", wrapper_path, obj_path);
        if (system(cmd) != 0) { fprintf(stderr, "Error: failed to compile wrapper\n"); goto wrapper_cleanup; }

        snprintf(cmd, sizeof(cmd), "ar rcs \"%s\" \"%s\"", a_path, obj_path);
        if (system(cmd) != 0) { fprintf(stderr, "Error: failed to create static library\n"); goto wrapper_cleanup; }

        snprintf(cmd, sizeof(cmd), "gcc -shared -o \"%s\" \"%s\"", so_path, obj_path);
        if (system(cmd) != 0) { fprintf(stderr, "Error: failed to create shared library\n"); goto wrapper_cleanup; }

        unlink(obj_path);
        printf("Generated %s\n", a_path);
        printf("Generated %s\n", so_path);

wrapper_cleanup:
        for (size_t i = 0; i < darr_len(s_wrapper_entries); i++) {
            free(s_wrapper_entries[i].c_name);
            free(s_wrapper_entries[i].c_ret_spelling);
            for (size_t j = 0; j < darr_len(s_wrapper_entries[i].c_param_types); j++) {
                free(s_wrapper_entries[i].c_param_types[j]);
                free(s_wrapper_entries[i].c_param_names[j]);
            }
            darr_free(s_wrapper_entries[i].c_param_types);
            darr_free(s_wrapper_entries[i].c_param_names);
        }
        darr_free(s_wrapper_entries);
        s_wrapper_entries = NULL;
    }

    yap_ctx_free(*ctx); free(ctx); unlink("/tmp/yap_bindgen_input.c");
    printf("Generated %s\n", outfile);
    return 0;
}

/* ===== type-graph walker ===== */

static yap_type_id process_type(yap_ctx *ctx, CXType t);

/* Print a type ID to the output file, inlining anonymous structs/unions */
static void print_type_inline(yap_ctx *ctx, FILE *out, yap_type_id id) {
  yap_type *typ = yap_ctx_get_type(ctx, id);
  if (!typ) { fprintf(out, "(error)"); return; }
  if (typ->kind == yap_type_struct && !typ->structure.name) {
    fprintf(out, "struct {\n");
    for (size_t j = 0; j < darr_len(typ->structure.fields); j++) {
      fprintf(out, "        ");
      print_type_inline(ctx, out, typ->structure.fields[j].type);
      fprintf(out, " %s,\n", typ->structure.fields[j].name ? typ->structure.fields[j].name : "_");
    }
    fprintf(out, "    }");
  } else if (typ->kind == yap_type_union && !typ->uni.name) {
    fprintf(out, "union {\n");
    for (size_t j = 0; j < darr_len(typ->uni.variants); j++) {
      fprintf(out, "        ");
      print_type_inline(ctx, out, typ->uni.variants[j].type);
      fprintf(out, " %s,\n", typ->uni.variants[j].name ? typ->uni.variants[j].name : "_");
    }
    fprintf(out, "    }");
  } else if (typ->kind == yap_type_struct && typ->structure.name) {
    fprintf(out, "%s", typ->structure.name);
  } else if (typ->kind == yap_type_union && typ->uni.name) {
    fprintf(out, "%s", typ->uni.name);
  } else if (typ->kind == yap_type_enum && typ->enumeration.name) {
    fprintf(out, "%s", typ->enumeration.name);
  } else if (typ->kind == yap_type_ptr) {
    print_type_inline(ctx, out, typ->pointer_type);
    fprintf(out, "@");
  } else if (typ->kind == yap_type_func) {
    yap_type *ret_typ = yap_ctx_get_type(ctx, typ->func.return_type);
    if (ret_typ) { fprintf(out, "("); print_type_inline(ctx, out, typ->func.return_type); fprintf(out, " fn"); }
    else fprintf(out, "(fn");
    for (size_t i = 0; i < darr_len(typ->func.args); i++) {
      fprintf(out, " "); print_type_inline(ctx, out, typ->func.args[i]);
      if (i != darr_len(typ->func.args) - 1) fprintf(out, ",");
    }
    fprintf(out, ")");
  } else {
    char *tstr = yap_ctx_type_id_to_string(ctx, id);
    fprintf(out, "%s", tstr);
    free(tstr);
  }
}

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
      bool is_anon = false;
      const char *tn = (cname && *cname) ? cname : NULL;
      if (!tn) { clang_disposeString(ns); CXString sp = clang_getTypeSpelling(t); tn = clang_getCString(sp);
        if (!tn || !*tn) { clang_disposeString(sp); return ctx->internal_error_type_id; }
        ns = sp; }
      // Sanitize: detect anonymous/file-path type names
      if (strstr(tn, "/nix/store/") || strstr(tn, "(unnamed") || strstr(tn, "unnamed at"))
        { is_anon = true; tn = "anon"; }
      // Check by clang name first, then by mangle name
      yap_type_id existing = yap_ctx_get_type_id_by_name(ctx, (char*)tn);
      if (existing) { clang_disposeString(ns); return existing; }

      // For anonymous types, generate a unique c_name for mangling.
      // The .name stays NULL so the output loop can detect and skip it.
      static int anon_counter = 0;
      char *anon_c_name = NULL;
      if (is_anon) {
        anon_c_name = yap_ctx_strus_newf(ctx, "__anon_%s_%d", is_union ? "union" : "struct", anon_counter++);
      }

      // Store opaque FIRST to break recursion.
      yap_type opaque_ty = is_union
        ? (yap_type){ .kind = yap_type_union, .uni = { .name = is_anon ? NULL : strdup(tn), .c_name = anon_c_name, .variants = darr_new(yap_struct_field) } }
        : (yap_type){ .kind = yap_type_struct, .structure = { .name = is_anon ? NULL : strdup(tn), .c_name = anon_c_name, .fields = darr_new(yap_struct_field) } };
      yap_type_id opaque_id = is_anon
        ? yap_ctx_push_type(ctx, opaque_ty)
        : yap_ctx_push_named_type(ctx, (char*)tn, NULL, opaque_ty);
      if (anon_c_name) {
        // Also register under the generated c_name for lookups
        yap_named_type anon_named = { .id = opaque_id, .name = anon_c_name, .c_name = NULL };
        hashmap_set(ctx->named_types, &anon_named);
      }

      // Now walk fields ; self-references will find the registered name
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
  CXType func_t = clang_getCursorType(c);
  if (clang_isFunctionTypeVariadic(func_t)) {
    yap_log("bindgen: skipping variadic function '%s'", fname);
    clang_disposeString(cs); return;
  }
  CXType ret_t = clang_getCursorResultType(c); yap_type_id ret_id = process_type(ctx, ret_t);
  darr(yap_func_arg) args = darr_new(yap_func_arg);
  int n = clang_Cursor_getNumArguments(c);

  bool collect_wrapper = (s_wrapper_entries != NULL);
  bindgen_wrapper_entry wentry = {0};
  if (collect_wrapper) {
    CXString ret_sp = clang_getTypeSpelling(ret_t);
    wentry.c_name = strdup(fname);
    wentry.c_ret_spelling = strdup(clang_getCString(ret_sp));
    wentry.c_param_types = darr_new(char*);
    wentry.c_param_names = darr_new(char*);
    clang_disposeString(ret_sp);
  }

  for (int i = 0; i < n; i++) {
    CXCursor param = clang_Cursor_getArgument(c, i);
    CXType pt = clang_getCursorType(param); yap_type_id pid = process_type(ctx, pt);
    CXString ps = clang_getCursorSpelling(param); const char *pn = clang_getCString(ps);
    yap_func_arg arg = { .kind = yap_func_arg_valid, .name = pn ? strdup(pn) : NULL, .type = pid };
    darr_push(args, arg);

    if (collect_wrapper) {
      CXType canon_pt = clang_getCanonicalType(pt);
      char *type_str;
      if (canon_pt.kind == CXType_ConstantArray || canon_pt.kind == CXType_IncompleteArray) {
        CXType elem = clang_getArrayElementType(canon_pt);
        CXString elem_sp = clang_getTypeSpelling(elem);
        size_t len = strlen(clang_getCString(elem_sp)) + 3;
        type_str = malloc(len);
        snprintf(type_str, len, "%s *", clang_getCString(elem_sp));
        clang_disposeString(elem_sp);
      } else {
        CXString pt_sp = clang_getTypeSpelling(pt);
        type_str = strdup(clang_getCString(pt_sp));
        clang_disposeString(pt_sp);
      }
      darr_push(wentry.c_param_types, type_str);
      char *pname;
      if (pn && *pn) pname = strdup(pn);
      else { char buf[32]; snprintf(buf, sizeof(buf), "_arg%d", i); pname = strdup(buf); }
      darr_push(wentry.c_param_names, pname);
    }

    clang_disposeString(ps);
  }
  yap_block nobody = { .kind = yap_block_none };
  yap_func_decl fd = { .name = strdup(fname), .args = args, .ret_typ = ret_id, .body = nobody };
  yap_decl decl = {0};
  decl.kind = yap_decl_func_def;
  decl.func_decl = fd;
  darr_push(ctx->semantic_decls, decl);
  if (collect_wrapper) darr_push(s_wrapper_entries, wentry);
  clang_disposeString(cs);
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
