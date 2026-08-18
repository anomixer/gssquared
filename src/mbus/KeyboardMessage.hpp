#pragma once

#include "Message.hpp"

struct message_keyboard_t {
    uint8_t last_key_val = 0x00;
};

class KeyboardMessage : public Message {
    public:
        KeyboardMessage(message_keyboard_t *msg) : Message(MESSAGE_TYPE_KEYBOARD, 0) {
            this->mk = msg;
        }
        message_keyboard_t *mk;
};