# Pipeline

## Goals
Macros are written in YAP itself. Macro invocations are allowed in further macros.

## Phases
1. [X] Parsing phase (follows imports) (yap-ts)
2. [ ] Internal information retrieval phase (yap-c, yap-semantic, libyap) (This registers most types, macros and modules)
    2.1. Macro engine initiation
    2.2. Macro compilation
       a. Semantic analysis
       b. Inside macro expansion
       c. Compilation of the main macro
       d. Registering result in macro engine
3. [ ] Macro expansion phase (yap-c)
4. [ ] Final semantic analysis phase (yap-semantic)
5. [ ] Codegen phase (yap-c)