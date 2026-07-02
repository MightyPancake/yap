import io
import foo_test_module
import "imports/defaults_helper.yap"

struct Point {
    i32 x = 0,
    i32 y = 0,
}

struct Mixed {
    i32 a,
    i32 b = 10,
    i32 c = 20,
}

i32 fn add(i32 a, i32 b = 1) {
    ret a + b;
}

i32 fn triple_default(i32 a = 1, i32 b = 2, i32 c = 3) {
    ret a + b + c;
}

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

    // --- Struct field defaults ---

    // Override first, default second
    Point p2 = [5];
    fail = fail + assert_eq(p2.x, 5, 1);
    fail = fail + assert_eq(p2.y, 0, 2);

    // Override both (no defaults used)
    Point p3 = [3, 7];
    fail = fail + assert_eq(p3.x, 3, 3);
    fail = fail + assert_eq(p3.y, 7, 4);

    // Named override, rest default
    Point p4 = [.y=9];
    fail = fail + assert_eq(p4.x, 0, 5);
    fail = fail + assert_eq(p4.y, 9, 6);

    // Mixed struct: first field required, rest have defaults
    Mixed m1 = [5];
    fail = fail + assert_eq(m1.a, 5, 7);

    // Mixed struct: override first two, default third
    Mixed m2 = [1, 2];
    fail = fail + assert_eq(m2.a, 1, 8);
    fail = fail + assert_eq(m2.b, 2, 9);

    // --- Function parameter defaults ---

    fail = fail + assert_eq(add(5), 6, 0);
    fail = fail + assert_eq(add(5, 3), 8, 1);
    fail = fail + assert_eq(triple_default(4), 9, 2);
    fail = fail + assert_eq(triple_default(1, 1, 1), 3, 3);

    // --- Cross-file defaults ---

    // Struct from imported file, partial override
    Color c2 = [.r=5];
    fail = fail + assert_eq(c2.r, 5, 4);
    fail = fail + assert_eq(c2.g, 0, 5);
    fail = fail + assert_eq(c2.b, 0, 6);

    // Function from imported file, use default
    fail = fail + assert_eq(add_with_offset(3, 4), 7, 7);

    // Function from imported file, override default
    fail = fail + assert_eq(add_with_offset(3, 4, 1), 8, 8);

    // --- Module defaults ---

    // Module function with default, use default
    fail = fail + assert_eq(foo_test_module->add_default(5), 15, 0);

    // Module function with default, override default
    fail = fail + assert_eq(foo_test_module->add_default(5, 3), 8, 1);

    if (fail == 0) {
        io->putchar(79); // 'O'
        io->putchar(75); // 'K'
        io->putchar(10);
    }

    // Defaults do not propagate magically
    _ func = triple_default;
    func(1, 2, 3);
    //func(1); // This would not work

    ret fail;
}
