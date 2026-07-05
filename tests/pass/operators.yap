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
    i32 a = 6;
    i32 b = 3;

    // --- Logical (&&, ||) ---
    if (true && true) fail = fail + 0; else fail = fail + 1;
    if (true && false) fail = fail + 1; else fail = fail + 0;
    if (false || true) fail = fail + 0; else fail = fail + 1;
    if (false || false) fail = fail + 1; else fail = fail + 0;
    // && binds tighter than ||: true || (false && false) == true,
    // not (true || false) && false == false
    if (true || false && false) fail = fail + 0; else fail = fail + 1;
    if ((a == 6) && (b == 3)) fail = fail + 0; else fail = fail + 1;

    // --- Bitwise (&, |, ^) ---
    fail = fail + assert_eq(a & b, 2, 0);
    fail = fail + assert_eq(a | b, 7, 1);
    fail = fail + assert_eq(a ^ b, 5, 2);
    // & binds tighter than |: 4 | (1 & 3) == 5, not (4 | 1) & 3 == 1
    fail = fail + assert_eq(4 | 1 & 3, 5, 3);

    // --- Shift (<<, >>) ---
    fail = fail + assert_eq(1 << 4, 16, 4);
    fail = fail + assert_eq(256 >> 4, 16, 5);
    // shift binds tighter than comparison: (2 << 1) == 4 is true
    if (2 << 1 == 4) fail = fail + 0; else fail = fail + 1;
    // shift binds looser than addition: (1 + 2) << 1 == 6, not 1 + (2 << 1) == 5
    fail = fail + assert_eq(1 + 2 << 1, 6, 6);

    // --- Coalesce (??, ?=) ---
    // a ?? b -> (a ? a : b): yields a when truthy/non-zero, else b
    fail = fail + assert_eq(0 ?? 5, 5, 7);
    fail = fail + assert_eq(3 ?? 5, 3, 8);
    // ?? binds looser than +: 0 ?? (1 + 2) == 3, not (0 ?? 1) + 2 == 2
    fail = fail + assert_eq(0 ?? 1 + 2, 3, 9);

    // a ?= b -> a = (a ? a : b): only overwrites a when it's zero
    i32 z = 0;
    z ?= 9;
    fail = fail + assert_eq(z, 9, 10);
    i32 nz = 4;
    nz ?= 9;
    fail = fail + assert_eq(nz, 4, 11);

    ret fail;
}
