#pragma once

#include "Device_ID.hpp"
#include "gs2.hpp"

/**
 * Every slot device data struct inherits this - allows to find device type by id, and,
 * get slot number when needed.
 */

struct SlotData {
    device_id id;
    SlotType_t _slot;
};
