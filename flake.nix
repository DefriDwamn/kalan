{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
  };

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        packages = with pkgs; [
          pkgs.libGL

          # X11 dependencies
          pkgs.xorg.libX11
          pkgs.xorg.libX11.dev
          pkgs.xorg.libXcursor
          pkgs.xorg.libXi
          pkgs.xorg.libXinerama
          pkgs.xorg.libXrandr

          # Build with web
          pkgs.emscripten

          pkgs.bear
          pkgs.cmake
          pkgs.zlib

          clang_19
        ];
      };
    };
}
