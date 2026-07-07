import std/strutils
import std/os

proc fib(n: uint64): uint64 =
    if n <= 1: n
    else: fib(n - 1) + fib(n - 2)

when isMainModule:
    if paramCount() < 1:
        stderr.writeLine("Usage: fib <n>")
        quit(1)
    let n = parseBiggestUint(paramStr(1))
    echo fib(n)
