fn add(i32:a, i32:b) -> i32 {
	1+2
}

fn foo() {}
fn bar() {}

fn some_func_using_funcs(
	(fn):a,
	(fn):a2,
	(i32 fn):b,
	(i32@ fn i32@):c,
	i32:num,
	f32:f
) -> i32 {
	d := 123
	d = 12
	// f2 := f + 1
	res := add(1, 2)
	//res(1)
	//add(1,2,3)
	//add(f, 1)
	a()
	ret add(6, 7)
	ret;
	if a foo()
	else bar()
	//add := (i32 fn i32 a, i32 b) {}
}
