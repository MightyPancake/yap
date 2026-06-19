module io {
    name: "io",
    version: "0.0.1",
    prefix: "yap_io"
}

//This tells yap what to expect from the lib
yap_header#(
    ast{
        int fn puts(byte@ const str);
    }
)

//This handles all that is needed
//You can also use the lib param to load a certain library
c_lib#(
    .header="<stdio.h>",
)

//Ideally, c_lib would automatically generate yap glue code out of the header, but that's a nice thing to have in the future, so...
//TODO