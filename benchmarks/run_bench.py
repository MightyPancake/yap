#!/usr/bin/env python3
"""
Benchmark runner for YAP thesis Chapter 11.

Runs compilation and execution benchmarks across C, Nim, Zig, and YAP
for each problem directory. Produces timing results in CSV and a summary
markdown table.
"""

import argparse
import csv
import json
import os
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

BENCH_DIR = Path(__file__).resolve().parent
YAP_DIR = BENCH_DIR.parent  # Root of the yap repo


def detect_fast_cores() -> Optional[set[int]]:
    """On a hybrid-core CPU (e.g. AMD's Zen5 + Zen5c "compact" core mix, or
    Intel's P-core/E-core split), the OS scheduler can land a freshly-spawned
    single-threaded benchmark process on a slow core right after a heavy
    multi-threaded compile (observed: zig's ReleaseFast LLVM build saturating
    all cores, then the *next* language's process landing on a ~1.6x-slower
    core) -- inflating that language's measured runtime by up to ~2x with
    zero difference in the actual compiled code's speed. Confirmed via
    /proc/<pid>/stat core sampling during a reproduction.

    Detected by reading each online CPU's cpuinfo_max_freq and keeping only
    the ones at the system's max (all cores tie on a homogeneous machine, so
    this is a no-op there -- returns every online core). Returns None if the
    frequency info isn't available at all (e.g. non-Linux), meaning "don't
    pin, leave default affinity"."""
    cpu_dir = Path("/sys/devices/system/cpu")
    freqs: dict[int, int] = {}
    for cpu_path in cpu_dir.glob("cpu[0-9]*"):
        try:
            cpu_id = int(cpu_path.name.removeprefix("cpu"))
            freq = int((cpu_path / "cpufreq" / "cpuinfo_max_freq").read_text().strip())
        except (OSError, ValueError):
            continue
        freqs[cpu_id] = freq
    if not freqs:
        return None
    fastest = max(freqs.values())
    return {cpu for cpu, f in freqs.items() if f == fastest}


FAST_CORES = detect_fast_cores() if hasattr(os, "sched_setaffinity") else None


def _pin_to_fast_cores():
    """preexec_fn for subprocess.run: restrict the child to FAST_CORES (a
    no-op affinity mask on a homogeneous-core machine). Applied uniformly to
    every compiler and every compiled benchmark binary so the comparison
    stays apples-to-apples -- not just a fix for whichever language happens
    to run right after zig."""
    os.sched_setaffinity(0, FAST_CORES)


def read_cpu_temp_c() -> Optional[float]:
    """Highest ACPI thermal-zone reading in Celsius, or None if unavailable.
    Used to pace the benchmark so sustained load (esp. zig's repeated LLVM
    ReleaseFast compiles) doesn't quietly throttle later cases/languages
    relative to earlier ones within the same run."""
    thermal_dir = Path("/sys/class/thermal")
    if not thermal_dir.is_dir():
        return None
    best = None
    for zone in thermal_dir.glob("thermal_zone*"):
        try:
            kind = (zone / "type").read_text().strip().lower()
            if "acpitz" not in kind and "x86_pkg_temp" not in kind and "cpu" not in kind:
                continue
            milli_c = int((zone / "temp").read_text().strip())
        except (OSError, ValueError):
            continue
        c = milli_c / 1000.0
        if best is None or c > best:
            best = c
    return best


def wait_for_cooldown(baseline_c: Optional[float], max_wait_s: float = 90.0, poll_interval_s: float = 3.0):
    """Block until CPU temp drops back near baseline_c (or max_wait_s elapses).
    No-op if no sensor was found at startup (baseline_c is None)."""
    if baseline_c is None:
        return
    temp = read_cpu_temp_c()
    if temp is None or temp <= baseline_c:
        return
    waited = 0.0
    while temp is not None and temp > baseline_c and waited < max_wait_s:
        time.sleep(poll_interval_s)
        waited += poll_interval_s
        temp = read_cpu_temp_c()
    status = f"{temp:.1f}°C" if temp is not None else "unknown"
    print(f"    (cooldown: waited {waited:.0f}s, now {status}, target <={baseline_c:.1f}°C)")


@dataclass
class RunResult:
    """Result of a single benchmark run."""
    compile_time_s: float
    runtime_s: float
    exit_code: int
    stdout: str
    stderr: str


