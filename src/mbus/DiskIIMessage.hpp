#pragma once

#include "Message.hpp"

struct diskii_disk_status_t {
    bool is_mounted;
    const char *filename;
    bool motor_on;
    int position;
    bool is_modified;
};

class DiskIIMessage : public Message {
    public:
        DiskIIMessage(uint32_t instance, diskii_disk_status_t *status) : Message(MESSAGE_TYPE_DISKII, instance) {}
        diskii_disk_status_t *status;
};