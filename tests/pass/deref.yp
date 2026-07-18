import io
import stdlib

struct Point {
    i32 x,
    i32 y,
}

// Self-referential (regression test): a struct that points to its own type
// needs a forward-declaration placeholder to resolve 'Link@ next' while
// 'Link' itself is still being built. Pointer types dedupe by name, so if
// the finished struct doesn't reuse that placeholder's type id in place,
// 'Link@' stays permanently bound to the empty placeholder and reading a
// field through it (even via the auto-deref below) segfaults.
struct Link {
    i32 value,
    Link@ next,
}

i32 fn main() {
    // Deref a pointer to a primitive
    i32 n = 7;
    i32@ np = n@;
    i32 v1 = np.;
    io->putchar(v1 + 48);
    io->putchar(10);

    // Deref a pointer to a struct (whole-value deref)
    Point p = [3, 9];
    Point@ pp = p@;
    Point v2 = pp.;
    io->putchar(v2.x + 48);
    io->putchar(44);
    io->putchar(v2.y + 48);
    io->putchar(10);

    // Assign through a deref (a deref is an lvalue)
    np. = 5;
    io->putchar(n + 48);
    io->putchar(10);

    // Member access auto-derefs through a pointer to struct/union -- no
    // '->' needed (that's already taken for module access, e.g. 'io->print'),
    // and no explicit deref required either. Both a read and a write here,
    // and on a self-referential struct specifically (see comment above).
    Point@ pp2 = p@;
    io->putchar(pp2.x + 48);
    io->putchar(10);

    none@ raw = stdlib->malloc(24);
    Link@ head = raw.(Link@);
    head.value = 1;
    head.next = null.(Link@);
    io->putchar(head.value + 48);
    io->putchar(10);
    stdlib->free(raw);

    // Explicit deref chained into a member access ('ptr..field') still
    // works too -- it's just no longer necessary.
    io->putchar(head..value + 48);
    io->putchar(10);

    ret 0;
}
