#include "DeviceFrameDispatcher.hpp"

DeviceFrameDispatcher::DeviceFrameDispatcher() {
}

DeviceFrameDispatcher::~DeviceFrameDispatcher() {
}

void DeviceFrameDispatcher::registerHandler(EventHandler handler) {
    handlers.push_back(handler);
}

void DeviceFrameDispatcher::dispatch() {
    for (auto& handler : handlers) {
        handler();
    }
}