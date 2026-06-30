import io

struct Point {
    i32 x,
    i32 y,
}

struct Vector {
    i32 x,
    i32 y,
}

i32 fn len(Point p) {
    ret p.x + p.y;
}

i32 fn len(Vector v) {
    ret v.x * v.y;
}

Point fn scaled(Point p, i32 factor) {
    ret [p.x * factor, p.y * factor].(Point);
}

i32 fn main() {
    Point p = [2, 3];
    Vector v = [2, 3];

    // Same method name on two different types: no collision.
    io->putchar(p:len() + 48);
    io->putchar(10);
    io->putchar(v:len() + 48);
    io->putchar(10);

    // Chained method calls: scaled() returns a Point, call len() on the result.
    io->putchar(p:scaled(2):len() + 48);
    io->putchar(10);

    ret 0;
}
