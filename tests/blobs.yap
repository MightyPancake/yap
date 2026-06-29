import io

struct Point {
    i32 x,
    i32 y,
}

i32 fn sum_point(Point p) {
    ret p.x + p.y;
}

i32 fn main() {
    // Blob cast to struct (positional)
    _ p1 = [1, 2].(Point);
    io->putchar(p1.x + 48);
    io->putchar(44);
    io->putchar(p1.y + 48);
    io->putchar(10);

    // Blob cast to struct (named)
    _ p2 = [.y=3, .x=4].(Point);
    io->putchar(p2.x + 48);
    io->putchar(44);
    io->putchar(p2.y + 48);
    io->putchar(10);

    // Blob cast to array
    i32[3] arr = [5, 6, 7].(i32[3]);
    io->putchar(arr:[0] + 48);
    io->putchar(arr:[1] + 48);
    io->putchar(arr:[2] + 48);
    io->putchar(10);

    // Blob assigned to typed variable (auto-cast)
    Point p3 = [1, 3];
    io->putchar(p3.x + 48);
    io->putchar(44);
    io->putchar(p3.y + 48);
    io->putchar(10);

    // Array auto-cast via typed variable
    i32[2] arr2 = [8, 9];
    io->putchar(arr2:[0] + 48);
    io->putchar(arr2:[1] + 48);
    io->putchar(10);

    // Blob cast to slice
    _ s = [1, 2, 3].(i32[]);
    io->putchar(s:[0] + 48);
    io->putchar(s:[1] + 48);
    io->putchar(s:[2] + 48);
    io->putchar(10);

    // Blob as function argument (auto-cast)
    _ r = sum_point([3, 4]);
    io->putchar(r + 48);
    io->putchar(10);

    ret 0;
}
