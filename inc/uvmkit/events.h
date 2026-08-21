#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "uvm32.h"

//int32_t syscalls_generic_handler(uvm32_state_t *vmst, uvm32_evt_t *evt);

int32_t uvmkit_event_handler(uvm32_state_t *vmst, uvm32_evt_t *evt);
