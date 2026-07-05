// Helper for func_literals.yap: this file mints its own function literal, so
// together with the importer it proves anon names are unique across source
// files (ctx-global counter ; per-source counters made both files emit
// __anon_func_0 into the same translation unit).
i32 fn seven_via_literal() {
    _ mk = (i32 fn) { ret 7; };
    ret mk();
}
