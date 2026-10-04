#include <errno.h>
#include <sys/stat.h>

#include "uvm32_target.h"

void *_sbrk(int incr) {
    extern char __heap_end;          // defined by linker script
    static char *heap = &__heap_end;
    char *prev = heap;
    heap += incr;
    return (void *)prev;
}

/* Retarget newlib's stdio output to UVM32's host character output syscall. */
int _write(int file, char *buf, int len) {
  if ((file != 1 && file != 2) || len < 0 || (len > 0 && buf == 0)) {
    errno = EBADF;
    return -1;
  }

  println(buf);

  return len;
}

/* Newlib checks the stream type before selecting its stdio buffering mode. */
int _fstat(int file, struct stat *st) {
  if ((file != 0 && file != 1 && file != 2) || st == 0) {
    errno = EBADF;
    return -1;
  }

  st->st_mode = S_IFCHR;
  st->st_size = 0;
  return 0;
}

int _isatty(int file) {
  if (file == 0 || file == 1 || file == 2) {
    return 1;
  }

  errno = EBADF;
  return 0;
}
