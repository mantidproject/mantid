// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_PRECOMPILEDHEADER_H_
#define MANTID_KERNEL_PRECOMPILEDHEADER_H_

#include "MantidKernel/System.h"

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
// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on

#endif
