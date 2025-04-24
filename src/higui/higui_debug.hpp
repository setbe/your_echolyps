#pragma once

#include "higui_platform.hpp"
#include "higui_types.hpp"

#ifndef NDEBUG

#include <stdio.h>

namespace hi::debug {}

#else // #ifndef NDEBUG

#define printf(...)

#endif // #ifndef NDEBUG