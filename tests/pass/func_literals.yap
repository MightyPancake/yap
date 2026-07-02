import io
import "imports/func_literal_helper.yap"

i32 fn apply((i32 fn i32, i32) op, i32 a, i32 b) {
    ret op(a, b);
}

i32 fn main() {
    // Assign a function literal to a variable, then call it
    _ inc = (i32 fn i32 x) { ret x + 1; };
    _ five = inc(4);
    if (five == 5) io->puts(c"Function literal assign+call OK");

    // Pass a function literal directly as an argument
    _ sum = apply((i32 fn i32 a, i32 b) { ret a + b; }, 30, 7);
    if (sum == 37) io->puts(c"Function literal as argument OK");

    // No params, explicit return type
    _ answer = (i32 fn) { ret 42; };

    // No params, no return type (void)
    _ shout = (fn) { io->puts(c"Void function literal OK"); };
    shout();

    // Literal minted in an imported file must not collide with ours
    _ seven = seven_via_literal();
    if (seven == 7) io->puts(c"Cross-file literal OK");

    ret five + sum + answer() + seven - 91; // 5 + 37 + 42 + 7 - 91 = 0
}
