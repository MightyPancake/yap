#include <stdio.h>

__attribute__((visibility("default")))
int io_remove(const char * __filename) {
    return remove(__filename);
}

__attribute__((visibility("default")))
int io_rename(const char * __old, const char * __new) {
    return rename(__old, __new);
}

__attribute__((visibility("default")))
int io_renameat(int __oldfd, const char * __old, int __newfd, const char * __new) {
    return renameat(__oldfd, __old, __newfd, __new);
}

__attribute__((visibility("default")))
int io_fclose(FILE * __stream) {
    return fclose(__stream);
}

__attribute__((visibility("default")))
FILE * io_tmpfile(void) {
    return tmpfile();
}

__attribute__((visibility("default")))
char * io_tmpnam(char * _arg0) {
    return tmpnam(_arg0);
}

__attribute__((visibility("default")))
char * io_tmpnam_r(char * __s) {
    return tmpnam_r(__s);
}

__attribute__((visibility("default")))
char * io_tempnam(const char * __dir, const char * __pfx) {
    return tempnam(__dir, __pfx);
}

__attribute__((visibility("default")))
int io_fflush(FILE * __stream) {
    return fflush(__stream);
}

__attribute__((visibility("default")))
int io_fflush_unlocked(FILE * __stream) {
    return fflush_unlocked(__stream);
}

__attribute__((visibility("default")))
FILE * io_fopen(const char *restrict __filename, const char *restrict __modes) {
    return fopen(__filename, __modes);
}

__attribute__((visibility("default")))
FILE * io_freopen(const char *restrict __filename, const char *restrict __modes, FILE *restrict __stream) {
    return freopen(__filename, __modes, __stream);
}

__attribute__((visibility("default")))
FILE * io_fdopen(int __fd, const char * __modes) {
    return fdopen(__fd, __modes);
}

__attribute__((visibility("default")))
FILE * io_fopencookie(void *restrict __magic_cookie, const char *restrict __modes, cookie_io_functions_t __io_funcs) {
    return fopencookie(__magic_cookie, __modes, __io_funcs);
}

__attribute__((visibility("default")))
FILE * io_fmemopen(void * __s, size_t __len, const char * __modes) {
    return fmemopen(__s, __len, __modes);
}

__attribute__((visibility("default")))
FILE * io_open_memstream(char ** __bufloc, size_t * __sizeloc) {
    return open_memstream(__bufloc, __sizeloc);
}

__attribute__((visibility("default")))
void io_setbuf(FILE *restrict __stream, char *restrict __buf) {
    setbuf(__stream, __buf);
}

__attribute__((visibility("default")))
int io_setvbuf(FILE *restrict __stream, char *restrict __buf, int __modes, size_t __n) {
    return setvbuf(__stream, __buf, __modes, __n);
}

__attribute__((visibility("default")))
void io_setbuffer(FILE *restrict __stream, char *restrict __buf, size_t __size) {
    setbuffer(__stream, __buf, __size);
}

__attribute__((visibility("default")))
void io_setlinebuf(FILE * __stream) {
    setlinebuf(__stream);
}

__attribute__((visibility("default")))
int io_fgetc(FILE * __stream) {
    return fgetc(__stream);
}

__attribute__((visibility("default")))
int io_getc(FILE * __stream) {
    return getc(__stream);
}

__attribute__((visibility("default")))
int io_getchar(void) {
    return getchar();
}

__attribute__((visibility("default")))
int io_getc_unlocked(FILE * __stream) {
    return getc_unlocked(__stream);
}

__attribute__((visibility("default")))
int io_getchar_unlocked(void) {
    return getchar_unlocked();
}

__attribute__((visibility("default")))
int io_fgetc_unlocked(FILE * __stream) {
    return fgetc_unlocked(__stream);
}

__attribute__((visibility("default")))
int io_fputc(int __c, FILE * __stream) {
    return fputc(__c, __stream);
}

__attribute__((visibility("default")))
int io_putc(int __c, FILE * __stream) {
    return putc(__c, __stream);
}

__attribute__((visibility("default")))
int io_putchar(int __c) {
    return putchar(__c);
}

__attribute__((visibility("default")))
int io_fputc_unlocked(int __c, FILE * __stream) {
    return fputc_unlocked(__c, __stream);
}

__attribute__((visibility("default")))
int io_putc_unlocked(int __c, FILE * __stream) {
    return putc_unlocked(__c, __stream);
}

__attribute__((visibility("default")))
int io_putchar_unlocked(int __c) {
    return putchar_unlocked(__c);
}

