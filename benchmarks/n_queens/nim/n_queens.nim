import std/strutils
import std/os

proc solve(cols, diag1, diag2, full: uint64): uint64 =
    if cols == full:
        return 1
    var avail = full and not (cols or diag1 or diag2)
    var total: uint64 = 0
    while avail != 0:
        let bit = avail and (not avail + 1)
        avail = avail xor bit
        total = total + solve(cols or bit, (diag1 or bit) shl 1, (diag2 or bit) shr 1, full)
    return total

when isMainModule:
    if paramCount() < 1:
        stderr.writeLine("Usage: n_queens <n>")
        quit(1)
    let n = parseBiggestUint(paramStr(1))
    let full: uint64 = if n >= 64: high(uint64) else: (uint64(1) shl n) - 1
    echo solve(0, 0, 0, full)
