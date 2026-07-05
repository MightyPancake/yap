/** Simple libclang smoke test ; parse stdio.h and print exported functions. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clang-c/Index.h>

static enum CXChildVisitResult visitor(CXCursor c, CXCursor parent, CXClientData cd) {
  (void)parent; (void)cd;
  if (clang_getCursorKind(c) == CXCursor_FunctionDecl) {
    enum CXLinkageKind lk = clang_getCursorLinkage(c);
    if (lk == CXLinkage_External || lk == CXLinkage_UniqueExternal) {
      CXString name = clang_getCursorSpelling(c);
      CXString type = clang_getTypeSpelling(clang_getCursorResultType(c));
      printf("  FUNCTION: %-30s → %s\n", clang_getCString(name), clang_getCString(type));
      clang_disposeString(name);
      clang_disposeString(type);
    }
  }
  return CXChildVisit_Continue;
}

int main(int argc, char **argv) {
  const char *clang_args[32];
  int nargs = 0;

  // On NixOS, clang can't find system headers.  Look for glibc's include dir.
  const char *glibc_inc = NULL;
  // Check two common env vars used by cc-wrapper on NixOS
  if (getenv("NIX_GLIBC_INCLUDE")) {
    glibc_inc = getenv("NIX_GLIBC_INCLUDE");
  } else {
    // Fallback: scan /nix/store for a glibc-dev include dir
    static char buf[512];
    FILE *fp = popen(
        "ls -d /nix/store/*-glibc-*-dev/include 2>/dev/null | head -1", "r");
    if (fp) {
      if (fgets(buf, sizeof(buf), fp)) {
        buf[strcspn(buf, "\n")] = 0;
        glibc_inc = strdup(buf);
      }
      pclose(fp);
    }
  }

  if (glibc_inc) {
    // clang needs -isystem not -I for system headers (suppresses warnings)
    char *tmp = malloc(strlen(glibc_inc) + 16);
    tmp[0] = 0;
    strcat(tmp, "-isystem");  // separate argv entries
    clang_args[nargs++] = "-isystem";
    clang_args[nargs++] = strdup(glibc_inc);
    fprintf(stderr, "Using glibc include: %s\n", glibc_inc);
  }
  clang_args[nargs++] = "-x";
  clang_args[nargs++] = "c";

  const char *header;

  if (argc > 1) {
    if (argv[1][0] == '<') {
      FILE *f = fopen("/tmp/test_c_import.c", "w");
      fprintf(f, "#include %s\n", argv[1]);
      fclose(f);
      header = "/tmp/test_c_import.c";
    } else {
      header = argv[1];
    }
  } else {
    FILE *f = fopen("/tmp/test_c_import.h", "w");
    fprintf(f, "int printf(const char *fmt, ...);\nint puts(const char *s);\n");
    fclose(f);
    header = "/tmp/test_c_import.h";
  }

  printf("Parsing: %s\n", header);

  CXIndex idx = clang_createIndex(0, 0);
  if (!idx) { fprintf(stderr, "clang_createIndex failed\n"); return 1; }

  CXTranslationUnit tu = clang_parseTranslationUnit(
      idx, header, clang_args, nargs, NULL, 0,
      CXTranslationUnit_SkipFunctionBodies |
      CXTranslationUnit_DetailedPreprocessingRecord);

  if (!tu) {
    fprintf(stderr, "clang_parseTranslationUnit returned NULL\n");
    fprintf(stderr, "  header: %s\n", header);
    fprintf(stderr, "  clang args: ");
    for (int i = 0; i < nargs; i++) fprintf(stderr, "'%s' ", clang_args[i]);
    fprintf(stderr, "\n");
    fprintf(stderr, "  Does the file exist? ");
    FILE *ff = fopen(header, "r");
    fprintf(stderr, "%s\n", ff ? "YES" : "NO");
    if (ff) fclose(ff);
    clang_disposeIndex(idx);
    return 1;
  }

  unsigned diags = clang_getNumDiagnostics(tu);
  printf("Diagnostics: %u\n", diags);
  for (unsigned i = 0; i < diags; i++) {
    CXDiagnostic d = clang_getDiagnostic(tu, i);
    CXString s = clang_formatDiagnostic(d, clang_defaultDiagnosticDisplayOptions());
    printf("  [%u] %s\n", i, clang_getCString(s));
    clang_disposeString(s);
    clang_disposeDiagnostic(d);
  }

  CXCursor root = clang_getTranslationUnitCursor(tu);
  clang_visitChildren(root, visitor, NULL);

  clang_disposeTranslationUnit(tu);
  clang_disposeIndex(idx);
  printf("OK ; libclang smoke test passed.\n");
  return 0;
}
