{
  description = "Flake for my project using gcc14 and C++23";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        match_version = "0.0.1";
        pkgs = import nixpkgs { inherit system; };

        # Read all files in the examples folder and filter for .cpp files.
        exampleFiles = builtins.filter (f: builtins.match ".*\\.cpp$" f != null) (
          builtins.attrNames (builtins.readDir ./examples)
        );

        # Create a derivation for each example file.
        examplesDerivations = builtins.listToAttrs (
          map (
            file:
            let
              # Strip the ".cpp" extension.
              name = builtins.substring 0 (builtins.stringLength file - 4) file;
            in
            {
              name = name;
              value = pkgs.stdenv.mkDerivation {
                pname = name;
                version = match_version;
                # Make the entire repository available so that both examples/ and include/ are visible.
                src = ./.;
                buildInputs = [ pkgs.gcc14 ];
                configurePhase = "";
                buildPhase = ''
                  g++ -std=c++23 -O3 examples/${file} -Iinclude -o ${name}
                '';
                installPhase = ''
                  mkdir -p $out/bin
                  cp ${name} $out/bin/
                '';
              };
            }
          ) exampleFiles
        );
      in
      {
        # Development shell available on all systems.
        devShells.default = pkgs.mkShell {
          buildInputs = [ pkgs.gcc14 ];
        };

        # Define packages: your tests plus all the examples.
        packages = pkgs.lib.recursiveUpdate {
          tests = pkgs.stdenv.mkDerivation {
            pname = "cppmatch_tests";
            version = match_version;
            src = ./.;
            buildInputs = [ pkgs.gcc14 ];
            configurePhase = "";
            buildPhase = "g++ -std=c++23 test/main.cpp -Iinclude -o cppmatch_tests";
            installPhase = ''
              mkdir -p $out/bin
              cp cppmatch_tests $out/bin/
            '';
          };
        } examplesDerivations;

      }
    );
}
