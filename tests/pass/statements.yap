module hello {}

i32 fn main() {
	//1. incremental_statement
	//TODO

	//2. var_decl
	_ a = 1;

	//3. expr_statement
	a;

	//4. if_statement
	if (a)
		a;

	//5. if_else_statement
	if (a)
		a;
	else
		a;

	//6. empty_statement
	;

	//7. while_loop (terminating)
	i32 w = 3;
	while (w)
		w = w - 1;

	//8. for_loop (terminating)
	i32 s = 0;
	for (i32 i = 0; i < 3; i = i + 1)
		s = s + 1;

	//10. break_statement
	while (1) {
		a;
		break;
	}

	//11. continue_statement
	i32 c = 0;
	while (c < 3) {
		c = c + 1;
		if (c) continue;
		a;
	}

	//12. block_expr
	i32 blk = ({
		i32 res = 1;
		res;
	});

	//9. return_statement (success == 0)
	ret blk - 1;
}
