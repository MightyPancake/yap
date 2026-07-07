import std/strutils
import std/os

# splitmix64 (public domain, Vigna). A single multiply-mod step (as used in
# the quicksort benchmark) turned out to have too little avalanche once
# reduced to a 4-symbol alphabet (near-identical buffers, LCS almost n);
# splitmix64's xor-shift/multiply finalizer avalanches properly even after
# `mod 4`.
proc splitmix64(xIn: uint64): uint64 =
    var x = xIn
    x = x + 0x9E3779B97F4A7C15'u64
    x = (x xor (x shr 30)) * 0xBF58476D1CE4E5B9'u64
    x = (x xor (x shr 27)) * 0x94D049BB133111EB'u64
    x = x xor (x shr 31)
    return x

type ByteArr = ptr UncheckedArray[uint8]
type IntArr = ptr UncheckedArray[int32]

# Different seed per buffer (not just a shifted index into the same stream)
# so A and B are independent-looking sequences, not one shifted copy of the
# other.
proc fill(buf: ByteArr, n: int32, seed: uint64) =
    for i in 0 ..< n:
        let idx = uint64(i + 1) + seed
        let h = splitmix64(idx) mod 4
        buf[i] = uint8(65 + h.int)

proc maxI32(a, b: int32): int32 =
    if a > b: a else: b

proc lcsLength(a, b: ByteArr, n: int32): int32 =
    var prev = cast[IntArr](alloc((n + 1) * sizeof(int32)))
    var curr = cast[IntArr](alloc((n + 1) * sizeof(int32)))
    for j in 0 .. n:
        prev[j] = 0

    for i in 1 .. n:
        curr[0] = 0
        for j in 1 .. n:
            if a[i - 1] == b[j - 1]:
                curr[j] = prev[j - 1] + 1
            else:
                curr[j] = maxI32(prev[j], curr[j - 1])
        swap(prev, curr)

    result = prev[n]
    dealloc(prev)
    dealloc(curr)

when isMainModule:
    if paramCount() < 1:
        stderr.writeLine("Usage: lcs <n>")
        quit(1)
    let n = parseInt(paramStr(1)).int32

    if n <= 0:
        echo 0
        quit(0)

    let a = cast[ByteArr](alloc(int(n)))
    let b = cast[ByteArr](alloc(int(n)))
    fill(a, n, 0'u64)
    fill(b, n, 0x9E3779B9'u64)

    let result = lcsLength(a, b, n)
    dealloc(a)
    dealloc(b)
    echo result
