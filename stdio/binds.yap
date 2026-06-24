// C bindings from <stdio.h>

struct __va_list_tag {
    gp_offset: i32,
    fp_offset: i32,
    overflow_arg_area: none@,
    reg_save_area: none@,
}

struct __fsid_t {
    __val: i32@,
}

struct __mbstate_t {
    __count: i32,
    __value: union anon,
}

union anon {
    __wch: i32,
    __wchb: byte@,
}

struct _G_fpos_t {
    __pos: i64,
    __state: struct __mbstate_t,
}

struct _G_fpos64_t {
    __pos: i64,
    __state: struct __mbstate_t,
}

struct _IO_FILE {
    _flags: i32,
    _IO_read_ptr: byte@,
    _IO_read_end: byte@,
    _IO_read_base: byte@,
    _IO_write_base: byte@,
    _IO_write_ptr: byte@,
    _IO_write_end: byte@,
    _IO_buf_base: byte@,
    _IO_buf_end: byte@,
    _IO_save_base: byte@,
    _IO_backup_base: byte@,
    _IO_save_end: byte@,
    _markers: struct _IO_marker@,
    _chain: struct _IO_FILE@,
    _fileno: i32,
    _flags2: i32,
    _short_backupbuf: byte@,
    _old_offset: i64,
    _cur_column: i32,
    _vtable_offset: byte,
    _shortbuf: byte@,
    _lock: none@,
    _offset: i64,
    _codecvt: struct _IO_codecvt@,
    _wide_data: struct _IO_wide_data@,
    _freeres_list: struct _IO_FILE@,
    _freeres_buf: none@,
    _prevchain: struct _IO_FILE@@,
    _mode: i32,
    _unused3: i32,
    _total_written: i64,
    _unused2: byte@,
}

struct _IO_marker {
}

struct _IO_codecvt {
}

struct _IO_wide_data {
}

struct _IO_cookie_io_functions_t {
    read: (i64 fn none@,  byte@,  i64)@,
    write: (i64 fn none@,  byte@,  i64)@,
    seek: (i32 fn none@,  i64@,  i32)@,
    close: (i32 fn none@)@,
}

