#pragma once

#include "Message.hpp"
#include <unordered_map>
#include <cstdint>

class MessageBus {
    public:
        MessageBus();
        void send(Message *message);
        Message *read(uint64_t address);
        Message *read(message_type_t mtype, uint32_t instance) {
            uint64_t address = ((uint64_t)mtype << 32 | instance);
            return read(address);
        }
        Message *read(message_type_t mtype) {
            uint64_t address = ((uint64_t)mtype << 32); // assume instance 0
            return read(address);
        }
    
    private:
        std::unordered_map<uint64_t, Message*> messages;
};
