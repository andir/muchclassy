{ stdenv, lib, notmuch, gmime3, pkgconfig }:
stdenv.mkDerivation {
  pname = "muchclassy";
  version = "0.0";

  src = lib.cleanSource ./.;

  buildInputs = [ notmuch gmime3 pkgconfig ];

  installPhase = ''
    mkdir -p $out/bin
    cp muchclassy $out/bin
  '';
}
