#pragma once

#include "higui_platform.hpp"
#include "higui_types.hpp"

#if !defined(NDEBUG) || defined(HI_PUBLIC)

#include <stdio.h>

#define debug_print printf

#else // !defined(NDEBUG) || defined(HI_PUBLIC)

#define debug_print(...)

#endif // !defined(NDEBUG) || defined(HI_PUBLIC)