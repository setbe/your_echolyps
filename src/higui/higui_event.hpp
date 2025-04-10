#pragma once

#include "higui_types.hpp"
#include "higui.hpp"

namespace hi {
    namespace callback {
        // Callbacks
        extern internal::VoidCallbackHandler start;
        extern internal::VoidCallbackHandler render; 
        extern internal::VoidCallbackHandler update;
        extern internal::VoidCallbackHandlerIntInt resize;
        extern internal::VoidCallbackHandlerIntInt mouse_move;
        extern internal::VoidCallbackHandlerInt key_up;
        extern internal::VoidCallbackHandlerInt key_down;
        extern internal::VoidCallbackHandler focus_gained;
        extern internal::VoidCallbackHandler focus_lost;
    } // namespace callback

    // Key states (pressed — true, otherwise — false)
    extern unsigned char key[256];
} // namespace hi