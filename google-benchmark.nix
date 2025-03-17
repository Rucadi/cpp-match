{ lib
, gcc14Stdenv
, fetchFromGitHub
, cmake
}:

gcc14Stdenv.mkDerivation rec {
  pname = "google-benchmark";
  version = "1.8.3";

  src = fetchFromGitHub {
    owner = "google";
    repo = "benchmark";
    rev = "v${version}";
    sha256 = "sha256-gztnxui9Fe/FTieMjdvfJjWHjkImtlsHn6fM1FruyME=";
  };

  nativeBuildInputs = [ cmake ];
  buildInputs = [  ];

  cmakeFlags = [
    "-DBENCHMARK_ENABLE_GTEST_TESTS=OFF"
    "-DBENCHMARK_ENABLE_TESTING=OFF"
  ];

  meta = with lib; {
    description = "A microbenchmark support library";
    homepage = "https://github.com/google/benchmark";
    license = licenses.asl20;
    platforms = platforms.all;
    maintainers = with maintainers; [ ];
  };
} 