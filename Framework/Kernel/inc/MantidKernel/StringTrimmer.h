// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <algorithm>
#include <string>

namespace Mantid {
namespace Kernel {

// implement our own trim function to avoid the locale overhead in boost::trim.

// trim from start
static void trimStringFromStart(std::string &s) { s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), ::isspace)); }

// trim from end
static void trimStringFromEnd(std::string &s) {
  s.erase(std::find_if_not(s.rbegin(), s.rend(), ::isspace).base(), s.end());
}

// trim from both ends
static void trimString(std::string &s) {
  trimStringFromStart(s);
  trimStringFromEnd(s);
}

} // namespace Kernel
} // namespace Mantid
