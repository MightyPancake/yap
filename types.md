# Context
- Sources (array of sources)
- Objects (array of IR objects/modules)
- Global settings
- Parse func (FE)
- Comp func (BE)

## Input
Files/strings passed to the compiler as input, ie. main.yap

## Source
Information about source (either file or string) that were passed to the compiler.
- kind (string/file)
- trace stack
- content (path or code string)

## Object
A single result unit of compilation. A single module.
This is the result of parsing a file.

- Declarations (array of module or top-level declarations)

## Global settings
Parsed from command line. These include:
- (-o, --output <names>) Comma separated list of of output file(s).
- (-oX) Optimization levels passed to the compiler.
- (-m, --modules-path) Printing the modules path.
- (-c, --c-flags) Printing C flags to compile modules with (needed for shared yap libs) (-c)
- (-r --run <args>) Run the resulting exec with <args>.
- (-f, --front <module>) Module used for the front end of the compiler. (default: yap-ts)
- (-b, --back <module>) Module used for the back end of the compiler. (default: yap-c)

yap-c exclusive
- (--output-c) Keep the resulting C file (yap-c backend only)
- (-C, --c-compiler) C compiler used on the intermediate C files. 

## Parse function
Front-end parsing function. This is loaded from the modules and has access to context.
It should fill in the following fields only:
- Objects

## Comp function
Back-end parsing function. This is lodaded from modules and has access to the context.
It should not feel-in any fields, just emit warnings and results (output files).


