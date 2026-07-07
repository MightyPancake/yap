/*
 * Fibonacci benchmark for YAP.
 * Usage: ./fib <n>
 * Uses YAP's native argc/argv support via `byte@[] args` in main.
 */
import io

u64 fn fib(u64 n) {
    if (n <= 1) ret n;
    ret fib(n - 1) + fib(n - 2);
}

i32 fn main(byte@[] args) {
    // args[0] is the program name, args[1] is the first argument
    // Parse args[1] as a u64 manually (no stdlib parse needed)
    byte@ n_str = args:[1];
    u64 n = 0;
    i32 i = 0;
    while (n_str:[i] != 0) {
        n = n * 10 + n_str:[i].(u64) - 48;
        i = i + 1;
    }

    u64 result = fib(n);
    io->print_i32(result.(i32));
    io->putchar('\n');
    ret 0;
}

