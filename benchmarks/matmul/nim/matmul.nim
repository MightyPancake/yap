import std/strutils
import std/os

type F64Arr = ptr UncheckedArray[float64]

proc splitmix64(x: uint64): uint64 =
  var z = x + 0x9E3779B97F4A7C15'u64
  z = (z xor (z shr 30)) * 0xBF58476D1CE4E5B9'u64
  z = (z xor (z shr 27)) * 0x94D049BB133111EB'u64
  z = z xor (z shr 31)
  return z

proc fill(mat: F64Arr, n: int32, seed: uint64) =
  for i in 0 ..< n:
    for j in 0 ..< n:
      let idx = uint64(i * n + j + 1) + seed
      mat[i * n + j] = float64(splitmix64(idx) mod 1000003'u64)

proc matmul(a: F64Arr, b: F64Arr, c: F64Arr, n: int32) =
  for i in 0 ..< n:
    for j in 0 ..< n:
      c[i * n + j] = 0.0
    for k in 0 ..< n:
      let aik = a[i * n + k]
      for j in 0 ..< n:
        c[i * n + j] += aik * b[k * n + j]

proc checksum(mat: F64Arr, n: int32): float64 =
  var sum: float64 = 0.0
  let total = n * n
  for i in 0 ..< total:
    sum += mat[i]
  return sum

when isMainModule:
  if paramCount() < 1:
    stderr.writeLine("Usage: matmul <n>")
    quit(1)
  let n = parseInt(paramStr(1)).int32

  if n <= 0:
    echo "0.000000"
    quit(0)

  let total = n * n
  let a = cast[F64Arr](alloc(total * sizeof(float64)))
  let b = cast[F64Arr](alloc(total * sizeof(float64)))
  let c = cast[F64Arr](alloc(total * sizeof(float64)))

  fill(a, n, 0)
  fill(b, n, 0x9E3779B9'u64)

  matmul(a, b, c, n)

  let result = checksum(c, n)
  dealloc(a)
  dealloc(b)
  dealloc(c)

  echo formatFloat(result, ffDecimal, 6)