@dataclass
class LangResults:
    """Aggregated results for one language on one problem."""
    language: str
    problem: str
    compile_times: list[float] = field(default_factory=list)
    run_times: list[float] = field(default_factory=list)
    binary_size_bytes: int = 0

    @property
    def mean_compile(self) -> float:
        return statistics.mean(self.compile_times) if self.compile_times else 0.0

    @property
    def mean_runtime(self) -> float:
        return statistics.mean(self.run_times) if self.run_times else 0.0

    @property
    def median_compile(self) -> float:
        return statistics.median(self.compile_times) if self.compile_times else 0.0

    @property
    def median_runtime(self) -> float:
        return statistics.median(self.run_times) if self.run_times else 0.0


def find_executable(name: str) -> Optional[Path]:
    """Find an executable in PATH."""
    return Path(p) if (p := shutil.which(name)) else None


# Nim compiles its generated C through gcc with these extra flags (verified via
# `nim c --listCmd`). Applied to the hand-written C and YAP's own gcc backend
# invocation too, so all three gcc-based toolchains use identical flags and any
# runtime difference reflects the generated code, not compiler-flag drift.
# (-w/-fmax-errors=3 are diagnostic-only for Nim's own generated C and are
# deliberately not mirrored here -- we want real warnings from our own sources.)
NIM_GCC_CFLAGS = ["-fno-strict-aliasing", "-fno-ident", "-fno-math-errno", "-pthread"]


def compile_c(src: Path, out: Path, opt: str = "-O3") -> tuple[float, str, str]:
    """Compile a C source file, return (elapsed_s, stdout, stderr)."""
    cc = find_executable("gcc") or find_executable("cc")
    if not cc:
        raise RuntimeError("No C compiler found (gcc/cc)")

    t0 = time.perf_counter()
    r = subprocess.run(
        [str(cc), opt, *NIM_GCC_CFLAGS, "-o", str(out), str(src)],
        capture_output=True, text=True,
        preexec_fn=_pin_to_fast_cores if FAST_CORES else None,
    )
    elapsed = time.perf_counter() - t0
    return elapsed, r.stdout, r.stderr


def compile_nim(src: Path, out: Path) -> tuple[float, str, str]:
    """Compile a Nim source file."""
    nim = find_executable("nim")
    if not nim:
        return 0.0, "", "nim compiler not found — skipping"

    t0 = time.perf_counter()
    r = subprocess.run(
        [str(nim), "c", "-d:release", "--opt:speed", "-o:" + str(out), str(src)],
        capture_output=True, text=True,
        cwd=src.parent,
        preexec_fn=_pin_to_fast_cores if FAST_CORES else None,
    )
    elapsed = time.perf_counter() - t0
    return elapsed, r.stdout, r.stderr


def compile_zig(src: Path, out: Path) -> tuple[float, str, str]:
    """Compile a Zig source file."""
    zig = find_executable("zig")
    if not zig:
        return 0.0, "", "zig compiler not found — skipping"

    t0 = time.perf_counter()
    r = subprocess.run(
        [str(zig), "build-exe", "-O", "ReleaseFast", "-femit-bin=" + str(out), str(src)],
        capture_output=True, text=True,
        preexec_fn=_pin_to_fast_cores if FAST_CORES else None,
    )
    elapsed = time.perf_counter() - t0
    return elapsed, r.stdout, r.stderr


def compile_yap(src: Path, out: Path) -> tuple[float, str, str]:
    """Compile a YAP source file using the yap compiler."""
    yap = find_executable("yap")
    if not yap:
        raise RuntimeError("yap not found (build the yap compiler first: make build)")

    t0 = time.perf_counter()
    r = subprocess.run(
        [str(yap), str(src), "-o", str(out), "-bO3"]
        + [f"-bf={flag}" for flag in NIM_GCC_CFLAGS],
        capture_output=True, text=True,
        env={**os.environ},
        preexec_fn=_pin_to_fast_cores if FAST_CORES else None,
    )
    elapsed = time.perf_counter() - t0
    return elapsed, r.stdout, r.stderr


COMPILERS = {
    "c":    compile_c,
    "nim":  compile_nim,
    "zig":  compile_zig,
    "yap":  compile_yap,
}


def run_binary(binary: Path, *args: str) -> tuple[float, int, str, str]:
    """Run a compiled binary and measure time."""
    t0 = time.perf_counter()
    r = subprocess.run(
        [str(binary), *args],
        capture_output=True, text=True,
        timeout=30,
        preexec_fn=_pin_to_fast_cores if FAST_CORES else None,
    )
    elapsed = time.perf_counter() - t0
    return elapsed, r.returncode, r.stdout, r.stderr


