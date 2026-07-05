#!/usr/bin/env bash
# Runs tests/pass/*.yap and tests/fail/*/ in parallel under valgrind, with a
# live gum-rendered status table. Invoked by `make test`.
set -uo pipefail

CYAN=$'\e[96m'; PURPLE=$'\e[94m'; GREEN=$'\e[92m'; RED=$'\e[91m'; RESET=$'\e[0m'

SHOW_TEST_OUTPUT="${SHOW_TEST_OUTPUT:-false}"
DETAILED="${DETAILED:-false}"
TEST_JOBS="${TEST_JOBS:-$(nproc)}"

work_dir=$(mktemp -d)
cleanup() {
    [ -t 1 ] && tput cnorm 2>/dev/null
    rm -rf "$work_dir"
}
trap cleanup EXIT
trap 'exit 130' INT TERM

# Each entry: test_file|expect|err_file
entries=()
for f in tests/pass/*.yap; do
    [ -e "$f" ] || { echo "No pass test files found in ./tests/pass"; exit 1; }
    entries+=("$f|pass|")
done
for d in tests/fail/*/; do
    [ -d "$d" ] || continue
    entries+=("${d}test.yap|fail|${d}err.txt")
done

names=()
for i in "${!entries[@]}"; do
    IFS='|' read -r test_file _ _ <<< "${entries[$i]}"
    names[i]="$test_file"
    echo pending > "$work_dir/status.$i"
done

run_one() {
    local i="$1" test_file="$2" expect="$3" err_file="$4"
    local out_bin="$work_dir/bin.$i"
    local vg_log="$work_dir/vg.$i.log"
    local cmd_log="$work_dir/cmd.$i.log"

    echo running > "$work_dir/status.$i"

    local run_exit=0
    valgrind --leak-check=full --error-exitcode=99 \
        --suppressions=valgrind_suppressions.supp \
        --log-file="$vg_log" \
        ./yap "$test_file" -o "$out_bin" >"$cmd_log" 2>&1 || run_exit=1

    local test_failed=0
    if [ "$expect" = "pass" ]; then
        if [ "$run_exit" -ne 0 ]; then
            test_failed=1
        elif [ -x "$out_bin" ]; then
            # Runtime gate: the compiled program must exit 0. Convention is
            # "success = main returns 0", so tests fold their assertions into the
            # exit code (e.g. `ret computed - expected;`). Not run under valgrind
            # ; we only leak-check the compiler, not the user program.
            local prog_exit=0
            timeout 10 "$out_bin" >"$work_dir/run.$i.log" 2>&1 || prog_exit=$?
            if [ "$prog_exit" -ne 0 ]; then
                test_failed=1
                if [ "$prog_exit" -eq 124 ]; then
                    echo "runtime TIMEOUT (>10s) ; program did not terminate" >> "$cmd_log"
                else
                    echo "runtime exit $prog_exit (expected 0)" >> "$cmd_log"
                fi
            fi
        fi
    else
        if [ "$run_exit" -ne 0 ] && grep -qF -- "$(cat "$err_file")" "$cmd_log"; then
            test_failed=0
        else
            test_failed=1
        fi
    fi

    local leak_found=0
    if grep -Eq 'definitely lost:[[:space:]]*[1-9][0-9]* bytes|indirectly lost:[[:space:]]*[1-9][0-9]* bytes|possibly lost:[[:space:]]*[1-9][0-9]* bytes' "$vg_log"; then
        leak_found=1
    fi

    if [ "$test_failed" -eq 1 ]; then
        echo fail > "$work_dir/status.$i"
    else
        echo pass > "$work_dir/status.$i"
    fi
    [ "$leak_found" -eq 1 ] && touch "$work_dir/leak.$i"
    { [ "$test_failed" -eq 1 ] || [ "$leak_found" -eq 1 ]; } && touch "$work_dir/bad.$i"
}

launcher() {
    for i in "${!entries[@]}"; do
        IFS='|' read -r test_file expect err_file <<< "${entries[$i]}"
        while [ "$(jobs -rp | wc -l)" -ge "$TEST_JOBS" ]; do
            wait -n
        done
        run_one "$i" "$test_file" "$expect" "$err_file" &
    done
    wait
}

render() {
    local rows="" i st color word label
    for i in "${!entries[@]}"; do
        st=$(cat "$work_dir/status.$i" 2>/dev/null || echo pending)
        case "$st" in
            pending) color="$CYAN";   word="pending" ;;
            running) color="$PURPLE"; word="running" ;;
            pass)    color="$GREEN";  word="passed" ;;
            fail)    color="$RED";    word="failed" ;;
            *)       color="$RESET";  word="$st" ;;
        esac
        label="${color}$(printf '%-7s' "$word")${RESET}"
        [ -e "$work_dir/leak.$i" ] && label="${label} ${RED}LEAK!${RESET}"
        rows+="${names[i]},${label}"$'\n'
    done
    # Render the full frame before touching the screen, to avoid a blank flash
    local frame
    frame=$(printf '%s' "$rows" | gum table -s',' -c 'Test,Status' -p)
    printf '\033[H\033[J%s\n' "$frame"
}

# Non-interactive (CI): plain line per finished test, no gum/cursor tricks
declare -A reported
report_progress() {
    local i st label
    for i in "${!entries[@]}"; do
        [ -n "${reported[$i]:-}" ] && continue
        st=$(cat "$work_dir/status.$i" 2>/dev/null || echo pending)
        case "$st" in
            pass) label="${GREEN}passed${RESET}" ;;
            fail) label="${RED}failed${RESET}" ;;
            *) continue ;;
        esac
        [ -e "$work_dir/leak.$i" ] && label="$label ${RED}LEAK!${RESET}"
        echo "${names[i]}: $label"
        reported[$i]=1
    done
}

launcher &
launcher_pid=$!

if [ -t 1 ]; then
    clear
    tput civis 2>/dev/null || true
    while kill -0 "$launcher_pid" 2>/dev/null; do
        render
        sleep 0.15
    done
    wait "$launcher_pid"
    render
    tput cnorm 2>/dev/null || true
else
    while kill -0 "$launcher_pid" 2>/dev/null; do
        report_progress
        sleep 1
    done
    wait "$launcher_pid"
    report_progress
fi

echo
failed_any=0
pass_count=0
for i in "${!entries[@]}"; do
    bad=0
    [ -e "$work_dir/bad.$i" ] && bad=1
    if [ "$SHOW_TEST_OUTPUT" = "true" ]; then
        cat "$work_dir/cmd.$i.log"
    fi
    if [ "$bad" -eq 1 ] || [ "$DETAILED" = "true" ]; then
        echo "${CYAN}Output for ${names[i]}${RESET}"
        cat "$work_dir/cmd.$i.log"
        echo "${CYAN}Valgrind output for ${names[i]}${RESET}"
        cat "$work_dir/vg.$i.log"
    fi
    if [ "$bad" -eq 1 ]; then
        failed_any=1
    else
        pass_count=$((pass_count + 1))
    fi
done

total=${#entries[@]}
if [ "$failed_any" -eq 0 ]; then
    echo "${GREEN}Tests passed! ($pass_count/$total)${RESET}"
else
    echo "${RED}Tests failed! ($pass_count/$total passed)${RESET}"
fi

exit "$failed_any"
