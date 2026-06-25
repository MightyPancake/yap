import io
import math

i32 fn main() {
    // sqrt(9.0) = 3.0 -> 3 + 48 = 51 = '3'
    _ s = math->sqrt(9.0);
    io->putchar(s.(i32) + 48);

    // fabs(-5.0) = 5.0 -> 5 + 48 = 53 = '5'
    _ a = math->fabs(-5.0);
    io->putchar(a.(i32) + 48);

    // ceil(2.3) = 3.0 -> 3 + 48 = 51 = '3'
    _ c = math->ceil(2.3);
    io->putchar(c.(i32) + 48);

    // floor(7.9) = 7.0 -> 7 + 48 = 55 = '7'
    _ f = math->floor(7.9);
    io->putchar(f.(i32) + 48);

    // pow(2.0, 3.0) = 8.0 -> 8 + 48 = 56 = '8'
    _ p = math->pow(2.0, 3.0);
    io->putchar(p.(i32) + 48);

    io->putchar('\n');
    ret 0;
}
