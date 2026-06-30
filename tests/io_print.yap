import io

i32 fn main() {
    i32 age = 7;
    io->print:(c"Hello, world!\n");
    io->print:(c"age = %d, tag = %c, name = %s, literal %% sign\n", [age, 33, c"Yap"]);
    ret 0;
}
