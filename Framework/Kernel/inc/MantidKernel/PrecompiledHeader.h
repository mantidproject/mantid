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
#include <crtdefs.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <istream>
#include <limits.h>
#include <map>
#include <string>
#include <vector>

// Boost
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

// Poco
#include <Poco/File.h>
#include <Poco/Message.h>
#include <Poco/Notification.h>
#include <Poco/Path.h>

// NeXus
#include <nexus/NeXusException.hpp>
#include <nexus/NeXusFile.hpp>

#endif
