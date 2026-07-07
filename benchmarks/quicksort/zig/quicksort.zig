const std = @import("std");

fn swapElem(arr: []i32, i: i32, j: i32) void {
    const tmp = arr[@intCast(i)];
    arr[@intCast(i)] = arr[@intCast(j)];
    arr[@intCast(j)] = tmp;
}

fn partition(arr: []i32, lo: i32, hi: i32) i32 {
    const pivot = arr[@intCast(hi)];
    var i = lo - 1;
    var j = lo;
    while (j < hi) : (j += 1) {
        if (arr[@intCast(j)] <= pivot) {
            i += 1;
            swapElem(arr, i, j);
        }
    }
    swapElem(arr, i + 1, hi);
    return i + 1;
}

fn quicksort(arr: []i32, lo: i32, hi: i32) void {
    if (lo < hi) {
        const p = partition(arr, lo, hi);
        quicksort(arr, lo, p - 1);
        quicksort(arr, p + 1, hi);
    }
}

fn fill(arr: []i32, n: i32) void {
    var i: i32 = 0;
    while (i < n) : (i += 1) {
        const idx: u64 = @intCast(i + 1);
        arr[@intCast(i)] = @intCast((idx *% 2654435761) % 1000003);
    }
}

fn checksum(arr: []i32, n: i32) u64 {
    var sum: u64 = 0;
    var i: i32 = 0;
    while (i < n) : (i += 1) {
        const v: u64 = @intCast(arr[@intCast(i)]);
        const idx: u64 = @intCast(i + 1);
        sum +%= v *% idx;
    }
    return sum;
}

pub fn main(init: std.process.Init) !void {
    var it = init.minimal.args.iterate();
    _ = it.skip(); // program name

    const n_str = it.next() orelse {
        std.debug.print("Usage: quicksort <n>\n", .{});
        std.process.exit(1);
    };

    const n = try std.fmt.parseInt(i32, n_str, 10);

    var buf: [32]u8 = undefined;
    var stdout = std.Io.File.stdout().writer(init.io, &buf);

    if (n <= 0) {
        try stdout.interface.print("{d}\n", .{@as(u64, 0)});
        try stdout.interface.flush();
        return;
    }

    const allocator = std.heap.page_allocator;
    const arr = try allocator.alloc(i32, @intCast(n));
    defer allocator.free(arr);

    fill(arr, n);
    quicksort(arr, 0, n - 1);
    const result = checksum(arr, n);

    try stdout.interface.print("{d}\n", .{result});
    try stdout.interface.flush();
}
