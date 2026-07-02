import io
import stdlib

i32 fn main() {
    _ a = stdlib->abs(-42);
    // 42 + 48 = 90 = 'Z'
    io->putchar(a + 48);

    stdlib->srand(123);
    _ r1 = stdlib->rand();
    stdlib->srand(123);
    _ r2 = stdlib->rand();
    // same seed -> same result -> difference is 0 + 48 = '0'
    io->putchar(r1 - r2 + 48);

    _ n = stdlib->atoi(c"7");
    // 7 + 48 = 55 = '7'
    io->putchar(n + 48);

    _ ptr = stdlib->malloc(64).(byte@);
    ptr:[0] = '4';
    ptr:[1] = '2';
    ptr:[2] = '\0';
    io->puts(ptr);
    stdlib->free(ptr.(none@));

    io->putchar('\n');
    ret 0;
}
