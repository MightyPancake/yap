import std/strutils
import std/os

type
    Node = ref object
        value: int32
        left, right: Node

# Use explicit new/free, not GC, to match the pointer-chasing profile
# of C/Zig/yap. Nim's ref uses GC by default; this still exercises
# allocation indirection with the same algorithm shape.
proc build(lo, hi: int32): Node =
    if lo > hi: return nil
    let mid = lo + (hi - lo) div 2
    result = Node(value: mid)
    result.left  = build(lo, mid - 1)
    result.right = build(mid + 1, hi)

proc checksum(root: Node): uint64 =
    if root.isNil: return 0
    result = checksum(root.left)
    result += uint64(root.value + 1) * 2654435761'u64
    result += checksum(root.right)

proc freeTree(root: Node) =
    if root.isNil: return
    freeTree(root.left)
    freeTree(root.right)
    # Nim's GC collects automatically; explicit dealloc not needed.
    # We still traverse the tree (exercising pointer chasing) then
    # let the GC clean up.

when isMainModule:
    if paramCount() < 1:
        stderr.writeLine("Usage: btree <n>")
        quit(1)
    let n = parseInt(paramStr(1)).int32
    if n <= 0:
        echo 0'u64
        quit(0)
    let root = build(1, n)
    echo checksum(root)
