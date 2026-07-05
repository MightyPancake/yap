import io
import foo_test_module

struct Point {
    i32 x,
    i32 y,
}

i32 fn sub(i32 a, i32 b = 100) {
    ret a - b;
}

i32 fn triple(i32 a = 1, i32 b = 2, i32 c = 3) {
    ret a * 100 + b * 10 + c;
}

i32 fn Point p:scaled_sum(i32 scale = 1) {
    ret (p.x + p.y) * scale;
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

    // Named arg, default fills the rest
    fail = fail + assert_eq(sub(.a=10), -90, 1);

    // Positional first, named override after
    fail = fail + assert_eq(sub(10, .b=3), 7, 2);

    // All named, reverse declaration order
    fail = fail + assert_eq(sub(.b=1, .a=5), 4, 3);

    // All named, every param out of order
    fail = fail + assert_eq(triple(.c=9, .a=1, .b=2), 129, 4);

    // Some named, some defaulted
    fail = fail + assert_eq(triple(.b=5), 153, 5);

    // Method call: named arg overrides default
    Point pt = [3, 4];
    fail = fail + assert_eq(pt:scaled_sum(.scale=2), 14, 6);

    // Method call: named arg matches the default value
    fail = fail + assert_eq(pt:scaled_sum(), 7, 7);

    // Module function with a named argument
    fail = fail + assert_eq(foo_test_module->add_default(.a=5, .b=3), 8, 8);

    if (fail == 0) io->print:(c"OK\n");

    ret fail;
}
