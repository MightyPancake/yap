module foo_test_module {
    version: "0.0.1",
    prefix: "foo_test_module_"
}

i32 fn add_default(i32 a, i32 b = 10) {
    ret a + b;
}

// Calls a sibling function in this same module by its bare, unprefixed name
// (as opposed to the 'module->func()' access path used from outside), to
// exercise module-local name mangling for identifier references.
i32 fn add_default_via_sibling(i32 a) {
    ret add_default(.a=a, .b=2);
}
