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

    // An untyped literal adapts to any compatible numeric target, not just
    // its own default (i32 for ints, f32 for floats).
    u64 wide_u64 = 42;
    i64 wide_i64 = -100;
    u32 wide_u32 = 300000;
    f64 wide_f64 = 1;
    f32 int_into_f32 = 7;
    byte small_byte = 65;
    u64 max_u64 = 18446744073709551615;
    i64 min_i64 = -9223372036854775808;

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
    if (wide_u64 == 42) io->print:(c"wide_u64 OK\n");
    if (wide_i64 == -100) io->print:(c"wide_i64 OK\n");
    if (wide_u32 == 300000) io->print:(c"wide_u32 OK\n");
    if (wide_f64 == 1.0) io->print:(c"wide_f64 OK\n");
    if (int_into_f32 == 7.0) io->print:(c"int_into_f32 OK\n");
    if (small_byte == 65) io->print:(c"small_byte OK\n");
    if (max_u64 == 18446744073709551615) io->print:(c"max_u64 OK\n");
    if (min_i64 == -9223372036854775808) io->print:(c"min_i64 OK\n");

    ret 0;
}
