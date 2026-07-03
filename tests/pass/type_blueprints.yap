import io

// type${ } eager blueprint -> yStructT template, :finish() as usual (Model A).
// $T splices the in-scope comptime yType param; a named type -> yapi->type(c"...").
yType fn pair(yType T) {
    _ st = type${
        struct {
            $T first,
            $T second
        }
    };
    ret st:finish(c"pair");
}

yType fn boxed() {
    _ st = type${
        struct {
            i32 value,
            i32 count
        }
    };
    ret st:finish(c"boxed");
}

i32 fn main() {
    pair:(i32) p;
    p.first = 10;
    p.second = 32;

    boxed:() b;
    b.value = 5;
    b.count = 7;

    i32 ok = 0;
    if (p.first + p.second == 42) { io->print:(c"type bp $T splice OK\n"); ok = ok + 1; }
    if (b.value + b.count == 12)  { io->print:(c"type bp named field OK\n"); ok = ok + 1; }
    ret ok - 2;
}
