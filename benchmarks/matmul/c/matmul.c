#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/*
 * Dense N×N matrix multiplication (f64) benchmark.
 * Fills A and B with deterministic pseudo-values via splitmix64 (to get good
 * f64 coverage without floating-point pattern artifacts), then computes
 * C = A × B using the classic i-k-j triple loop for cache-friendly access.
 *
 * Variant chosen: C[i][j] = sum_k A[i][k] * B[k][j] (i-k-j loop order)
 * This is the standard row-major-friendly order: A accessed row-wise (good),
 * B accessed column-wise (bad), C accessed row-wise (good). This order is
 * kept identical across all 4 languages.
 *
 * Checksum: sum of all elements of C (as f64), printed with 6 decimal places.
 * The checksum is computationally heavy to verify externally but provides a
 * precise correctness check — bit-identical across languages.
 */

/* splitmix64 (public domain, Vigna) — good avalanche even into floating-point
 * mantissa bits, avoids the short-period problems of a simple LCG when used
 * for high-dimension matrix fill. */
static uint64_t splitmix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    x = x ^ (x >> 31);
    return x;
}

/* Fill matrix mat (row-major, n×n) with f64 values in [0, 1000002].
 * Each element = splitmix64(i*n + j + 1 + seed) % 1000003, cast to f64.
 * Integer-range values keep relative error minimal when summing millions
 * of products; splitmix64 ensures good pseudo-random distribution. */
static void fill(double *mat, int32_t n, uint64_t seed) {
    for (int32_t i = 0; i < n; i++) {
        for (int32_t j = 0; j < n; j++) {
            uint64_t idx = (uint64_t)(i * n + j + 1) + seed;
            mat[i * n + j] = (double)(splitmix64(idx) % 1000003ULL);
        }
    }
}

/* Dense matrix multiply: C = A * B, all n×n, row-major.
 * Loop order i-k-j: for each row of C, accumulate across k. */
static void matmul(const double *a, const double *b, double *c, int32_t n) {
    for (int32_t i = 0; i < n; i++) {
        for (int32_t j = 0; j < n; j++) {
            c[i * n + j] = 0.0;
        }
        for (int32_t k = 0; k < n; k++) {
            double aik = a[i * n + k];
            for (int32_t j = 0; j < n; j++) {
                c[i * n + j] += aik * b[k * n + j];
            }
        }
    }
}

/* Sum all elements of an n×n matrix. */
static double checksum(const double *mat, int32_t n) {
    double sum = 0.0;
    int32_t total = n * n;
    for (int32_t i = 0; i < total; i++) {
        sum += mat[i];
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
        printf("0.000000\n");
        return 0;
    }

    size_t mat_bytes = (size_t)n * (size_t)n * sizeof(double);
    double *a = malloc(mat_bytes);
    double *b = malloc(mat_bytes);
    double *c = malloc(mat_bytes);
    if (!a || !b || !c) {
        fprintf(stderr, "malloc failed\n");
        free(a); free(b); free(c);
        return 1;
    }

    fill(a, n, 0);
    fill(b, n, 0x9E3779B9ULL);

    matmul(a, b, c, n);

    double result = checksum(c, n);
    free(a);
    free(b);
    free(c);

    printf("%.6f\n", result);
    return 0;
}
