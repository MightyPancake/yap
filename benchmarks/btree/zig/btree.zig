const std = @import("std");

const Node = struct {
    value: i32,
    left: ?*Node,
    right: ?*Node,
};

fn build(allocator: std.mem.Allocator, lo: i32, hi: i32) std.mem.Allocator.Error!?*Node {
    if (lo > hi) return null;
    const mid = lo + @divTrunc((hi - lo), 2);
    const n = try allocator.create(Node);
    n.* = .{ .value = mid, .left = null, .right = null };
    n.left  = try build(allocator, lo, mid - 1);
    n.right = try build(allocator, mid + 1, hi);
    return n;
}

fn checksum(root: ?*Node) u64 {
    var sum: u64 = 0;
    _ = traversal(root, &sum);
    return sum;
}

fn traversal(node: ?*Node, sum: *u64) void {
    if (node) |n| {
        traversal(n.left, sum);
        sum.* +%= @as(u64, @intCast(n.value + 1)) *% 2654435761;
        traversal(n.right, sum);
    }
}

fn freeTree(allocator: std.mem.Allocator, root: ?*Node) void {
    if (root) |n| {
        freeTree(allocator, n.left);
        freeTree(allocator, n.right);
        allocator.destroy(n);
    }
}

pub fn main(init: std.process.Init) !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    var it = init.minimal.args.iterate();
    _ = it.skip(); // program name

    const n_str = it.next() orelse {
        std.debug.print("Usage: btree <n>\n", .{});
        std.process.exit(1);
    };

    const n = try std.fmt.parseInt(i32, n_str, 10);
    if (n <= 0) {
        var buf: [32]u8 = undefined;
        var stdout = std.Io.File.stdout().writer(init.io, &buf);
        try stdout.interface.print("0\n", .{});
        try stdout.interface.flush();
        return;
    }

    const root = try build(allocator, 1, n);
    const ck = checksum(root);
    freeTree(allocator, root);

    var buf: [32]u8 = undefined;
    var stdout = std.Io.File.stdout().writer(init.io, &buf);
    try stdout.interface.print("{d}\n", .{ck});
    try stdout.interface.flush();
}
