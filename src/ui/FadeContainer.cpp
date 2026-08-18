#include "FadeContainer.hpp"

FadeContainer_t::FadeContainer_t(UIContext *ctx, const Style_t& initial_style, int fade_frames) : Container_t(ctx, initial_style) {
    this->fade_frames = fade_frames;
    fade_steps = 3;
}

bool FadeContainer_t::handle_mouse_event(const SDL_Event& event) {
    if (fade_frames == 0) return(false);
    
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        SDL_FRect eb = get_effective_bounds();
        if (event.motion.x >= eb.x && event.motion.x <= eb.x + eb.w &&
            event.motion.y >= eb.y && event.motion.y <= eb.y + eb.h) {
            frameCount = fade_frames;
            fade_steps = 0;
        } else {
            if (frameCount > 255) { // we were inside container (or submenu) and moved outside
                fade_steps = 3;
            }
        }
    }
    bool consumed = Container_t::handle_mouse_event(event);
    if (consumed && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        return true;
    }
    return(false);
}

void FadeContainer_t::render() {
    if (frameCount > 0) {
        frameCount -= fade_steps;
        if (frameCount < 0) frameCount = 0;
        int opc = frameCount > 255 ? 255 : frameCount;
        for (size_t i = 0; i < tiles.size(); i++) {
            if (tiles[i]) {
                tiles[i]->set_opacity(opc);
            }
        }
        Container_t::render();
    }
}