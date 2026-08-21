#pragma once

#include <stdint.h>
#include "uvm32.h"

uvm32_state_t* uvm32_init_from_file(char *path, int32_t *error);
