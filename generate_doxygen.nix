{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  name = "generate-doxygen-doc";
  buildInputs = [ pkgs.doxygen pkgs.graphviz ];

  src = ./include;

  buildPhase = ''
    # Create a basic Doxygen configuration file if not present
    if [ ! -f Doxyfile ]; then
      doxygen -g Doxyfile
    fi
    
    # Customize the Doxyfile for better output
    sed -i "s|^OUTPUT_DIRECTORY.*|OUTPUT_DIRECTORY = doxygen_output|" Doxyfile
    sed -i "s|^GENERATE_HTML.*NO|GENERATE_HTML = YES|" Doxyfile
    sed -i "s|^GENERATE_LATEX.*YES|GENERATE_LATEX = NO|" Doxyfile
    sed -i "s|^EXTRACT_ALL.*NO|EXTRACT_ALL = YES|" Doxyfile
    
    # Run Doxygen
    doxygen Doxyfile
  '';

  installPhase = ''
    mkdir -p $out
    cp -r doxygen_output $out/
  '';
}