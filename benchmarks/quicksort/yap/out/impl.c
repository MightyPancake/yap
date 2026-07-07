#line 0 "yap_c_output.c"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "types.h"
#include "prototypes.h"

void io_print_char(int c){
io_putchar(c);
}
void io_print_str(char * s){
int i = 0;
while (s[i] != 0)
{
io_print_char(((int)(s[i])));
i = i + 1;
}
}
void io_print_i32(int v){
if (v < 0)
{
io_print_char(45);
v = 0 - v;
}
if (v == 0)
{
io_print_char(48);
return;
}
int digits[12];
int n = 0;
while (v > 0)
{
digits[n] = (v % 10) + 48;
v = v / 10;
n = n + 1;
}
int i = n - 1;
while (i >= 0)
{
io_print_char(digits[i]);
i = i - 1;
}
}
void* io_print(char * fmt, yExprList * args){
void* stmts = yapi_stmt_list_new();
int ai = 0;
int i = 0;
while (fmt[i] != 0)
{
int ch = ((int)(fmt[i]));
if (ch == 37)
{
int spec = ((int)(fmt[i + 1]));
if (spec == 100)
{
void* call_expr = yapi_call1(yapi_var_value("io_print_i32"), (*(args)).data[ai]);
stmts = yapi_stmt_list_push(stmts, yapi_expr_stmt(call_expr));
ai = ai + 1;
i = i + 2;
}
else
if (spec == 115)
{
void* call_expr = yapi_call1(yapi_var_value("io_print_str"), (*(args)).data[ai]);
stmts = yapi_stmt_list_push(stmts, yapi_expr_stmt(call_expr));
ai = ai + 1;
i = i + 2;
}
else
if (spec == 99)
{
void* call_expr = yapi_call1(yapi_var_value("io_print_char"), (*(args)).data[ai]);
stmts = yapi_stmt_list_push(stmts, yapi_expr_stmt(call_expr));
ai = ai + 1;
i = i + 2;
}
else
{
void* call_expr = yapi_call1(yapi_var_value("io_print_char"), yapi_int(spec));
stmts = yapi_stmt_list_push(stmts, yapi_expr_stmt(call_expr));
i = i + 2;
}
}
else
{
void* call_expr = yapi_call1(yapi_var_value("io_print_char"), yapi_int(ch));
stmts = yapi_stmt_list_push(stmts, yapi_expr_stmt(call_expr));
i = i + 1;
}
}
return yapi_block(stmts);
}
void print_u64(unsigned long v){
if (v == 0)
{
io_print_char(48);
return;
}
int digits[24];
int n = 0;
while (v > 0)
{
unsigned long d = v % 10;
digits[n] = ((int)(d)) + 48;
v = v / 10;
n = n + 1;
}
int i = n - 1;
while (i >= 0)
{
io_print_char(digits[i]);
i = i - 1;
}
}
void swap_elem(int * arr, int i, int j){
int tmp = arr[i];
arr[i] = arr[j];
arr[j] = tmp;
return;
}
int partition(int * arr, int lo, int hi){
int pivot = arr[hi];
int i = lo - 1;
int j = lo;
while (j < hi)
{
if (arr[j] <= pivot)
{
i = i + 1;
swap_elem(arr, i, j);
}
j = j + 1;
}
swap_elem(arr, i + 1, hi);
return i + 1;
}
void quicksort(int * arr, int lo, int hi){
if (lo < hi)
{
int p = partition(arr, lo, hi);
quicksort(arr, lo, p - 1);
quicksort(arr, p + 1, hi);
}
return;
}
void fill(int * arr, int n){
int i = 0;
while (i < n)
{
unsigned long idx = ((unsigned long)((i + 1)));
arr[i] = ((int)(((idx * 2654435761) % 1000003)));
i = i + 1;
}
return;
}
unsigned long checksum(int * arr, int n){
unsigned long sum = 0;
int i = 0;
while (i < n)
{
sum = sum + ((unsigned long)(arr[i])) * ((unsigned long)((i + 1)));
i = i + 1;
}
return sum;
}
int main(int argc, char** argv){
struct { char ** data; unsigned long len; } args = { .data = argv, .len = (unsigned long)argc };
char * n_str = args.data[1];
int n = 0;
int k = 0;
while (n_str[k] != 0)
{
n = n * 10 + ((int)(n_str[k])) - 48;
k = k + 1;
}
if (n <= 0)
{
print_u64(0);
io_putchar(10);
return 0;
}
void * raw = stdlib_malloc((((long)(n))) * 4);
int * arr = ((int *)(raw));
fill(arr, n);
quicksort(arr, 0, n - 1);
unsigned long result = checksum(arr, n);
stdlib_free(raw);
print_u64(result);
io_putchar(10);
return 0;
}
