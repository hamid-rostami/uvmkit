void *_sbrk(int incr) {
    extern char __heap_end;          // defined by linker script
    static char *heap = &__heap_end;
    char *prev = heap;
    heap += incr;
    return (void *)prev;
}
