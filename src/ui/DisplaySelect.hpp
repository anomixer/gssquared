#pragma once

#include "Container.hpp"
#include <map>
#include "MainAtlas.hpp"
#include "util/MenuInterface.h"

class DisplaySelect : public Container_t {
protected:
    const std::map<int, int> monitor_asset =  {
        {MONITOR_COMPOSITE, ColorDisplayButton},
        {MONITOR_GS_RGB, RGBDisplayButton},
        {MONITOR_MONO_GREEN, GreenDisplayButton},
        {MONITOR_MONO_AMBER, AmberDisplayButton},
        {MONITOR_MONO_WHITE, WhiteDisplayButton},
    };
public:
    DisplaySelect(UIContext *ctx, const Style_t& initial_style = Style_t());
    ~DisplaySelect();
    virtual void set_visible(bool visible) override;
    virtual void render() override;
    virtual bool handle_mouse_event(const SDL_Event& event) override;
};