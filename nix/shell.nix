# Run like this: `nix-shell --pure nix.shell`

{ nixpkgs ? fetchTarball "https://github.com/NixOS/nixpkgs/archive/nixos-unstable.tar.gz" }:

let
  pkgs = import nixpkgs { };

  cross = import nixpkgs {
    crossSystem = {
      config = "riscv32-none-elf";
      libc   = "newlib-nano";
      gcc = {
        arch = "rv32ima_zicsr_zifencei";  # use "rv32ima" if you never touch CSRs / fence.i
        abi  = "ilp32";
      };
    };
  };
in

pkgs.mkShell {
  # Build tools executing on the host machine
  nativeBuildInputs = [
    # RISC-V cross-compiler toolchain (binaries prefixed with riscv32-none-elf-)
    cross.stdenv.cc            # riscv32-none-elf-gcc / g++
    cross.stdenv.cc.bintools   # riscv32-none-elf-as, -ld, -objdump, ...

    # Host build tools (native gcc / binutils are provided automatically by pkgs.stdenv)
    pkgs.pkg-config
    pkgs.gnumake
    pkgs.cmake
    pkgs.emscripten

    # Optional (built from source, slow):
    # cross.buildPackages.gdb  # riscv32-none-elf-gdb
  ];

  # Host target libraries and headers (CMake & pkg-config discover these automatically)
  buildInputs = [
    pkgs.libwebsockets
  ];

  shellHook = ''
    echo "=== Development Environment Ready ==="
    echo "  Native GCC: $(gcc --version | head -n1)"
    echo "  RISC-V GCC: $(riscv32-none-elf-gcc --version | head -n1)"
    echo "  Emscripten: $(emcc --version | head -n1)"
    echo "======================================"
    export RISCV_CC=${cross.stdenv.cc}/bin/riscv32-none-elf-gcc
    export NATIVE_CC=${pkgs.stdenv.cc}/bin/cc
    export NATIVE_CXX=${pkgs.stdenv.cc}/bin/c++
  '';
}
