/*
 * Quicksort benchmark for YAP.
 * Usage: ./quicksort <n>
 * In-place Lomuto-partition quicksort over a heap-allocated i32 array
 * (stdlib->malloc, i32@ pointer indexed via ':['). Heap allocation (not a
 * stack array) is deliberate and applies uniformly across all 4 languages in
 * this benchmark: n can reach into the millions, and a stack array that size
 * would overflow the default 8MB thread stack in any of C/Nim/Zig/yap alike.
 */
import io
import stdlib

// io module has no u64 print helper (only print_i32, which would truncate
// our checksum -- checksums here exceed i32 range even for small n), so this
// benchmark provides its own, same digit-buffer approach as io->print_i32.
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

none fn swap_elem(i32@ arr, i32 i, i32 j) {
    i32 tmp = arr:[i];
    arr:[i] = arr:[j];
    arr:[j] = tmp;
    ret;
}

i32 fn partition(i32@ arr, i32 lo, i32 hi) {
    i32 pivot = arr:[hi];
    i32 i = lo - 1;
    i32 j = lo;
    while (j < hi) {
        if (arr:[j] <= pivot) {
            i = i + 1;
            swap_elem(arr, i, j);
        }
        j = j + 1;
    }
    swap_elem(arr, i + 1, hi);
    ret i + 1;
}

none fn quicksort(i32@ arr, i32 lo, i32 hi) {
    if (lo < hi) {
        i32 p = partition(arr, lo, hi);
        quicksort(arr, lo, p - 1);
        quicksort(arr, p + 1, hi);
    }
    ret;
}

none fn fill(i32@ arr, i32 n) {
    i32 i = 0;
    while (i < n) {
        u64 idx = (i + 1).(u64);
        arr:[i] = ((idx * 2654435761) % 1000003).(i32);
        i = i + 1;
    }
    ret;
}

u64 fn checksum(i32@ arr, i32 n) {
    u64 sum = 0;
    i32 i = 0;
    while (i < n) {
        sum = sum + arr:[i].(u64) * (i + 1).(u64);
        i = i + 1;
    }
    ret sum;
}

i32 fn main(byte@[] args) {
    byte@ n_str = args:[1];
    i32 n = 0;
    i32 k = 0;
    while (n_str:[k] != 0) {
        n = n * 10 + n_str:[k].(i32) - 48;
        k = k + 1;
    }

    if (n <= 0) {
        print_u64(0);
        io->putchar(10);
        ret 0;
    }

    none@ raw = stdlib->malloc((n.(i64)) * 4);
    i32@ arr = raw.(i32@);

    fill(arr, n);
    quicksort(arr, 0, n - 1);
    u64 result = checksum(arr, n);
    stdlib->free(raw);

    print_u64(result);
    io->putchar(10);
    ret 0;
}
