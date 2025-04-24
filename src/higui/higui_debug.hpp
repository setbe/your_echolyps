#pragma once

#include "higui_platform.hpp"
#include "higui_types.hpp"

#if !defined(NDEBUG) || defined(HI_PUBLIC)

#include <stdio.h>

namespace hi::debug {}

#else // !defined(NDEBUG) || defined(HI_PUBLIC)

#define printf(...)

#endif // !defined(NDEBUG) || defined(HI_PUBLIC)