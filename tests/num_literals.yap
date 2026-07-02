import io

i32 fn main() {
    i32 hex = 0x1A;
    i32 oct = 0o17;
    i32 bin = 0b1010;
    i32 neg = -7;
    i32 neg_hex = -0x10;
    f32 flt = 2e2;
    f32 flt2 = 1.5E-2;
    f32 flt2_expected = 0.015;
    i32 min_i32 = -2147483648;
    i32 max_i32 = 2147483647;

    if (hex == 26) io->print:(c"hex OK\n");
    if (oct == 15) io->print:(c"oct OK\n");
    if (bin == 10) io->print:(c"bin OK\n");
    if (neg == -7) io->print:(c"neg OK\n");
    if (neg_hex == -16) io->print:(c"neg_hex OK\n");
    if (flt == 200.0) io->print:(c"flt OK\n");
    // Compared against another f32, not the bare double literal 0.015 directly --
    // 0.015 isn't exactly representable in binary floating point, so comparing
    // an f32 against the double literal fails on the double-rounding, not on
    // anything related to exponent-literal parsing.
    if (flt2 == flt2_expected) io->print:(c"flt2 OK\n");
    if (min_i32 == -2147483648) io->print:(c"min_i32 OK\n");
    if (max_i32 == 2147483647) io->print:(c"max_i32 OK\n");

    ret 0;
}
