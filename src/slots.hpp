#pragma once

#include "gs2.hpp"
#include "devices.hpp"

struct Device_t;

struct Slot_t {
    SlotType_t slot_number;
    Device_t *card;
};

class SlotManager_t {
    Slot_t Slots[NUM_SLOTS];
    
    public:
        SlotManager_t();
        ~SlotManager_t();
        void register_slot(Device_t *device, SlotType_t slot_number);
        void unregister_slot(SlotType_t slot_number);
        Device_t *get_device(SlotType_t slot_number);
};
