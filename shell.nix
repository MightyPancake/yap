{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
	packages = with pkgs; [
		gcc
		gnumake
		valgrind
		pkg-config
		git
	];
}
