import io

i32 fn main() {
    // Numeric <-> numeric: any width/signedness/float-ness, C-style truncation
    i32 a = 300;
    byte b = a.(byte);          // truncates like a raw C cast (300 & 0xFF = 44)
    f32 f = a.(f32);
    i32 back = f.(i32);
    u64 widened = a.(u64);

    // Numeric <-> pointer, pointer <-> pointer (including unrelated pointee types)
    i32@ p = a@;
    i64 addr = p.(i64);
    i64@ p2 = p.(i64@);
    f32@ p3 = p.(f32@);
    none@ vp = p.(none@);
    i32@ back_p = vp.(i32@);

    // Casting anything to bare 'none' is always allowed, like C's "(void)expr"
    a.(none);

    i32 ok = 0;
    if (b == 44) { io->print:(c"num-to-byte truncation OK\n"); ok = ok + 1; }
    if (back == 300) { io->print:(c"num round-trip through f32 OK\n"); ok = ok + 1; }
    if (widened == 300) { io->print:(c"widen through cast OK\n"); ok = ok + 1; }
    if (addr != 0) { io->print:(c"pointer-to-int OK\n"); ok = ok + 1; }
    if (back_p. == 300) { io->print:(c"pointer round-trip through none@ OK\n"); ok = ok + 1; }

    ret ok - 5;
}
