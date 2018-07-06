#ifndef MANTID_KERNEL_PRECOMPILEDHEADER_H_
#define MANTID_KERNEL_PRECOMPILEDHEADER_H_

#include "MantidKernel/System.h"

#if _WIN32
#define _WIN32_WINNT 0x0510
#include <winsock2.h> // prevents a conflict caused by windows.h
                      // trying to include winsock.h
#include <windows.h>
#endif

// STL
#include <cstdlib>
#include <crtdefs.h>
#include <istream>
#include <limits.h>
#include <iomanip>
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <map>
#include <string>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

// Poco
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/Notification.h>
#include <Poco/Message.h>

// NeXus
// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on

#endif
