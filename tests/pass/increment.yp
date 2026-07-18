import io

i32 fn assert_eq(i32 a, i32 b, i32 test_num) {
    if (a != b) {
        io->putchar(70); // 'F'
        io->putchar(test_num + 48);
        io->putchar(10);
        ret 1;
    }
    ret 0;
}

i32 fn main() {
    i32 fail = 0;

    // --- Postfix (expr++, expr--): yields the OLD value, then mutates ---
    i32 a = 5;
    fail = fail + assert_eq(a++, 5, 0);
    fail = fail + assert_eq(a, 6, 1);
    fail = fail + assert_eq(a--, 6, 2);
    fail = fail + assert_eq(a, 5, 3);

    // --- Prefix (++expr, --expr): mutates, then yields the NEW value ---
    i32 b = 5;
    fail = fail + assert_eq(++b, 6, 4);
    fail = fail + assert_eq(b, 6, 5);
    fail = fail + assert_eq(--b, 5, 6);
    fail = fail + assert_eq(b, 5, 7);

    // Prefix binds tighter than '+': (++c) + 10 == 16, not ++(c + 10)
    i32 c = 5;
    fail = fail + assert_eq(++c + 10, 16, 8);
    fail = fail + assert_eq(c, 6, 9);

    // Prefix works as a plain statement and as a for-loop step expression
    i32 sum = 0;
    for (i32 i = 0; i < 3; ++i) {
        sum = sum + 1;
    }
    fail = fail + assert_eq(sum, 3, 10);

    ret fail;
}