def run_one(
    lang: str, src: Path, case_input: str, work_dir: Path,
) -> RunResult:
    """Compile and run one case, returning timing results."""
    out = work_dir / f"fib_{lang}"
    compile_fn = COMPILERS[lang]

    ct, _, cerr = compile_fn(src, out)
    if cerr.strip():
        print(f"  [{lang}] compile stderr: {cerr.strip()[:200]}", file=sys.stderr)

    if not out.exists() or not os.access(out, os.X_OK):
        return RunResult(ct, 0.0, -1, "", f"binary not produced: {out}")

    rt, rc, stdout, stderr = run_binary(out, case_input)
    return RunResult(ct, rt, rc, stdout, stderr)


def load_cases(problem_dir: Path) -> list[tuple[str, str]]:
    """Load test cases from a CSV file (n,expected)."""
    csv_path = problem_dir / "cases" / "basic.csv"
    if not csv_path.exists():
        return []
    cases = []
    with open(csv_path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            cases.append((row["n"], row["expected"]))
    return cases


def load_timing_cases(problem_dir: Path) -> list[str] | None:
    """Load a problem's default timed-benchmark n values (cases/timing.csv,
    one column: n). Each problem's meaningful "n" is a different scale
    (recursion depth vs. board size vs. array length, ...), so this is kept
    per-problem instead of a single value shared across every problem."""
    csv_path = problem_dir / "cases" / "timing.csv"
    if not csv_path.exists():
        return None
    with open(csv_path) as f:
        reader = csv.DictReader(f)
        return [row["n"].strip() for row in reader if row["n"].strip()]


def _spawn_worker(worker_args: list[str]) -> dict:
    """Spawn `python3 run_bench.py <worker_args>` as a brand-new top-level
    process and parse its one-line JSON result from stdout.

    Why: a reproducible (if still unexplained) effect was found where
    running *any* other language's compiled binary earlier in the same
    Python process measurably slows down a subsequently-compiled yap
    binary's actual execution (confirmed via external `bash`/`date` timing
    that bypasses Python's own clock entirely, so it isn't a measurement
    artifact) -- e.g. ~30% slower for quicksort, ~2.2x slower for lcs.
    Ruled out: CPU core-type migration on this machine's hybrid Zen5/Zen5c
    cores (pinning to fast cores via FAST_CORES didn't fix it), thermal/
    power-budget throttling (a 30s idle wait didn't fix it), generic CPU
    load (busy-looping all cores didn't trigger it). Root cause unresolved.
    Isolating each language's entire compile+run sequence into its own
    freshly-spawned OS process (this function) eliminates the
    cross-contamination regardless of root cause, since the "victim"
    process then never shares ancestry with whatever "poisoned" it before."""
    r = subprocess.run(
        [sys.executable, str(Path(__file__).resolve())] + worker_args,
        capture_output=True, text=True,
    )
    try:
        return json.loads(r.stdout.strip())
    except json.JSONDecodeError:
        return {"error": f"worker crashed or produced invalid output: {r.stderr[:500]}"}


def _worker_verify(problem_dir: Path, lang: str) -> dict:
    """Runs inside an isolated worker subprocess (see _spawn_worker): compile
    once and check every basic.csv case for a single language."""
    compile_fn = COMPILERS[lang]
    src_dir = problem_dir / lang
    src = next(src_dir.glob(f"{problem_dir.name}.*"), None)
    if not src:
        return {"ok": False, "reason": "no source file found"}

    cases = load_cases(problem_dir)
    with tempfile.TemporaryDirectory() as td:
        out = Path(td) / f"verify_{lang}"
        ct, _, cerr = compile_fn(src, out)
        if not out.exists() or not os.access(out, os.X_OK):
            return {"ok": False, "reason": "compilation error"}

        for n, expected in cases:
            rt, rc, stdout, stderr = run_binary(out, n)
            actual = stdout.strip()
            if rc != 0 or actual != expected:
                return {
                    "ok": False,
                    "reason": (
                        f"{problem_dir.name}({n}) => '{actual}' "
                        f"(expected '{expected}', exit={rc})"
                    ),
                }
    return {"ok": True}


def verify_cases(
    problem_dir: Path, work_dir: Path, warmup_n: str = "30",
) -> dict[str, bool]:
    """Quick verification that each language produces correct output. Each
    language's compile+run happens in its own freshly-spawned worker
    subprocess -- see _spawn_worker's docstring for why."""
    results = {}
    cases = load_cases(problem_dir)
    if not cases:
        return results

    for lang in COMPILERS:
        src_dir = problem_dir / lang
        src = next(src_dir.glob(f"{problem_dir.name}.*"), None)
        if not src:
            print(f"  [{lang}] SKIP: no source file found in {src_dir}")
            results[lang] = False
            continue

        result = _spawn_worker(["--worker-verify", str(problem_dir), lang])
        if result.get("ok"):
            print(f"  [{lang}] PASS: all {len(cases)} cases correct")
        else:
            print(f"  [{lang}] FAIL: {result.get('reason', 'unknown error')}")
        results[lang] = bool(result.get("ok"))

    return results


def _worker_compile(
    problem_dir: Path, lang: str, warmup_iters: int, bench_iters: int,
) -> dict:
    """Compile-only worker: measure compile time and binary size once per language."""
    compile_fn = COMPILERS[lang]
    problem_name = problem_dir.name
    src_dir = problem_dir / lang
    src = next(src_dir.glob(f"{problem_name}.*"), None)
    if not src:
        return {"skip": "no source"}

    compile_times: list[float] = []
    binary_size_bytes = 0
    iter_failures: list[str] = []

    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        out = tmp / f"{problem_name}_{lang}"

        for _ in range(warmup_iters):
            compile_fn(src, out)

        for i in range(bench_iters):
            ct, _, cerr = compile_fn(src, out)
            if not out.exists() or not os.access(out, os.X_OK):
                iter_failures.append(f"iter {i}: compile FAILED ({cerr.strip()[:100]})")
                continue
            compile_times.append(ct)

        if out.exists():
            binary_size_bytes = out.stat().st_size

    return {
        "compile_times": compile_times,
        "binary_size_bytes": binary_size_bytes,
        "iter_failures": iter_failures,
    }


def _worker_benchmark(
    problem_dir: Path, lang: str, case_n: str, warmup_iters: int, bench_iters: int,
) -> dict:
    """Runs inside an isolated worker subprocess: compile once, then run
    warmup + timed iterations at a single case size.
    Compile timing is NOT measured here -- that's handled by _worker_compile."""
    compile_fn = COMPILERS[lang]
    problem_name = problem_dir.name
    src_dir = problem_dir / lang
    src = next(src_dir.glob(f"{problem_name}.*"), None)
    if not src:
        return {"skip": "no source"}

    run_times: list[float] = []
    iter_failures: list[str] = []

    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        out = tmp / f"{problem_name}_{lang}"

        # Compile once (not timed -- compile measurement is separate)
        _, _, cerr = compile_fn(src, out)
        if not out.exists() or not os.access(out, os.X_OK):
            return {
                "run_times": [],
                "binary_size_bytes": 0,
                "iter_failures": [f"compile FAILED ({cerr.strip()[:100]})"],
            }

        for _ in range(warmup_iters):
            run_binary(out, case_n)

        for i in range(bench_iters):
            rt, rc, stdout, _ = run_binary(out, case_n)
            if rc != 0:
                iter_failures.append(f"iter {i}: runtime FAILED (exit={rc})")
                continue
            run_times.append(rt)

    return {
        "run_times": run_times,
        "iter_failures": iter_failures,
    }


def benchmark_problem(
    problem_dir: Path, warmup_iters: int = 3, bench_iters: int = 10,
    case_ns: list[str] | None = None, cooldown_baseline_c: Optional[float] = None,
    cooldown_max_wait_s: float = 90.0,
) -> dict[str, list[LangResults]]:
    """Benchmark all languages for a single problem.
    Phase 1: compile each language once, measuring compile time and binary size.
    Phase 2: for each case size N, run the pre-compiled binary and measure runtime.
    Each language's work happens in its own freshly-spawned worker subprocess."""
    if case_ns is None:
        case_ns = ["40"]
    problem_name = problem_dir.name
    all_results: dict[str, list[LangResults]] = {}

    # --- Phase 1: Compile (once per language) ---
    print(f"\n{'='*60}")
    print(f"  Compile: {problem_name}")
    print(f"  Warmup iters: {warmup_iters}, Bench iters: {bench_iters}")
    print(f"{'='*60}")

    compile_results: dict[str, LangResults] = {}
    for lang, compile_fn in COMPILERS.items():
        src_dir = problem_dir / lang
        src = next(src_dir.glob(f"{problem_name}.*"), None)
        if not src:
            print(f"  [{lang}] SKIP: no source")
            continue
        if compile_fn is compile_nim and not find_executable("nim"):
            print(f"  [{lang}] SKIP: nim not installed")
            continue
        if compile_fn is compile_zig and not find_executable("zig"):
            print(f"  [{lang}] SKIP: zig not installed")
            continue

        wait_for_cooldown(cooldown_baseline_c, max_wait_s=cooldown_max_wait_s)
        print(f"\n  [{lang}] Compiling...")
        result = LangResults(lang, problem_name)
        worker_out = _spawn_worker([
            "--worker-compile", str(problem_dir), lang,
            str(warmup_iters), str(bench_iters),
        ])
        for msg in worker_out.get("iter_failures", []):
            print(f"    {msg}")
        if "error" in worker_out:
            print(f"    WORKER ERROR: {worker_out['error']}")
        result.compile_times = worker_out.get("compile_times", [])
        result.binary_size_bytes = worker_out.get("binary_size_bytes", 0)
        if result.compile_times:
            print(f"    compile: mean={result.mean_compile:.4f}s, "
                  f"median={result.median_compile:.4f}s")
            print(f"    binary:  {result.binary_size_bytes:,} bytes")
        else:
            print(f"    COMPILE FAILED")
        compile_results[lang] = result

    # --- Phase 2: Benchmark per case size ---
    for case_n in case_ns:
        print(f"\n{'='*60}")
        print(f"  Benchmark: {problem_name}  (n={case_n})")
        print(f"  Warmup iters: {warmup_iters}, Bench iters: {bench_iters}")
        print(f"{'='*60}")

        results: list[LangResults] = []
        for lang, compile_fn in COMPILERS.items():
            if lang not in compile_results:
                continue
            cr = compile_results[lang]
            if not cr.compile_times:
                # Compilation failed, skip runtime
                results.append(cr)
                continue

            wait_for_cooldown(cooldown_baseline_c, max_wait_s=cooldown_max_wait_s)
            print(f"\n  [{lang}] Running...")
            result = LangResults(lang, problem_name)
            result.compile_times = cr.compile_times
            result.binary_size_bytes = cr.binary_size_bytes

            worker_out = _spawn_worker([
                "--worker-benchmark", str(problem_dir), lang, case_n,
                str(warmup_iters), str(bench_iters),
            ])
            for msg in worker_out.get("iter_failures", []):
                print(f"    {msg}")
            if "error" in worker_out:
                print(f"    WORKER ERROR: {worker_out['error']}")
            result.run_times = worker_out.get("run_times", [])

            if result.run_times:
                print(f"    runtime: mean={result.mean_runtime:.4f}s, "
                      f"median={result.median_runtime:.4f}s")
            else:
                print(f"    RUNTIME FAILED")

            results.append(result)

        all_results[f"{problem_name}_n{case_n}"] = results

    return all_results


def write_csv(results: list[LangResults], path: Path):
    """Write results to CSV."""
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow([
            "language", "problem",
            "compile_mean_s", "compile_median_s",
            "runtime_mean_s", "runtime_median_s",
            "binary_size_bytes",
        ])
        for r in results:
            w.writerow([
                r.language, r.problem,
                f"{r.mean_compile:.6f}", f"{r.median_compile:.6f}",
                f"{r.mean_runtime:.6f}", f"{r.median_runtime:.6f}",
                r.binary_size_bytes,
            ])


def write_markdown(results_by_problem: dict[str, list[LangResults]], path: Path):
    """Write results as a markdown table."""
    lines = [
        "# Benchmark Results\n",
        f"*Generated by `{Path(__file__).name}`*\n",
    ]

    for problem, results in results_by_problem.items():
        lines.append(f"## {problem}\n")
        lines.append("| Language | Compile (mean) | Compile (median) | "
                      "Runtime (mean) | Runtime (median) | Binary Size |")
        lines.append("|----------|---------------|-----------------|"
                      "--------------|----------------|-------------|")
        for r in sorted(results, key=lambda x: x.mean_runtime):
            lines.append(
                f"| {r.language} "
                f"| {r.mean_compile:.4f}s "
                f"| {r.median_compile:.4f}s "
                f"| {r.mean_runtime:.4f}s "
                f"| {r.median_runtime:.4f}s "
                f"| {r.binary_size_bytes:,} B |"
            )
        lines.append("")

    with open(path, "w") as f:
        f.write("\n".join(lines))
    print(f"\nMarkdown report: {path}")


def write_plots(results_by_problem: dict[str, list[LangResults]], out_dir: Path):
    """Generate bar charts comparing compile time and runtime across languages."""
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not installed, skipping plots.", file=sys.stderr)
        return

    LANG_COLORS = {
        "c":   "#555555",
        "nim": "#f3d400",
        "zig": "#f7a41d",
        "yap": "#7b2d8b",
    }

    for problem, results in results_by_problem.items():
        langs = [r.language for r in results if r.compile_times]
        if not langs:
            continue

        fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(15, 5))
        fig.suptitle(f"{problem}", fontweight="bold")

        colors = [LANG_COLORS.get(l, "#999") for l in langs]
        x = range(len(langs))

        # Compile time [s]
        compile_means = [r.mean_compile for r in results if r.compile_times]
        bars = ax1.bar(x, compile_means, color=colors, edgecolor="black", linewidth=0.5)
        ax1.set_title("Compile time [s]")
        ax1.set_xticks(x)
        ax1.set_xticklabels(langs)
        ax1.bar_label(bars, fmt="%.3f", fontsize=8, padding=2)

        # Run time [s]
        run_means = [r.mean_runtime for r in results if r.run_times]
        bars2 = ax2.bar(x, run_means, color=colors, edgecolor="black", linewidth=0.5)
        ax2.set_title("Run time [s]")
        ax2.set_xticks(x)
        ax2.set_xticklabels(langs)
        ax2.bar_label(bars2, fmt="%.4f", fontsize=8, padding=2)

        # Binary size (KB)
        size_kb = [r.binary_size_bytes / 1024 for r in results if r.compile_times]
        bars3 = ax3.bar(x, size_kb, color=colors, edgecolor="black", linewidth=0.5)
        ax3.set_title("Binary [KB]")
        ax3.set_xticks(x)
        ax3.set_xticklabels(langs)
        ax3.bar_label(bars3, fmt="%.1f", fontsize=8, padding=2)

        plt.tight_layout()
        path = out_dir / f"{problem}_chart.png"
        fig.savefig(path, dpi=150)
        plt.close(fig)
        print(f"Plot saved: {path}")

    # Combined summary plot across all problems
    _write_summary_plot(results_by_problem, out_dir, LANG_COLORS)


