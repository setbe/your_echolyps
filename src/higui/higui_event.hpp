#pragma once

#include "higui.hpp"

namespace hi {
    namespace callback {
        namespace internal {
            inline void void_noop_win(window::Handler) noexcept {}
            inline void void_noop_win_int(window::Handler, int) noexcept {}
            inline void void_noop_win_int_int(window::Handler, int, int) noexcept {}

            typedef void (*VoidCallbackHandler)(window::Handler);
            typedef void (*VoidCallbackHandlerInt)(window::Handler, int);
            typedef void (*VoidCallbackHandlerIntInt)(window::Handler, int, int);
        } // namespace internal

        // Callbacks
        extern internal::VoidCallbackHandler render;
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