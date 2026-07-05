// Higher-order methods on arr(T) (map/filter/fold), driven by function
// literals. The callback params are declared with exact function types
// (yapi->func_typeN in modules/arr/arr.yap), so passing a wrongly-typed
// function is a compile-time "Argument type mismatch" error.
import io
import arr

i32 fn main() {
    arr->arr:(i32) nums;
    nums:init();
    nums:push(1);
    nums:push(2);
    nums:push(3);
    nums:push(4);

    // map: [1,2,3,4] -> [2,4,6,8]
    _ doubled = nums:map((i32 fn i32 x) { ret x * 2; });
    if (doubled:len() == 4) io->print:(c"map len OK\n");
    _ dsum = doubled:at(0) + doubled:at(1) + doubled:at(2) + doubled:at(3); // 20
    if (dsum == 20) io->print:(c"map values OK\n");

    // filter: [1,2,3,4] -> [2,4]
    _ evens = nums:filter((bool fn i32 x) { ret (x % 2) == 0; });
    if (evens:len() == 2) io->print:(c"filter len OK\n");
    _ esum = evens:at(0) + evens:at(1); // 6
    if (esum == 6) io->print:(c"filter values OK\n");

    // fold: 0+1+2+3+4 = 10
    _ total = nums:fold((i32 fn i32 acc, i32 x) { ret acc + x; }, 0);
    if (total == 10) io->print:(c"fold OK\n");

    // for: side-effecting iteration with index+value, in order -- doesn't
    // build a new arr(T), so just confirm nums itself is unaffected after.
    nums:for((none fn u32 i, i32 v) { io->print:(c"%u: %d\n", [i, v]); });
    if (nums:len() == 4) io->print:(c"for len OK\n");

    nums:free();
    doubled:free();
    evens:free();

    ret dsum + esum + total - 36; // 20 + 6 + 10 - 36 = 0
}