def _write_summary_plot(
    results_by_problem: dict[str, list[LangResults]],
    out_dir: Path,
    colors: dict[str, str],
):
    """One summary figure per metric: grouped bars per problem with languages side-by-side."""
    import matplotlib.pyplot as plt

    all_langs = sorted(set(
        r.language
        for results in results_by_problem.values()
        for r in results
        if r.compile_times
    ))
    problems = list(results_by_problem.keys())

    n_problems = len(problems)
    if n_problems <= 1:
        return

    # Determine the bench_iters count from the first problem's metadata
    bench_iters = 0
    for results in results_by_problem.values():
        for r in results:
            if r.compile_times:
                bench_iters = len(r.compile_times)
                break
        if bench_iters:
            break

    n_langs = len(all_langs)
    width = 0.8 / n_langs

    # --- Three separate summary charts, one per metric ---
    metrics = [
        ("Compile time [s]",   lambda r: r.mean_compile,        "%.3f", "compile_time"),
        ("Run time [s]",       lambda r: r.mean_runtime,        "%.4f", "runtime"),
        ("Binary [KB]",        lambda r: r.binary_size_bytes / 1024, "%.0f", "binary_size"),
    ]

    for metric_title, getter, fmt, filename in metrics:
        # For binary size, use the first case of each problem (deduplicated)
        if filename == "binary_size":
            seen = {}
            dedup_problems = []
            for p in problems:
                base = p.split("_n")[0] if "_n" in p else p
                if base not in seen:
                    seen[base] = p
                    dedup_problems.append(p)
            xlabels = [p.split("_n")[0] if "_n" in p else p for p in dedup_problems]
            plot_problems = dedup_problems
        else:
            xlabels = [p.replace("_n", " (n=") + ")" for p in problems]
            plot_problems = problems

        n_plot = len(plot_problems)
        fig, ax = plt.subplots(figsize=(max(8, 3 * n_plot), 5))
        fig.suptitle(
            f"{metric_title}, mean of {bench_iters} attempts",
            fontweight="bold",
        )

        x = range(n_plot)

        ymax = 0
        for li, lang in enumerate(all_langs):
            offset = (li - (n_langs - 1) / 2) * width
            vals = []
            for problem in plot_problems:
                for r in results_by_problem[problem]:
                    if r.language == lang and r.compile_times:
                        vals.append(getter(r))
                        break
                else:
                    vals.append(0)
            bars = ax.bar(
                [xi + offset for xi in x], vals, width,
                label=lang, color=colors.get(lang, "#999"),
                edgecolor="black", linewidth=0.5,
            )
            ax.bar_label(bars, fmt=fmt, fontsize=6, padding=1, rotation=0)
            ymax = max(ymax, max(vals) if vals else 0)

        ax.set_xticks(x)
        ax.set_xticklabels(xlabels, fontsize=8)
        ax.set_ylim(0, ymax * 1.25)
        if n_langs > 1:
            ax.legend(fontsize=7)

        plt.tight_layout()
        path = out_dir / f"summary_{filename}.png"
        fig.savefig(path, dpi=150)
        plt.close(fig)
        print(f"Summary plot saved: {path}")


