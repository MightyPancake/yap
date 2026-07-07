{
  description = "YAP development shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              wasmtime
              gcc
              gnumake
              valgrind
              pkg-config
              git
              # TCC (Tiny C Compiler) for incremental compilation; provides tcc binary and libtcc1.a
              # On NixOS, tcc's libtcc1.a is at ${tinycc}/lib/tcc/libtcc1.a
              tinycc
              # libclang for C binding generation
              clang
              libclang
              # gum: renders the live test-status table in tests/run_tests.sh
              gum
              # python with matplotlib for benchmark plots
              (python3.withPackages (ps: [ ps.matplotlib ]))
            ];

            shellHook = ''
              export CLANG_VERSION=$(clang --version | head -n1)
              export CLANG_LIBDIR=${pkgs.libclang.lib}/lib
              export CLANG_INCDIR=${pkgs.libclang.dev}/include
              echo "============================================"
              echo "  libclang"
              echo "  Version : $CLANG_VERSION"
              echo "  Libs    : $CLANG_LIBDIR"
              echo "  Headers : $CLANG_INCDIR"
              echo "============================================"
            '';

          };
        }
      );
    };
}
