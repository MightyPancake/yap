// Profiling example: struct methods, generic arrays, HOFs, loops, C modules.
import io
import math
import arr

struct Vec2 {
    f64 x,
    f64 y,
}

f64 fn Vec2 v:length() {
    ret math->sqrt(v.x * v.x + v.y * v.y);
}

Vec2 fn Vec2 v:scaled(f64 factor) {
    ret [v.x * factor, v.y * factor].(Vec2);
}

f64 fn Vec2 a:dot(Vec2 b) {
    ret a.x * b.x + a.y * b.y;
}

i32 fn square(i32 x) {
    ret x * x;
}

i32 fn sum_range(i32 n) {
    i32 total = 0;
    for (i32 i = 0; i < n; i = i + 1)
        total = total + i;
    ret total;
}

i32 fn factorial(i32 n) {
    i32 result = 1;
    i32 i = 1;
    while (i <= n) {
        result = result * i;
        i = i + 1;
    }
    ret result;
}

i32 fn main() {
    Vec2 a = [3.0, 4.0];
    Vec2 b = [1.0, 2.0];

    f64 len = a:length();
    io->print:(c"Vector length:\n");
    io->putchar(len.(i32) + 48);
    io->putchar(10);

    Vec2 scaled = a:scaled(2.0);
    io->print:(c"Scaled x:\n");
    io->putchar(scaled.x.(i32) + 48);
    io->putchar(10);

    f64 d = a:dot(b);
    io->print:(c"Dot product:\n");
    io->putchar(d.(i32) + 48);
    io->putchar(10);

    _ total = sum_range(10);
    io->print:(c"Sum range mod 10:\n");
    io->putchar((total % 10) + 48);
    io->putchar(10);

    _ fact = factorial(5);
    io->print:(c"Factorial mod 10:\n");
    io->putchar((fact % 10) + 48);
    io->putchar(10);

    arr->arr:(i32) nums;
    nums:init();
    i32 i = 0;
    while (i < 20) {
        nums:push(i);
        i = i + 1;
    }

    _ squared = nums:map((i32 fn i32 x) { ret x * x; });
    _ evens = nums:filter((bool fn i32 x) { ret (x % 2) == 0; });
    _ total_sum = nums:fold((i32 fn i32 acc, i32 x) { ret acc + x; }, 0);

    io->print:(c"Squared count:\n");
    io->putchar((squared:len() % 10).(i32) + 48);
    io->putchar(10);

    io->print:(c"Evens count:\n");
    io->putchar((evens:len() % 10).(i32) + 48);
    io->putchar(10);

    io->print:(c"Fold total mod 10:\n");
    io->putchar((total_sum % 10) + 48);
    io->putchar(10);

    nums:free();
    squared:free();
    evens:free();

    ret 0;
}
