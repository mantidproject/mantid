#pragma once

#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

/*
 * Integer type definitions
 *
 * int32_t etc will be defined by configure in nxconfig.h
 * if they exist; otherwise include an appropriate header
 */
#include <stdint.h>
