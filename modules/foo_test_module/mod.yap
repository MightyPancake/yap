module foo_test_module {
    version: "0.0.1",
    prefix: "foo_test_module_"
}

i32 fn add_default(i32 a, i32 b = 10) {
    ret a + b;
}
