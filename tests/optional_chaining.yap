import io

struct Inner {
    i32 val,
}

struct Outer {
    Inner@ inner,
}

i32 fn main() {
    // Optional chaining on a non-null pointer dereferences normally
    Inner in = [42];
    Inner@ in_ptr = in@;
    _ v1 = in_ptr?.val;
    io->putchar(v1 / 10 + 48);
    io->putchar(v1 % 10 + 48);
    io->putchar(10);

    // Optional chaining on a null pointer falls back to the zero value
    Inner@ null_in = null;
    _ v2 = null_in?.val;
    io->putchar(v2 + 48);
    io->putchar(10);

    // Chained optional access through two levels of pointers
    Outer out = [in_ptr];
    Outer@ out_ptr = out@;
    _ v3 = out_ptr?.inner?.val;
    io->putchar(v3 / 10 + 48);
    io->putchar(v3 % 10 + 48);
    io->putchar(10);

    // Chained optional access short-circuits at the first null
    Outer@ null_out = null;
    _ v4 = null_out?.inner?.val;
    io->putchar(v4 + 48);
    io->putchar(10);

    ret 0;
}
