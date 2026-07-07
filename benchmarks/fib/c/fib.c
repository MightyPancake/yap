#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

static uint64_t fib(uint64_t n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }
    uint64_t n = strtoull(argv[1], NULL, 10);
    uint64_t result = fib(n);
    printf("%" PRIu64 "\n", result);
    return 0;
}
