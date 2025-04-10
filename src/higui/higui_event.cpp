#pragma once

extern "C" const int _fltused = 0;

#include "higui.hpp"

namespace hi {

    namespace callback { // Callbacks
        internal::VoidCallbackHandler render = internal::void_noop_win;
        internal::VoidCallbackHandler update = internal::void_noop_win;
        internal::VoidCallbackHandlerIntInt resize = internal::void_noop_win_int_int;
        internal::VoidCallbackHandlerIntInt mouse_move = internal::void_noop_win_int_int;
        internal::VoidCallbackHandlerInt key_up = internal::void_noop_win_int;
        internal::VoidCallbackHandlerInt key_down = internal::void_noop_win_int;
        internal::VoidCallbackHandler focus_gained = internal::void_noop_win;
        internal::VoidCallbackHandler focus_lost = internal::void_noop_win;
    } // namespace callback

    // Key states (pressed — true, otherwise — false)
    unsigned char key[256] = {0};
} // namespace hi