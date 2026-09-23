// Scratch C function for verifying yapi->call_args_new/push + yapi->call can
// build a call with more than 3 plain args to an externally-bound function.
int arity_test_sum5(int a, int b, int c, int d, int e){
    return a + b + c + d + e;
}