def write_summary(results_by_problem: dict[str, list[LangResults]], path: Path):
    """Write a plain-English summary of benchmark results."""
    lines = []
    lines.append("=" * 60)
    lines.append("  YAP Thesis — Chapter 11 Benchmark Summary")
    lines.append("=" * 60)
    lines.append("")

    for problem, results in results_by_problem.items():
        lines.append(f"Problem: {problem}")
        lines.append("-" * 40)
        sorted_results = sorted(
            [r for r in results if r.compile_times],
            key=lambda r: r.mean_runtime,
        )
        baseline = sorted_results[0] if sorted_results else None
        for r in sorted_results:
            c_med = f"{r.median_compile:.4f}s"
            r_med = f"{r.median_runtime:.4f}s"
            bin_sz = f"{r.binary_size_bytes:,}B"

            # Relative to fastest
            rel_str = ""
            if baseline and r.language != baseline.language and baseline.median_runtime > 0:
                ratio = r.median_runtime / baseline.median_runtime
                rel_str = f"  ({ratio:.1f}× vs {baseline.language})"

            lines.append(
                f"  {r.language:6s}  compile: {c_med:>10s}  "
                f"run: {r_med:>10s}{rel_str}  binary: {bin_sz}"
            )
        lines.append("")

    with open(path, "w") as f:
        f.write("\n".join(lines))
    print(f"Summary saved: {path}")


