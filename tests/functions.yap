i32 fn add(i32 a, i32 b) {
	1+2;
}

none fn foo() {}
none fn bar() {}

fn main(){}

i32 fn some_func_using_funcs(
	(fn) a,
	(fn) a2,
	(i32 fn) b,
	(i32@ fn i32@) c,
	i32 num,
	f32 f
) {
	_ d = 123;
	d = 12;
	// f2 = f + 1
	_ res = add(1, 2);
	//res(1)
	//add(1,2,3)
	//add(f, 1)
	a();
	if a foo();
	else bar();
	//_ add = (i32 fn i32 a, i32 b) {}
	ret 0;
}
