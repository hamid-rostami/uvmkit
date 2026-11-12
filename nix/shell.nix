# Bare-metal RISC-V rv32ima toolchain: GCC + binutils + newlib-nano
# Works with plain `nix-shell`, no experimental features required.
#
# Pinning: replace the URL with a specific commit for reproducibility, e.g.
#   fetchTarball {
#     url = "https://github.com/NixOS/nixpkgs/archive/<commit>.tar.gz";
#     sha256 = "<hash>";   # get it with: nix-prefetch-url --unpack <url>
#   }
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
cross.mkShell {
  nativeBuildInputs = [
    cross.stdenv.cc            # riscv32-none-elf-gcc / g++
    cross.stdenv.cc.bintools   # riscv32-none-elf-as, -ld, -objdump, ...
    pkgs.gnumake
    pkgs.cmake
    # pkgs.qemu                  # qemu-system-riscv32 for testing

    # Optional (built from source, slow):
    # cross.buildPackages.gdb  # riscv32-none-elf-gdb
  ];

  shellHook = ''
    echo "RISC-V rv32ima toolchain ready:"
    riscv32-none-elf-gcc --version | head -n1
  '';
}