__attribute__((visibility("default")))
int io_getw(FILE * __stream) {
    return getw(__stream);
}

__attribute__((visibility("default")))
int io_putw(int __w, FILE * __stream) {
    return putw(__w, __stream);
}

__attribute__((visibility("default")))
char * io_fgets(char *restrict __s, int __n, FILE *restrict __stream) {
    return fgets(__s, __n, __stream);
}

__attribute__((visibility("default")))
__ssize_t io_getdelim(char **restrict __lineptr, size_t *restrict __n, int __delimiter, FILE *restrict __stream) {
    return getdelim(__lineptr, __n, __delimiter, __stream);
}

__attribute__((visibility("default")))
__ssize_t io_getline(char **restrict __lineptr, size_t *restrict __n, FILE *restrict __stream) {
    return getline(__lineptr, __n, __stream);
}

__attribute__((visibility("default")))
int io_fputs(const char *restrict __s, FILE *restrict __stream) {
    return fputs(__s, __stream);
}

__attribute__((visibility("default")))
int io_puts(const char * __s) {
    return puts(__s);
}

__attribute__((visibility("default")))
int io_ungetc(int __c, FILE * __stream) {
    return ungetc(__c, __stream);
}

__attribute__((visibility("default")))
unsigned long io_fread(void *restrict __ptr, size_t __size, size_t __n, FILE *restrict __stream) {
    return fread(__ptr, __size, __n, __stream);
}

__attribute__((visibility("default")))
unsigned long io_fwrite(const void *restrict __ptr, size_t __size, size_t __n, FILE *restrict __s) {
    return fwrite(__ptr, __size, __n, __s);
}

__attribute__((visibility("default")))
size_t io_fread_unlocked(void *restrict __ptr, size_t __size, size_t __n, FILE *restrict __stream) {
    return fread_unlocked(__ptr, __size, __n, __stream);
}

__attribute__((visibility("default")))
size_t io_fwrite_unlocked(const void *restrict __ptr, size_t __size, size_t __n, FILE *restrict __stream) {
    return fwrite_unlocked(__ptr, __size, __n, __stream);
}

__attribute__((visibility("default")))
int io_fseek(FILE * __stream, long __off, int __whence) {
    return fseek(__stream, __off, __whence);
}

__attribute__((visibility("default")))
long io_ftell(FILE * __stream) {
    return ftell(__stream);
}

__attribute__((visibility("default")))
void io_rewind(FILE * __stream) {
    rewind(__stream);
}

__attribute__((visibility("default")))
int io_fseeko(FILE * __stream, __off_t __off, int __whence) {
    return fseeko(__stream, __off, __whence);
}

__attribute__((visibility("default")))
__off_t io_ftello(FILE * __stream) {
    return ftello(__stream);
}

__attribute__((visibility("default")))
void io_clearerr(FILE * __stream) {
    clearerr(__stream);
}

__attribute__((visibility("default")))
int io_feof(FILE * __stream) {
    return feof(__stream);
}

__attribute__((visibility("default")))
int io_ferror(FILE * __stream) {
    return ferror(__stream);
}

__attribute__((visibility("default")))
void io_clearerr_unlocked(FILE * __stream) {
    clearerr_unlocked(__stream);
}

__attribute__((visibility("default")))
int io_feof_unlocked(FILE * __stream) {
    return feof_unlocked(__stream);
}

__attribute__((visibility("default")))
int io_ferror_unlocked(FILE * __stream) {
    return ferror_unlocked(__stream);
}

__attribute__((visibility("default")))
void io_perror(const char * __s) {
    perror(__s);
}

__attribute__((visibility("default")))
int io_fileno(FILE * __stream) {
    return fileno(__stream);
}

__attribute__((visibility("default")))
int io_fileno_unlocked(FILE * __stream) {
    return fileno_unlocked(__stream);
}

__attribute__((visibility("default")))
int io_pclose(FILE * __stream) {
    return pclose(__stream);
}

__attribute__((visibility("default")))
FILE * io_popen(const char * __command, const char * __modes) {
    return popen(__command, __modes);
}

__attribute__((visibility("default")))
char * io_ctermid(char * __s) {
    return ctermid(__s);
}

__attribute__((visibility("default")))
void io_flockfile(FILE * __stream) {
    flockfile(__stream);
}

__attribute__((visibility("default")))
int io_ftrylockfile(FILE * __stream) {
    return ftrylockfile(__stream);
}

__attribute__((visibility("default")))
void io_funlockfile(FILE * __stream) {
    funlockfile(__stream);
}

