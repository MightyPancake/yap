/*
 * N-Queens benchmark for YAP.
 * Usage: ./n_queens <n>
 * Classic bitmask backtracking: cols/diag1/diag2 track occupied columns and
 * both diagonals as bits of a u64, avoiding arrays/pointers entirely so the
 * whole thing is scalar recursion, matching the other 3 languages exactly.
 */
import io

u64 fn solve(u64 cols, u64 diag1, u64 diag2, u64 full) {
    if (cols == full) {
        u64 one = 1;
        ret one;
    }
    u64 avail = full & ~(cols | diag1 | diag2);
    u64 total = 0;
    while (avail != 0) {
        u64 bit = avail & (~avail + 1);
        avail = avail ^ bit;
        total = total + solve(cols | bit, (diag1 | bit) << 1, (diag2 | bit) >> 1, full);
    }
    ret total;
}

i32 fn main(byte@[] args) {
    byte@ n_str = args:[1];
    u64 n = 0;
    i32 i = 0;
    while (n_str:[i] != 0) {
        n = n * 10 + n_str:[i].(u64) - 48;
        i = i + 1;
    }

    u64 one = 1;
    u64 full = 0;
    if (n >= 64) {
        full = ~full;
    } else {
        full = (one << n) - one;
    }

    u64 result = solve(0, 0, 0, full);
    io->print_i32(result.(i32));
    io->putchar('\n');
    ret 0;
}
