const std = @import("std");

fn fib(n: u64) u64 {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

pub fn main(init: std.process.Init) !void {
    var it = init.minimal.args.iterate();
    _ = it.skip(); // program name

    const n_str = it.next() orelse {
        std.debug.print("Usage: fib <n>\n", .{});
        std.process.exit(1);
    };

    const n = try std.fmt.parseInt(u64, n_str, 10);

    // std.debug.print writes to stderr; the benchmark harness reads stdout.
    var buf: [32]u8 = undefined;
    var stdout = std.Io.File.stdout().writer(init.io, &buf);
    try stdout.interface.print("{d}\n", .{fib(n)});
    try stdout.interface.flush();
}
