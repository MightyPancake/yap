# yap - The tool

The compiler for language itself and the tools are accessible from this command.
Compiling the code, getting packages, utilities and other yap related tools.

This doesn't mean there's only one way to get thing done in yap tho; this is only the interface.

Let's take a look at the package manager. Let's say a package is wanted, but it hasn't been uploaded using the package manager in yap, but is shared in a different way.
One can easily just create their own package managers and boot it to yap.

All yap does is launch utilities that are meant to have unified argument syntax.

This means each yap module will have it's own arguments and if you want a different implementation you can just swap it for anything that follows the guidelines.

## What's inside this directory then?

The yap program is used to launch yap related modules.
Each module should have at least 1 category. For example, yap ships with the `front/ts` module which is used to parse yap code using tree sitter.

Every directory here represents a module category.
The `shared` directory is used to 
