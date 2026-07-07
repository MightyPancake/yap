#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/*
 * In-place Lomuto-partition quicksort over a heap-allocated i32 array.
 * Heap allocation (not a stack array) is deliberate and applies uniformly
 * across all 4 languages in this benchmark: n can reach into the millions,
 * and a stack array that size would overflow the default 8MB thread stack
 * in any of C/Nim/Zig/yap alike, not just here.
 */
static void swap_elem(int32_t *arr, int32_t i, int32_t j) {
    int32_t tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;
}

static int32_t partition(int32_t *arr, int32_t lo, int32_t hi) {
    int32_t pivot = arr[hi];
    int32_t i = lo - 1;
    for (int32_t j = lo; j < hi; j++) {
        if (arr[j] <= pivot) {
            i++;
            swap_elem(arr, i, j);
        }
    }
    swap_elem(arr, i + 1, hi);
    return i + 1;
}

static void quicksort(int32_t *arr, int32_t lo, int32_t hi) {
    if (lo < hi) {
        int32_t p = partition(arr, lo, hi);
        quicksort(arr, lo, p - 1);
        quicksort(arr, p + 1, hi);
    }
}

/* Deterministic pseudo-shuffled fill, identical formula across all 4 languages. */
static void fill(int32_t *arr, int32_t n) {
    for (int32_t i = 0; i < n; i++) {
        arr[i] = (int32_t)(((uint64_t)(i + 1) * 2654435761ULL) % 1000003ULL);
    }
}

static uint64_t checksum(const int32_t *arr, int32_t n) {
    uint64_t sum = 0;
    for (int32_t i = 0; i < n; i++) {
        sum += (uint64_t)arr[i] * (uint64_t)(i + 1);
    }
    return sum;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }
    int32_t n = (int32_t)strtol(argv[1], NULL, 10);

    if (n <= 0) {
        printf("%" PRIu64 "\n", (uint64_t)0);
        return 0;
    }

    int32_t *arr = malloc((size_t)n * sizeof(int32_t));
    if (!arr) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    fill(arr, n);
    quicksort(arr, 0, n - 1);
    uint64_t result = checksum(arr, n);
    free(arr);

    printf("%" PRIu64 "\n", result);
    return 0;
}
