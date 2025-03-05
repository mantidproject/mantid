// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
#include <memory>

// Poco
#include <Poco/File.h>
#include <Poco/Message.h>
#include <Poco/Notification.h>
#include <Poco/Path.h>

// NeXus
#include "MantidNexus/NeXusException.hpp"
#include "MantidNexus/NeXusFile.hpp"