def main():
    # Worker dispatch: handled before argparse since it's an internal,
    # differently-shaped calling convention (see _spawn_worker's docstring)
    # -- each invocation does exactly one language's work and prints a
    # single JSON line to stdout, nothing else.
    argv = sys.argv[1:]
    if argv and argv[0] == "--worker-verify":
        print(json.dumps(_worker_verify(Path(argv[1]), argv[2])))
        return
    if argv and argv[0] == "--worker-compile":
        print(json.dumps(_worker_compile(
            Path(argv[1]), argv[2], int(argv[3]), int(argv[4]),
        )))
        return
    if argv and argv[0] == "--worker-benchmark":
        print(json.dumps(_worker_benchmark(
            Path(argv[1]), argv[2], argv[3], int(argv[4]), int(argv[5]),
        )))
        return

    parser = argparse.ArgumentParser(
        description="Run Chapter 11 benchmarks (compile + runtime timing)"
    )
    parser.add_argument(
        "problems", nargs="*",
        help="Problem directories to benchmark (default: all in benchmarks/)",
    )
    parser.add_argument(
        "--warmup", type=int, default=3,
        help="Warmup iterations (default: 3)",
    )
    parser.add_argument(
        "--iterations", type=int, default=10,
        help="Benchmark iterations (default: 10)",
    )
    parser.add_argument(
        "--case", type=str, default=None,
        help="Comma-separated n values for benchmark timing (e.g. '30,35,40,42,45'). "
             "Overrides every problem's cases/timing.csv when given; otherwise each "
             "problem uses its own timing.csv (falling back to '40' if it has none).",
    )
    parser.add_argument(
        "--verify", action="store_true",
        help="Verify correctness against test cases, then exit",
    )
    parser.add_argument(
        "--output-dir", type=Path, default=None,
        help="Output directory for results (default: benchmarks/results, or "
             "benchmarks/results/test-run under --test-run, so a smoke test "
             "never clobbers a real recorded run)",
    )
    parser.add_argument(
        "--plot", action="store_true",
        help="Generate bar-chart plots (requires matplotlib)",
    )
    parser.add_argument(
        "--no-cooldown", action="store_true",
        help="Disable the thermal cooldown pause between languages (default: on, "
             "waits for CPU temp to return near its reading at startup before each "
             "language's compile+run block, so e.g. zig's repeated LLVM ReleaseFast "
             "compiles don't leave the machine hotter/throttled for whichever "
             "language benchmarks next).",
    )
    parser.add_argument(
        "--cooldown-max-wait", type=float, default=90.0,
        help="Max seconds to wait per cooldown pause before giving up and "
             "proceeding anyway (default: 90)",
    )
    parser.add_argument(
        "--test-run", action="store_true",
        help="Smoke-test mode: only the last (largest) n from each problem's "
             "case list, one compile+run per language, no warmup. Fast sanity "
             "check that everything still compiles/runs correctly and that the "
             "relative language ordering looks reasonable, without waiting "
             "through the full multi-case/multi-iteration sweep.",
    )
    args = parser.parse_args()

    # Discover problems
    if args.problems:
        problem_dirs = [BENCH_DIR / p for p in args.problems]
    else:
        problem_dirs = sorted(
            d for d in BENCH_DIR.iterdir()
            if d.is_dir() and (d / "cases").is_dir()
        )

    if not problem_dirs:
        print("No benchmark problems found.", file=sys.stderr)
        sys.exit(1)

    if args.output_dir is not None:
        output_dir = args.output_dir
    elif args.test_run:
        output_dir = BENCH_DIR / "results" / "test-run"
    else:
        output_dir = BENCH_DIR / "results"
    output_dir.mkdir(parents=True, exist_ok=True)

    cooldown_baseline_c = None
    if not args.no_cooldown:
        cooldown_baseline_c = read_cpu_temp_c()
        if cooldown_baseline_c is not None:
            print(f"Thermal cooldown enabled: baseline {cooldown_baseline_c:.1f}°C "
                  f"(max wait {args.cooldown_max_wait:.0f}s/pause)")
        else:
            print("Thermal cooldown requested but no sensor found under /sys/class/thermal; skipping.")

    with tempfile.TemporaryDirectory() as td:
        work_dir = Path(td)

        # Verification pass
        print("Verifying correctness...")
        all_ok = True
        for pd in problem_dirs:
            print(f"\n  {pd.name}:")
            ok = verify_cases(pd, work_dir)
            if not all(ok.values()):
                all_ok = False

        if not all_ok:
            print("\nSome verifications FAILED. Fix before benchmarking.", file=sys.stderr)
            if args.verify:
                sys.exit(1)

        if args.verify:
            print("\nAll verifications passed.")
            return

        # Benchmark pass
        explicit_case_ns = (
            [n.strip() for n in args.case.split(",") if n.strip()]
            if args.case is not None else None
        )
        warmup_iters = args.warmup
        bench_iters = args.iterations
        if args.test_run:
            warmup_iters = 0
            bench_iters = 1
            print("\n*** --test-run: last case only, 1 iteration, no warmup ***")

        results_by_problem: dict[str, list[LangResults]] = {}
        for pd in problem_dirs:
            case_ns = explicit_case_ns or load_timing_cases(pd) or ["40"]
            if args.test_run:
                case_ns = case_ns[-1:]
            all_r = benchmark_problem(
                pd,
                warmup_iters=warmup_iters,
                bench_iters=bench_iters,
                case_ns=case_ns,
                cooldown_baseline_c=cooldown_baseline_c,
                cooldown_max_wait_s=args.cooldown_max_wait,
            )
            results_by_problem.update(all_r)

    # Write outputs
    all_results = [
        r for rlist in results_by_problem.values() for r in rlist
    ]
    csv_path = output_dir / "results.csv"
    md_path = output_dir / "results.md"
    summary_path = output_dir / "summary.txt"
    write_csv(all_results, csv_path)
    write_markdown(results_by_problem, md_path)
    write_summary(results_by_problem, summary_path)
    if args.plot:
        write_plots(results_by_problem, output_dir)
    print(f"\nResults written to {output_dir}/")


if __name__ == "__main__":
    main()
