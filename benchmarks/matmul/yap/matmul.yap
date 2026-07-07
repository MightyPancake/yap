/*
 * Dense N×N matrix multiplication (f64) benchmark for YAP.
 * Usage: ./matmul <n>
 *
 * Fills A and B with deterministic f64 values via splitmix64 % 1000003,
 * computes C = A × B (i-k-j loop order), then prints the sum of all
 * elements of C. Heap allocation matches the C implementation.
 */
import io
import stdlib

u64 fn splitmix64(u64 x) {
    x = x + 0x9E3779B97F4A7C15;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EB;
    x = x ^ (x >> 31);
    ret x;
}

none fn print_u64(u64 v) {
    if (v == 0) {
        io->print_char(48);
        ret;
    }
    i32[24] digits;
    i32 n = 0;
    while (v > 0) {
        u64 d = v % 10;
        digits:[n] = d.(i32) + 48;
        v = v / 10;
        n = n + 1;
    }
    i32 i = n - 1;
    while (i >= 0) {
        io->print_char(digits:[i]);
        i = i - 1;
    }
}

/* Print f64 with 6 decimal places. Simple approach: print integer part,
 * then ".", then 6 fractional digits. */
none fn print_f64(f64 val) {
    f64 zero = 0;
    if (val < zero) {
        io->print_char(45);
        val = zero - val;
    }
    i64 int_part = val.(i64);
    print_u64(int_part.(u64));
    io->print_char(46);
    f64 frac = val - int_part.(f64);
    f64 ten = 10;
    i32 d = 0;
    while (d < 6) {
        frac = frac * ten;
        i64 digit = frac.(i64);
        io->print_char(digit.(i32) + 48);
        frac = frac - digit.(f64);
        d = d + 1;
    }
}

/* Fill matrix mat (row-major, n×n) with f64 values in [0, 1000002]. */
none fn fill(f64@ mat, i32 n, u64 seed) {
    i32 i = 0;
    while (i < n) {
        i32 j = 0;
        while (j < n) {
            u64 idx = (i * n + j + 1).(u64) + seed;
            mat:[i * n + j] = (splitmix64(idx) % 1000003).(f64);
            j = j + 1;
        }
        i = i + 1;
    }
    ret;
}

/* Dense matrix multiply: C = A * B, i-k-j loop order. */
none fn matmul(f64@ a, f64@ b, f64@ c, i32 n) {
    i32 i = 0;
    while (i < n) {
        i32 j = 0;
        while (j < n) {
            c:[i * n + j] = 0.0;
            j = j + 1;
        }
        i32 k = 0;
        while (k < n) {
            f64 aik = a:[i * n + k];
            i32 j = 0;
            while (j < n) {
                c:[i * n + j] = c:[i * n + j] + aik * b:[k * n + j];
                j = j + 1;
            }
            k = k + 1;
        }
        i = i + 1;
    }
    ret;
}

/* Sum all elements of an n×n matrix. */
f64 fn checksum(f64@ mat, i32 n) {
    f64 sum = 0;
    i32 total = n * n;
    i32 i = 0;
    while (i < total) {
        sum = sum + mat:[i];
        i = i + 1;
    }
    ret sum;
}

i32 fn main(byte@[] args) {
    byte@ n_str = args:[1];
    i32 n = 0;
    i32 i = 0;
    while (n_str:[i] != 0) {
        n = n * 10 + n_str:[i].(i32) - 48;
        i = i + 1;
    }

    if (n <= 0) {
        print_f64(0);
        io->putchar(10);
        ret 0;
    }

    i64 mat_bytes = (n * n).(i64) * 8;
    none@ raw_a = stdlib->malloc(mat_bytes);
    none@ raw_b = stdlib->malloc(mat_bytes);
    none@ raw_c = stdlib->malloc(mat_bytes);
    f64@ a = raw_a.(f64@);
    f64@ b = raw_b.(f64@);
    f64@ c = raw_c.(f64@);

    fill(a, n, 0);
    fill(b, n, 0x9E3779B9);

    matmul(a, b, c, n);

    f64 result = checksum(c, n);
    stdlib->free(raw_a);
    stdlib->free(raw_b);
    stdlib->free(raw_c);

    print_f64(result);
    io->putchar(10);
    ret 0;
}
