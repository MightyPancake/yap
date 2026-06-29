import io

struct Vec3 {
    i32[3] v,
}

fn main() {
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
}
