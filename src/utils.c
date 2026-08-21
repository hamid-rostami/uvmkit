#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "uvm32.h"

uvm32_state_t* uvm32_init_from_file(char *path, int32_t *error) {
  int32_t err = 0;
  FILE *f = NULL;
  uvm32_state_t* vmst = NULL;

  do {
    if (NULL == path) {
      err = -1;
      break;
    }

    vmst = malloc(sizeof(uvm32_state_t));
    if (NULL == vmst) {
      err = -2;
      break;
    }
    uvm32_init(vmst);

    /* Open the file */
    f = fopen(path, "rb");
    if (f == NULL) {
      err = -3;
      break;
    }

    /* Determine the file size */
    if(0 != fseek(f, 0, SEEK_END)) {
      err = -3;
      break;
    }
    
    long img_size;
    if (-1L == (img_size = ftell(f))) {
      err = -4;
      break;
    }
    rewind(f);

    /* Sanity check file size with available memory
     * Since vm image memory is used to read the file content
     * four bytes are reserved in the begining, because uvm32_load
     * destination and source are the same array: vmst->_memory
     */
    if ((UVM32_MEMORY_SIZE - 4) < img_size) {
      err = -5;
      break;
    }

    /* Read the file into vm memory shifted by 4, since uvm32_load is
     * going to use memcpy from this address into the beginning of
     * the same buffer
     */
    //size_t bytes_read = fread(&params->vmst->_memory[3], sizeof(uint8_t), img_size, f);
    size_t bytes_read = fread(&vmst->_memory[4], sizeof(uint8_t), img_size, f);
    if (bytes_read != img_size) {
      err = -6;
      break;
    }

    /* Load the image into vm memory */
    if(!uvm32_load(vmst, &vmst->_memory[4], img_size)) {
      err = -7;
      break;
    }
  } while (0);

  /* Cleanup */
  if (NULL != f) {
    fclose(f);
  }

  if ((NULL != vmst) && (0 != err)) {
    free(vmst);
    vmst = NULL;
  }

  return vmst;
}
