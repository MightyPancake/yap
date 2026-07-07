import std/strutils
import std/os

type IntArr = ptr UncheckedArray[int32]

# ptr UncheckedArray (not seq) so passing the array around a recursive call is
# a raw pointer copy, not a seq value-copy -- matches C's pointer semantics
# exactly and avoids any ambiguity around Nim's seq copy-on-assign behavior.
proc swapElem(arr: IntArr, i, j: int32) =
    let tmp = arr[i]
    arr[i] = arr[j]
    arr[j] = tmp

proc partition(arr: IntArr, lo, hi: int32): int32 =
    let pivot = arr[hi]
    var i = lo - 1
    for j in lo ..< hi:
        if arr[j] <= pivot:
            i += 1
            swapElem(arr, i, j)
    swapElem(arr, i + 1, hi)
    return i + 1

proc quicksort(arr: IntArr, lo, hi: int32) =
    if lo < hi:
        let p = partition(arr, lo, hi)
        quicksort(arr, lo, p - 1)
        quicksort(arr, p + 1, hi)

proc fill(arr: IntArr, n: int32) =
    for i in 0 ..< n:
        arr[i] = int32((uint64(i + 1) * 2654435761'u64) mod 1000003'u64)

proc checksum(arr: IntArr, n: int32): uint64 =
    var sum: uint64 = 0
    for i in 0 ..< n:
        sum += uint64(arr[i]) * uint64(i + 1)
    return sum

when isMainModule:
    if paramCount() < 1:
        stderr.writeLine("Usage: quicksort <n>")
        quit(1)
    let n = parseInt(paramStr(1)).int32

    if n <= 0:
        echo 0'u64
        quit(0)

    let arr = cast[IntArr](alloc(int(n) * sizeof(int32)))
    fill(arr, n)
    quicksort(arr, 0, n - 1)
    let result = checksum(arr, n)
    dealloc(arr)
    echo result
