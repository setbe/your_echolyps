// For now simply provides macro `debug_print` which will be ignored if
/// not defined NDEBUG

#pragma once

#if !defined(NDEBUG) || defined(HI_PUBLIC)

#include <stdio.h>

#define debug_print printf

#else // !defined(NDEBUG) || defined(HI_PUBLIC)

#define debug_print(...)

#endif // !defined(NDEBUG) || defined(HI_PUBLIC)