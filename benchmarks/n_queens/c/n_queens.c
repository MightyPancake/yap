#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/*
 * Classic bitmask backtracking N-Queens solver: cols/diag1/diag2 track
 * occupied columns and both diagonals as bits; "full" has the low n bits
 * set. Chosen (over an array-based safety check) so every language in this
 * benchmark can implement the exact same algorithm on scalars alone.
 */
static uint64_t solve(uint64_t cols, uint64_t diag1, uint64_t diag2, uint64_t full) {
    if (cols == full) return 1;
    uint64_t avail = full & ~(cols | diag1 | diag2);
    uint64_t total = 0;
    while (avail != 0) {
        uint64_t bit = avail & (~avail + 1);
        avail = avail ^ bit;
        total = total + solve(cols | bit, (diag1 | bit) << 1, (diag2 | bit) >> 1, full);
    }
    return total;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }
    uint64_t n = strtoull(argv[1], NULL, 10);
    uint64_t full = (n >= 64) ? ~(uint64_t)0 : ((((uint64_t)1) << n) - 1);
    uint64_t result = solve(0, 0, 0, full);
    printf("%" PRIu64 "\n", result);
    return 0;
}
