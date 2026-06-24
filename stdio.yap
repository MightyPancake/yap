// C bindings from <stdio.h>

type _IO_marker
type _IO_codecvt
type _IO_wide_data

struct _IO_FILE {
    i32 _flags,
    byte@ _IO_read_ptr,
    byte@ _IO_read_end,
    byte@ _IO_read_base,
    byte@ _IO_write_base,
    byte@ _IO_write_ptr,
    byte@ _IO_write_end,
    byte@ _IO_buf_base,
    byte@ _IO_buf_end,
    byte@ _IO_save_base,
    byte@ _IO_backup_base,
    byte@ _IO_save_end,
    _IO_marker@ _markers,
    _IO_FILE@ _chain,
    i32 _fileno,
    i32 _flags2,
    byte@ _short_backupbuf,
    i64 _old_offset,
    i32 _cur_column,
    byte _vtable_offset,
    byte@ _shortbuf,
    none@ _lock,
    i64 _offset,
    _IO_codecvt@ _codecvt,
    _IO_wide_data@ _wide_data,
    _IO_FILE@ _freeres_list,
    none@ _freeres_buf,
    _IO_FILE@@ _prevchain,
    i32 _mode,
    i32 _unused3,
    i64 _total_written,
    byte@ _unused2,
}

struct _IO_cookie_io_functions_t {
    (i64 fn none@, byte@, i64)@ read,
    (i64 fn none@, byte@, i64)@ write,
    (i32 fn none@, i64@, i32)@ seek,
    (i32 fn none@)@ close,
}

i32 fn remove(byte@ __filename);
i32 fn rename(byte@ __old, byte@ __new);
i32 fn renameat(i32 __oldfd, byte@ __old, i32 __newfd, byte@ __new);
i32 fn fclose(_IO_FILE@ __stream);
_IO_FILE@ fn tmpfile();
byte@ fn tmpnam(byte@ _arg0);
byte@ fn tmpnam_r(byte@ __s);
byte@ fn tempnam(byte@ __dir, byte@ __pfx);
i32 fn fflush(_IO_FILE@ __stream);
i32 fn fflush_unlocked(_IO_FILE@ __stream);
_IO_FILE@ fn fopen(byte@ __filename, byte@ __modes);
_IO_FILE@ fn freopen(byte@ __filename, byte@ __modes, _IO_FILE@ __stream);
_IO_FILE@ fn fdopen(i32 __fd, byte@ __modes);
_IO_FILE@ fn fopencookie(none@ __magic_cookie, byte@ __modes, _IO_cookie_io_functions_t __io_funcs);
_IO_FILE@ fn fmemopen(none@ __s, i64 __len, byte@ __modes);
_IO_FILE@ fn open_memstream(byte@@ __bufloc, i64@ __sizeloc);
none fn setbuf(_IO_FILE@ __stream, byte@ __buf);
i32 fn setvbuf(_IO_FILE@ __stream, byte@ __buf, i32 __modes, i64 __n);
none fn setbuffer(_IO_FILE@ __stream, byte@ __buf, i64 __size);
none fn setlinebuf(_IO_FILE@ __stream);
i32 fn fgetc(_IO_FILE@ __stream);
i32 fn getc(_IO_FILE@ __stream);
i32 fn getchar();
i32 fn getc_unlocked(_IO_FILE@ __stream);
i32 fn getchar_unlocked();
i32 fn fgetc_unlocked(_IO_FILE@ __stream);
i32 fn fputc(i32 __c, _IO_FILE@ __stream);
i32 fn putc(i32 __c, _IO_FILE@ __stream);
i32 fn putchar(i32 __c);
i32 fn fputc_unlocked(i32 __c, _IO_FILE@ __stream);
i32 fn putc_unlocked(i32 __c, _IO_FILE@ __stream);
i32 fn putchar_unlocked(i32 __c);
i32 fn getw(_IO_FILE@ __stream);
i32 fn putw(i32 __w, _IO_FILE@ __stream);
byte@ fn fgets(byte@ __s, i32 __n, _IO_FILE@ __stream);
i64 fn getdelim(byte@@ __lineptr, i64@ __n, i32 __delimiter, _IO_FILE@ __stream);
i64 fn getline(byte@@ __lineptr, i64@ __n, _IO_FILE@ __stream);
i32 fn fputs(byte@ __s, _IO_FILE@ __stream);
i32 fn puts(byte@ __s);
i32 fn ungetc(i32 __c, _IO_FILE@ __stream);
i64 fn fread(none@ __ptr, i64 __size, i64 __n, _IO_FILE@ __stream);
i64 fn fwrite(none@ __ptr, i64 __size, i64 __n, _IO_FILE@ __s);
i64 fn fread_unlocked(none@ __ptr, i64 __size, i64 __n, _IO_FILE@ __stream);
i64 fn fwrite_unlocked(none@ __ptr, i64 __size, i64 __n, _IO_FILE@ __stream);
i32 fn fseek(_IO_FILE@ __stream, i64 __off, i32 __whence);
i64 fn ftell(_IO_FILE@ __stream);
none fn rewind(_IO_FILE@ __stream);
i32 fn fseeko(_IO_FILE@ __stream, i64 __off, i32 __whence);
i64 fn ftello(_IO_FILE@ __stream);
none fn clearerr(_IO_FILE@ __stream);
i32 fn feof(_IO_FILE@ __stream);
i32 fn ferror(_IO_FILE@ __stream);
none fn clearerr_unlocked(_IO_FILE@ __stream);
i32 fn feof_unlocked(_IO_FILE@ __stream);
i32 fn ferror_unlocked(_IO_FILE@ __stream);
none fn perror(byte@ __s);
i32 fn fileno(_IO_FILE@ __stream);
i32 fn fileno_unlocked(_IO_FILE@ __stream);
i32 fn pclose(_IO_FILE@ __stream);
_IO_FILE@ fn popen(byte@ __command, byte@ __modes);
byte@ fn ctermid(byte@ __s);
none fn flockfile(_IO_FILE@ __stream);
i32 fn ftrylockfile(_IO_FILE@ __stream);
none fn funlockfile(_IO_FILE@ __stream);
