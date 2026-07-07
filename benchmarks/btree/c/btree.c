#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/*
 * Binary tree build → traverse → free.
 * Heap-allocation/pointer-chasing heavy; benchmark for manual malloc/free
 * (yap/C/Zig) vs GC/ARC (Nim).
 * Builds a perfectly balanced complete tree of n nodes, post-order frees it,
 * and computes a checksum via in-order traversal.
 */
typedef struct Node {
    int32_t value;
    struct Node *left;
    struct Node *right;
} Node;

static Node *build(int32_t lo, int32_t hi) {
    if (lo > hi) return NULL;
    int32_t mid = lo + (hi - lo) / 2;
    Node *n = (Node *)malloc(sizeof(Node));
    n->value = mid;
    n->left  = build(lo, mid - 1);
    n->right = build(mid + 1, hi);
    return n;
}

static uint64_t checksum(Node *root) {
    if (!root) return 0;
    uint64_t sum = checksum(root->left);
    /* Deterministic mixing: node value * position-based weight folded into sum */
    sum += (uint64_t)(root->value + 1) * 2654435761ULL;
    sum += checksum(root->right);
    return sum;
}

static void free_tree(Node *root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }
    int32_t n = (int32_t)strtol(argv[1], NULL, 10);
    if (n <= 0) {
        printf("0\n");
        return 0;
    }
    Node *root = build(1, n);
    uint64_t ck = checksum(root);
    free_tree(root);
    printf("%" PRIu64 "\n", ck);
    return 0;
}
