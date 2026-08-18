#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <SDL3/SDL.h>


class EventDispatcher {
public:
    using EventHandler = std::function<bool(const SDL_Event&)>;
    
private:
    // Map SDL event types to lists of handlers
    std::unordered_map<Uint32, std::vector<EventHandler>> handlers;
    
public:
    EventDispatcher() {}

    // Register a handler for specific SDL event type(s)
    void registerHandler(Uint32 eventType, EventHandler handler);

    //bool handlePreEvents(const SDL_Event& event);
    
    // Register handler for multiple event types
    void registerHandler(const std::vector<Uint32>& eventTypes, EventHandler handler);
    
    // Dispatch an SDL event to all registered handlers
    // Returns true if event was handled (consumed)
    bool dispatch(const SDL_Event& event);
    
    // Main event loop integration
    /* void processEvents(); */
    
private:
};