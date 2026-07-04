import io

// (RET fn$ params){body} eager blueprint -> yFnT (Model A); :finish emits the function.
// $T splices the in-scope comptime yType; the body refs the fn's own params (a, b).
// Exercised here inside a type macro (composing use): it builds+emits an adder via
// fn$ as a side effect, then returns a struct the caller uses.
yType fn boxed(yType T) {
    _ ft = ($T fn$ $T a, $T b) {
        ret a + b;
    };
    _ f = ft:finish(c"bp_add");   // emits the function

    _ st = type${ struct { $T val } };
    ret st:finish(c"boxed");
}

i32 fn main() {
    boxed:(i32) b;
    b.val = 42;
    if (b.val == 42) { io->print:(c"fn blueprint built+emitted OK\n"); }
    ret 0;
}