i32 fn remove(__filename: byte@) i32;
i32 fn rename(__old: byte@, __new: byte@) i32;
i32 fn renameat(__oldfd: i32, __old: byte@, __newfd: i32, __new: byte@) i32;
i32 fn fclose(__stream: struct _IO_FILE@) i32;
struct _IO_FILE@ fn tmpfile() struct _IO_FILE@;
byte@ fn tmpnam(byte@) byte@;
byte@ fn tmpnam_r(__s: byte@) byte@;
byte@ fn tempnam(__dir: byte@, __pfx: byte@) byte@;
i32 fn fflush(__stream: struct _IO_FILE@) i32;
i32 fn fflush_unlocked(__stream: struct _IO_FILE@) i32;
struct _IO_FILE@ fn fopen(__filename: byte@, __modes: byte@) struct _IO_FILE@;
struct _IO_FILE@ fn freopen(__filename: byte@, __modes: byte@, __stream: struct _IO_FILE@) struct _IO_FILE@;
struct _IO_FILE@ fn fdopen(__fd: i32, __modes: byte@) struct _IO_FILE@;
struct _IO_FILE@ fn fopencookie(__magic_cookie: none@, __modes: byte@, __io_funcs: struct _IO_cookie_io_functions_t) struct _IO_FILE@;
struct _IO_FILE@ fn fmemopen(__s: none@, __len: i64, __modes: byte@) struct _IO_FILE@;
struct _IO_FILE@ fn open_memstream(__bufloc: byte@@, __sizeloc: i64@) struct _IO_FILE@;
none fn setbuf(__stream: struct _IO_FILE@, __buf: byte@) none;
i32 fn setvbuf(__stream: struct _IO_FILE@, __buf: byte@, __modes: i32, __n: i64) i32;
none fn setbuffer(__stream: struct _IO_FILE@, __buf: byte@, __size: i64) none;
none fn setlinebuf(__stream: struct _IO_FILE@) none;
i32 fn fprintf(__stream: struct _IO_FILE@, __format: byte@) i32;
i32 fn printf(__format: byte@) i32;
i32 fn sprintf(__s: byte@, __format: byte@) i32;
i32 fn vfprintf(__s: struct _IO_FILE@, __format: byte@, __arg: struct __va_list_tag@) i32;
i32 fn vprintf(__format: byte@, __arg: struct __va_list_tag@) i32;
i32 fn vsprintf(__s: byte@, __format: byte@, __arg: struct __va_list_tag@) i32;
i32 fn snprintf(__s: byte@, __maxlen: i64, __format: byte@) i32;
i32 fn vsnprintf(__s: byte@, __maxlen: i64, __format: byte@, __arg: struct __va_list_tag@) i32;
i32 fn vasprintf(__ptr: byte@@, __f: byte@, __arg: struct __va_list_tag@) i32;
i32 fn __asprintf(__ptr: byte@@, __fmt: byte@) i32;
i32 fn asprintf(__ptr: byte@@, __fmt: byte@) i32;
i32 fn vdprintf(__fd: i32, __fmt: byte@, __arg: struct __va_list_tag@) i32;
i32 fn dprintf(__fd: i32, __fmt: byte@) i32;
i32 fn fscanf(__stream: struct _IO_FILE@, __format: byte@) i32;
i32 fn scanf(__format: byte@) i32;
i32 fn sscanf(__s: byte@, __format: byte@) i32;
i32 fn fscanf(__stream: struct _IO_FILE@, __format: byte@) i32;
i32 fn scanf(__format: byte@) i32;
i32 fn sscanf(__s: byte@, __format: byte@) i32;
i32 fn vfscanf(__s: struct _IO_FILE@, __format: byte@, __arg: struct __va_list_tag@) i32;
i32 fn vscanf(__format: byte@, __arg: struct __va_list_tag@) i32;
i32 fn vsscanf(__s: byte@, __format: byte@, __arg: struct __va_list_tag@) i32;
i32 fn vfscanf(__s: struct _IO_FILE@, __format: byte@, __arg: struct __va_list_tag@) i32;
i32 fn vscanf(__format: byte@, __arg: struct __va_list_tag@) i32;
i32 fn vsscanf(__s: byte@, __format: byte@, __arg: struct __va_list_tag@) i32;
i32 fn fgetc(__stream: struct _IO_FILE@) i32;
i32 fn getc(__stream: struct _IO_FILE@) i32;
i32 fn getchar() i32;
i32 fn getc_unlocked(__stream: struct _IO_FILE@) i32;
i32 fn getchar_unlocked() i32;
i32 fn fgetc_unlocked(__stream: struct _IO_FILE@) i32;
i32 fn fputc(__c: i32, __stream: struct _IO_FILE@) i32;
i32 fn putc(__c: i32, __stream: struct _IO_FILE@) i32;
i32 fn putchar(__c: i32) i32;
i32 fn fputc_unlocked(__c: i32, __stream: struct _IO_FILE@) i32;
i32 fn putc_unlocked(__c: i32, __stream: struct _IO_FILE@) i32;
i32 fn putchar_unlocked(__c: i32) i32;
i32 fn getw(__stream: struct _IO_FILE@) i32;
i32 fn putw(__w: i32, __stream: struct _IO_FILE@) i32;
byte@ fn fgets(__s: byte@, __n: i32, __stream: struct _IO_FILE@) byte@;
i64 fn __getdelim(__lineptr: byte@@, __n: i64@, __delimiter: i32, __stream: struct _IO_FILE@) i64;
i64 fn getdelim(__lineptr: byte@@, __n: i64@, __delimiter: i32, __stream: struct _IO_FILE@) i64;
i64 fn getline(__lineptr: byte@@, __n: i64@, __stream: struct _IO_FILE@) i64;
i32 fn fputs(__s: byte@, __stream: struct _IO_FILE@) i32;
i32 fn puts(__s: byte@) i32;
i32 fn ungetc(__c: i32, __stream: struct _IO_FILE@) i32;
i64 fn fread(__ptr: none@, __size: i64, __n: i64, __stream: struct _IO_FILE@) i64;
i64 fn fwrite(__ptr: none@, __size: i64, __n: i64, __s: struct _IO_FILE@) i64;
i64 fn fread_unlocked(__ptr: none@, __size: i64, __n: i64, __stream: struct _IO_FILE@) i64;
i64 fn fwrite_unlocked(__ptr: none@, __size: i64, __n: i64, __stream: struct _IO_FILE@) i64;
i32 fn fseek(__stream: struct _IO_FILE@, __off: i64, __whence: i32) i32;
i64 fn ftell(__stream: struct _IO_FILE@) i64;
none fn rewind(__stream: struct _IO_FILE@) none;
i32 fn fseeko(__stream: struct _IO_FILE@, __off: i64, __whence: i32) i32;
i64 fn ftello(__stream: struct _IO_FILE@) i64;
i32 fn fgetpos(__stream: struct _IO_FILE@, __pos: struct _G_fpos_t@) i32;
i32 fn fsetpos(__stream: struct _IO_FILE@, __pos: struct _G_fpos_t@) i32;
none fn clearerr(__stream: struct _IO_FILE@) none;
i32 fn feof(__stream: struct _IO_FILE@) i32;
i32 fn ferror(__stream: struct _IO_FILE@) i32;
none fn clearerr_unlocked(__stream: struct _IO_FILE@) none;
i32 fn feof_unlocked(__stream: struct _IO_FILE@) i32;
i32 fn ferror_unlocked(__stream: struct _IO_FILE@) i32;
none fn perror(__s: byte@) none;
i32 fn fileno(__stream: struct _IO_FILE@) i32;
i32 fn fileno_unlocked(__stream: struct _IO_FILE@) i32;
i32 fn pclose(__stream: struct _IO_FILE@) i32;
struct _IO_FILE@ fn popen(__command: byte@, __modes: byte@) struct _IO_FILE@;
byte@ fn ctermid(__s: byte@) byte@;
none fn flockfile(__stream: struct _IO_FILE@) none;
i32 fn ftrylockfile(__stream: struct _IO_FILE@) i32;
none fn funlockfile(__stream: struct _IO_FILE@) none;
i32 fn __uflow(struct _IO_FILE@) i32;
i32 fn __overflow(struct _IO_FILE@, i32) i32;
