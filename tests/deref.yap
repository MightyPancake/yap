import io

struct Point {
    i32 x,
    i32 y,
}

i32 fn main() {
    // Deref a pointer to a primitive
    i32 n = 7;
    i32@ np = n@;
    i32 v1 = np.;
    io->putchar(v1 + 48);
    io->putchar(10);

    // Deref a pointer to a struct (whole-value deref)
    Point p = [3, 9];
    Point@ pp = p@;
    Point v2 = pp.;
    io->putchar(v2.x + 48);
    io->putchar(44);
    io->putchar(v2.y + 48);
    io->putchar(10);

    // Assign through a deref (a deref is an lvalue)
    np. = 5;
    io->putchar(n + 48);
    io->putchar(10);

    ret 0;
}
