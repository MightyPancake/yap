i32 fn test() {
	//1. incremental_statement
	//TODO

	//2. var_decl
	_ a = 1;

	//3. expr_statement
	a;
	
	//4. if_statement
	if a
		a;

	//5. if_else_statement
	if a
		a;
	else
		a;
	
	//6. empty_statement
	;

	//7. while_loop
	while a
		a;
	
	//8. for_loop
	for _ a = 0;, a, a+1
		a;

	//9. return_statement
	ret a;

	//10. break_statement
	while 1 {
		a;
		break
	}

	//11. continue_statement
	while 1 {
		if a continue;
		a+1;
	}
}
