#include "uvm32_target.h"
#include <string.h>
//#include <stdint.h>


int main(void) {
  //volatile uint8_t buf[] = {"Hello world from RISC-V :)"};
  //memset(buf, 0, 10);

  /* struct { */
  /*   char s[3]; */
  /*   uint8_t buf[12];// __attribute__ ((aligned (4))); */
  /* } a; */
  /* uint32_t foo = 0x1234; */
  /* __builtin_memcpy(a.buf, &foo, 4); */
  /* foo++; */
  /* __builtin_memcpy(&a.buf[4], &foo, 4); */
  /* foo++; */
  /* __builtin_memcpy(&a.buf[8], &foo, 4); */
  
  println("Hello world from RISC-V :)");

  
  return 0;
}

