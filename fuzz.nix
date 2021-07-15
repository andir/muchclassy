{ pkgs ? import <nixpkgs> {} }:
let
  drv = (pkgs.callPackage ./default.nix {}).overrideAttrs (_: {
    preConfigure = ''
      export CC=${pkgs.afl}/bin/afl-gcc
      export CXX=${pkgs.afl}/bin/afl-g++
      export CFLAGS="-O3 -funroll-loops"
    '';

    makeFlags = ["fuzz"];
    installPhase = ''
      mkdir -p $out/bin
      cp fuzz $out/bin/
    '';
  });

  to_files = pkgs.writeScriptBin "to_files.py" ''
    #!${pkgs.python3}/bin/python3
    import fileinput

    for (n,line) in enumerate(fileinput.input()):
      with open(f'inputs/corpus/{n}.txt', 'w+') as fh:
        fh.write(line)
  '';
in rec {
  inherit drv;
  generate-corpus = pkgs.writeShellScript "generate-corpus"  ''
    export PATH="${pkgs.buildEnv { name = "generate-corpus-env"; paths = [ pkgs.notmuch pkgs.mblaze pkgs.coreutils pkgs.findutils pkgs.gawk to_files ]; }}/bin"
#    set -o pipefail
    set -ex
    mkdir -p inputs
    test -e inputs/corpus || {
      mkdir inputs/corpus
      notmuch search --format=text0 --output=files 'tag:/^list::/' | xargs -0 mhdr -h List-id  2>/dev/null > inputs/corpus.tmp
      sort < inputs/corpus.tmp | uniq | to_files.py
    }
    test -e inputs/dict || {
      mkdir inputs/dict
      ${pkgs.lib.concatStringsSep "\n" (
        pkgs.lib.imap0 (n: word: "echo -e '${word}' > inputs/dict/${toString n}.txt") [
          " "
          "\t"
          "<"
          ">"
          "@"
          "."
        ]
      )}
    }
  '';
  runner = pkgs.writeShellScript "run.sh" ''
    set -ex
    ${generate-corpus}
    exec ${pkgs.afl}/bin/afl-fuzz -i inputs/corpus -x inputs/dict -o findings -m 300 ${drv}/bin/fuzz "$@"
  '';
}
