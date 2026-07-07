const std = @import("std");

// splitmix64 (public domain, Vigna). A single multiply-mod step (as used in
// the quicksort benchmark) turned out to have too little avalanche once
// reduced to a 4-symbol alphabet (near-identical buffers, LCS almost n);
// splitmix64's xor-shift/multiply finalizer avalanches properly even after
// `% 4`.
fn splitmix64(x_in: u64) u64 {
    var x = x_in;
    x +%= 0x9E3779B97F4A7C15;
    x = (x ^ (x >> 30)) *% 0xBF58476D1CE4E5B9;
    x = (x ^ (x >> 27)) *% 0x94D049BB133111EB;
    x = x ^ (x >> 31);
    return x;
}

// Different seed per buffer (not just a shifted index into the same stream)
// so A and B are independent-looking sequences, not one shifted copy of the
// other.
fn fill(buf: []u8, n: i32, seed: u64) void {
    var i: i32 = 0;
    while (i < n) : (i += 1) {
        const idx: u64 = @as(u64, @intCast(i + 1)) +% seed;
        const h = splitmix64(idx) % 4;
        buf[@intCast(i)] = @intCast(@as(u64, 'A') + h);
    }
}

fn maxI32(a: i32, b: i32) i32 {
    return if (a > b) a else b;
}

fn lcsLength(allocator: std.mem.Allocator, a: []const u8, b: []const u8, n: i32) !i32 {
    var prev = try allocator.alloc(i32, @intCast(n + 1));
    defer allocator.free(prev);
    var curr = try allocator.alloc(i32, @intCast(n + 1));
    defer allocator.free(curr);

    for (prev) |*v| v.* = 0;

    var i: i32 = 1;
    while (i <= n) : (i += 1) {
        curr[0] = 0;
        var j: i32 = 1;
        while (j <= n) : (j += 1) {
            if (a[@intCast(i - 1)] == b[@intCast(j - 1)]) {
                curr[@intCast(j)] = prev[@intCast(j - 1)] + 1;
            } else {
                curr[@intCast(j)] = maxI32(prev[@intCast(j)], curr[@intCast(j - 1)]);
            }
        }
        const tmp = prev;
        prev = curr;
        curr = tmp;
    }

    return prev[@intCast(n)];
}

pub fn main(init: std.process.Init) !void {
    var it = init.minimal.args.iterate();
    _ = it.skip(); // program name

    const n_str = it.next() orelse {
        std.debug.print("Usage: lcs <n>\n", .{});
        std.process.exit(1);
    };

    const n = try std.fmt.parseInt(i32, n_str, 10);

    var buf: [32]u8 = undefined;
    var stdout = std.Io.File.stdout().writer(init.io, &buf);

    if (n <= 0) {
        try stdout.interface.print("{d}\n", .{@as(i32, 0)});
        try stdout.interface.flush();
        return;
    }

    const allocator = std.heap.page_allocator;
    const a = try allocator.alloc(u8, @intCast(n));
    defer allocator.free(a);
    const b = try allocator.alloc(u8, @intCast(n));
    defer allocator.free(b);

    fill(a, n, 0);
    fill(b, n, 0x9E3779B9);

    const result = try lcsLength(allocator, a, b, n);

    try stdout.interface.print("{d}\n", .{result});
    try stdout.interface.flush();
}
