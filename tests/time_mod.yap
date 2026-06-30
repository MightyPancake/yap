import io
import time

i32 fn main() {
    // time(null) returns current unix timestamp (> 0)
    _ t = time->time(null.(i64@));

    // clock() returns processor ticks (>= 0)
    _ c = time->clock();

    // difftime(t, t) = 0.0
    _ d = time->difftime(t, t);

    // difftime(t, t) should be 0
    if (d == 0.0) io->putchar('O');
    else io->putchar('X');

    // time() should return nonzero (unix timestamp)
    if (t != 0) io->putchar('K');
    else io->putchar('X');

    io->putchar('\n');
    ret 0;
}
