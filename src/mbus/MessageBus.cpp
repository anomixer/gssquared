#include "MessageBus.hpp"
#include "Message.hpp"

MessageBus::MessageBus() {
    // Constructor - messages map is automatically initialized
}

void MessageBus::send(Message *message) {
    // Store the message, replacing any existing message with the same address
    messages[message->get_address()] = message;
}

Message *MessageBus::read(uint64_t address) {
    // Find the message with the given address
    auto it = messages.find(address);
    if (it != messages.end()) {
        return it->second;
    }
    return nullptr; // No message found for this address
}