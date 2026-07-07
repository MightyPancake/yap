/*
 * Longest Common Subsequence benchmark for YAP.
 * Usage: ./lcs <n>
 * Bottom-up DP (rolling 2-row storage, O(n) memory / O(n^2) time) computing
 * the LCS length of two same-length byte buffers. Buffers are filled with a
 * deterministic, small-alphabet (4 symbols) pseudo-random formula via
 * splitmix64 (public domain, Vigna) so real, non-trivial matches occur --
 * a single multiply-mod step (as used in the quicksort benchmark) turned out
 * to have too little avalanche once reduced mod 4 (near-identical buffers,
 * LCS almost n); splitmix64's xor-shift/multiply finalizer avalanches
 * properly even after '% 4'.
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

// Different seed per buffer (not just a shifted index into the same stream)
// so A and B are independent-looking sequences, not one shifted copy of the
// other.
none fn fill(byte@ buf, i32 n, u64 seed) {
    i32 i = 0;
    while (i < n) {
        u64 idx = (i + 1).(u64) + seed;
        u64 h = splitmix64(idx) % 4;
        buf:[i] = (65 + h.(i32)).(byte);
        i = i + 1;
    }
    ret;
}

i32 fn max_i32(i32 a, i32 b) {
    if (a > b) ret a;
    ret b;
}

i32 fn lcs_length(byte@ a, byte@ b, i32 n) {
    none@ prev_raw = stdlib->malloc(((n + 1).(i64)) * 4);
    none@ curr_raw = stdlib->malloc(((n + 1).(i64)) * 4);
    i32@ prev = prev_raw.(i32@);
    i32@ curr = curr_raw.(i32@);

    i32 j0 = 0;
    while (j0 <= n) {
        prev:[j0] = 0;
        j0 = j0 + 1;
    }

    i32 i = 1;
    while (i <= n) {
        curr:[0] = 0;
        i32 j = 1;
        while (j <= n) {
            if (a:[i - 1] == b:[j - 1]) {
                curr:[j] = prev:[j - 1] + 1;
            } else {
                curr:[j] = max_i32(prev:[j], curr:[j - 1]);
            }
            j = j + 1;
        }
        i32@ tmp = prev;
        prev = curr;
        curr = tmp;
        i = i + 1;
    }

    i32 result = prev:[n];
    stdlib->free(prev_raw);
    stdlib->free(curr_raw);
    ret result;
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
        io->print_i32(0);
        io->putchar(10);
        ret 0;
    }

    none@ a_raw = stdlib->malloc(n.(i64));
    none@ b_raw = stdlib->malloc(n.(i64));
    byte@ a = a_raw.(byte@);
    byte@ b = b_raw.(byte@);

    u64 seed_b = 0x9E3779B9;
    fill(a, n, 0);
    fill(b, n, seed_b);

    i32 result = lcs_length(a, b, n);

    stdlib->free(a_raw);
    stdlib->free(b_raw);

    io->print_i32(result);
    io->putchar(10);
    ret 0;
}
