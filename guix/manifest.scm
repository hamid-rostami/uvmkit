(use-modules
 (gnu packages cross-base)
 (gnu packages embedded)
 (gnu packages cmake)
 (gnu packages web))


(packages->manifest
 (list
  ;; --- Host-side development tools and libraries ---
  cmake
  libwebsockets
  ;;emscripten

  ;; --- RISC-V 32-bit bare-metal cross-toolchain ---
  (cross-gcc "riscv32-elf")
  (cross-binutils "riscv32-elf")))
