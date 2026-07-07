const std = @import("std");

fn solve(cols: u64, diag1: u64, diag2: u64, full: u64) u64 {
    if (cols == full) return 1;
    var avail = full & ~(cols | diag1 | diag2);
    var total: u64 = 0;
    while (avail != 0) {
        const bit = avail & (~avail +% 1);
        avail = avail ^ bit;
        total += solve(cols | bit, (diag1 | bit) << 1, (diag2 | bit) >> 1, full);
    }
    return total;
}

pub fn main(init: std.process.Init) !void {
    var it = init.minimal.args.iterate();
    _ = it.skip(); // program name

    const n_str = it.next() orelse {
        std.debug.print("Usage: n_queens <n>\n", .{});
        std.process.exit(1);
    };

    const n = try std.fmt.parseInt(u64, n_str, 10);
    const shift_n: u6 = if (n >= 63) 63 else @intCast(n);
    const full: u64 = if (n >= 64) std.math.maxInt(u64) else (@as(u64, 1) << shift_n) - 1;

    var buf: [32]u8 = undefined;
    var stdout = std.Io.File.stdout().writer(init.io, &buf);
    try stdout.interface.print("{d}\n", .{solve(0, 0, 0, full)});
    try stdout.interface.flush();
}
