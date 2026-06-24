import "../modules/io/binds.yap"

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

    putchar(nums:[0]);
    putchar(nums:[1]);
    putchar(nums:[2]);
    putchar(nums:[3]);
    putchar(nums:[4]);
    putchar(10);

    Vec3 pos;
    pos.v:[0] = 1;
    pos.v:[1] = 2;
    pos.v:[2] = pos.v:[0] + pos.v:[1];
    putchar(pos.v:[2] + 48);
    putchar(10);
}
