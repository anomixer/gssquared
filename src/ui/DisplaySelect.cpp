#include "DisplaySelect.hpp"
#include "SelectButton.hpp"
#include "MainAtlas.hpp"

DisplaySelect::DisplaySelect(UIContext *ctx, const Style_t& initial_style) : Container_t(ctx, initial_style) {
    Style_t CB = {
        .background_color = 0x00000000,
        .border_color = 0x000000FF,
        .hover_color = 0x00C0C0FF,
        .padding = 2,
        .border_width = 1,        
    };

    // don't set position yet, we'll do that when we open the submenu.
    size(360, 65);
    SelectButton_t *mc1 = new SelectButton_t(ctx, ColorDisplayButton, CB, MONITOR_COMPOSITE);
    SelectButton_t *mc2 = new SelectButton_t(ctx, RGBDisplayButton, CB, MONITOR_GS_RGB);
    SelectButton_t *mc3 = new SelectButton_t(ctx, GreenDisplayButton, CB, MONITOR_MONO_GREEN);
    SelectButton_t *mc4 = new SelectButton_t(ctx, AmberDisplayButton, CB, MONITOR_MONO_AMBER);
    SelectButton_t *mc5 = new SelectButton_t(ctx, WhiteDisplayButton, CB, MONITOR_MONO_WHITE);
    add(mc1);
    add(mc2);
    add(mc3);
    add(mc4);
    add(mc5);
    layout();
    set_visible(false);

    // iterate tiles and set onclick for each
    for (size_t i = 0; i < tiles.size(); i++) {
        Tile_t *tile = tiles[i];
        tile->on_click([this,tile](const SDL_Event& event) -> bool {
            this->selected_value(tile->value());
            getMenuInterface()->setMonitor(tile->value());
            this->set_visible(false);
            return true;
        });
    }

    style.hover_color = 0xEE0000FF;
}

// whenever we go from invisible to visible, set the selected value to the current clock mode
void DisplaySelect::set_visible(bool visible) {
    Container_t::set_visible(visible);
    if (visible) {
        this->selected_value(getMenuInterface()->getCurrentMonitor());
    }
}

void DisplaySelect::render() {
    if (!visible) return;
    Container_t::render();
}

bool DisplaySelect::handle_mouse_event(const SDL_Event& event) {
    if (!visible) return false;
    bool consumed = Container_t::handle_mouse_event(event);
    //printf("DisplaySelect::handle_mouse_event: %d\n", consumed);
    if (consumed && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        return true;
    }
    return false;
}

DisplaySelect::~DisplaySelect() {
    // deallocate all tiles
    for (size_t i = 0; i < tiles.size(); i++) {
        delete tiles[i];
    }
    tiles.clear();
}