# Pipeline

## Goals
Incremental compilation allows YAP code to be compiled and evaluated at compile time. Incremental invocations are allowed in further incremental blocks.

## Phases
1. [X] Parsing phase (follows imports) (yap-ts)
2. [ ] Internal information retrieval phase (yap-c, yap-semantic, libyap) (This registers most types, incremental blocks and modules)
    2.1. Incremental engine initiation
    2.2. Incremental compilation
       a. Semantic analysis
       b. Inside incremental expansion
       c. Compilation of the main incremental block
       d. Registering result in incremental engine
3. [ ] Incremental expansion phase (yap-c)
4. [ ] Final semantic analysis phase (yap-semantic)
5. [ ] Codegen phase (yap-c)