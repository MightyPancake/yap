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
          # lib = pkgs.lib;
          # tcc = pkgs.tinycc;
          # tccDev = lib.getDev tcc;
          # tccLib = lib.getLib tcc;
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              gcc
              gnumake
              valgrind
              pkg-config
              git
            ];

            # buildInputs = [ tcc ];

            # shellHook = ''
            #   export CPATH="${tccDev}/include''${CPATH:+:$CPATH}"
            #   export LIBRARY_PATH="${tccLib}/lib''${LIBRARY_PATH:+:$LIBRARY_PATH}"
            #   export LD_LIBRARY_PATH="${tccLib}/lib''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
            # '';
          };
        }
      );
    };
}
