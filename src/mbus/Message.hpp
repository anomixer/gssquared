#pragma once

#include <cstdint>

enum message_type_t {
    MESSAGE_TYPE_NONE = 0,
    MESSAGE_TYPE_DISKII = 1,
    MESSAGE_TYPE_KEYBOARD = 2,
};


class Message {
    private:
        uint64_t address;
        message_type_t mtype;
        uint32_t instance;
    public:
        Message(message_type_t mtype, uint32_t instance) : mtype(mtype), instance(instance) {
            address = ((uint64_t)mtype << 32 | instance);
        }
        uint64_t get_address() { return address; };
        message_type_t get_mtype() { return mtype; };
        uint32_t get_instance() { return instance; };
};

