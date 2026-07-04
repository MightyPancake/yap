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

// --- $x in a fn$ body is an EAGER splice (fn$ has no fill methods, so a lazy
// hole here could never be closed), and yFn:ref() gives a callable yExpr for
// an already-finished function -- composed together: build+emit a function
// whose body eagerly splices an outer comptime yExpr ($bonus, this function's
// own param), then build+return an actual call to it.
yExpr fn make_and_call_adder(yExpr bonus, yExpr arg) {
    _ ft = (i32 fn$ i32 a) {
        ret a + $bonus;
    };
    _ f = ft:finish(c"adder_with_bonus");
    ret yapi->call1(f:ref(), arg);
}

i32 fn main() {
    boxed:(i32) b;
    b.val = 42;
    if (b.val == 42) { io->print:(c"fn blueprint built+emitted OK\n"); }

    _ result = make_and_call_adder:(#10, #5); // adder_with_bonus(5) = 5 + 10 = 15
    if (result == 15) { io->print:(c"fn$ eager splice + callable ref OK\n"); }

    ret result - 15;
}
