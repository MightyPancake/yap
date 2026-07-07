const std = @import("std");

fn splitmix64(x: u64) u64 {
    var z: u64 = x +% 0x9E3779B97F4A7C15;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    z = z ^ (z >> 31);
    return z;
}

fn fill(mat: []f64, n: i32, seed: u64) void {
    var i: i32 = 0;
    while (i < n) : (i += 1) {
        var j: i32 = 0;
        while (j < n) : (j += 1) {
            const idx: u64 = @intCast(i * n + j + 1);
            const h = splitmix64(idx +% seed) % 1000003;
            mat[@intCast(i * n + j)] = @floatFromInt(h);
        }
    }
}

fn matmul(a: []const f64, b: []const f64, c: []f64, n: i32) void {
    var i: i32 = 0;
    while (i < n) : (i += 1) {
        var j: i32 = 0;
        while (j < n) : (j += 1) {
            c[@intCast(i * n + j)] = 0.0;
        }
        var k: i32 = 0;
        while (k < n) : (k += 1) {
            const aik = a[@intCast(i * n + k)];
            var j2: i32 = 0;
            while (j2 < n) : (j2 += 1) {
                c[@intCast(i * n + j2)] += aik * b[@intCast(k * n + j2)];
            }
        }
    }
}

fn checksum(mat: []const f64, n: i32) f64 {
    var sum: f64 = 0.0;
    const total = n * n;
    var i: i32 = 0;
    while (i < total) : (i += 1) {
        sum += mat[@intCast(i)];
    }
    return sum;
}

pub fn main(init: std.process.Init) !void {
    var it = init.minimal.args.iterate();
    _ = it.skip(); // program name

    const n_str = it.next() orelse {
        std.debug.print("Usage: matmul <n>\n", .{});
        std.process.exit(1);
    };

    const n = try std.fmt.parseInt(i32, n_str, 10);

    var buf: [64]u8 = undefined;
    var stdout = std.Io.File.stdout().writer(init.io, &buf);

    if (n <= 0) {
        try stdout.interface.print("0.000000\n", .{});
        try stdout.interface.flush();
        return;
    }

    const total: usize = @intCast(n * n);
    const allocator = std.heap.page_allocator;
    const a = try allocator.alloc(f64, total);
    const b = try allocator.alloc(f64, total);
    const c = try allocator.alloc(f64, total);
    defer allocator.free(a);
    defer allocator.free(b);
    defer allocator.free(c);

    fill(a, n, 0);
    fill(b, n, 0x9E3779B9);

    matmul(a, b, c, n);

    const result = checksum(c, n);

    try stdout.interface.print("{d:.6}\n", .{result});
    try stdout.interface.flush();
}
