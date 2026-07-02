import io
import arr

struct Vec3 {
    i32[3] v,
}

i32 fn main() {
    i32[5] nums;
    nums:[0] = 72;
    nums:[1] = 101;
    nums:[2] = 108;
    nums:[3] = 108;
    nums:[4] = 111;

    io->putchar(nums:[0]);
    io->putchar(nums:[1]);
    io->putchar(nums:[2]);
    io->putchar(nums:[3]);
    io->putchar(nums:[4]);
    io->putchar(10);

    Vec3 pos;
    pos.v:[0] = 1;
    pos.v:[1] = 2;
    pos.v:[2] = pos.v:[0] + pos.v:[1];
    io->putchar(pos.v:[2] + 48);
    io->putchar(10);

    // Generic growable array (modules/arr), built entirely in yap via the
    // yapi.md builder API -- no C backend; see modules/arr/arr.yap.
    arr->arr:(i32) gnums;
    gnums:init();
    gnums:push(10);
    gnums:push(20);
    gnums:push(12);

    _ gtotal = gnums:at(0) + gnums:at(1) + gnums:at(2);
    if (gnums:len() == 3) io->puts(c"Generic array len OK");
    if (gtotal == 42) io->puts(c"Generic array push/at OK");
    gnums:free();

    ret gtotal - 42;
}
