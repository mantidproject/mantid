// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cstring>
#include <map>
#include <string>

namespace Mantid {
namespace Kernel {

/** Functor to provide a case insensitive string comparator.
 */
struct CaseInsensitiveStringComparator {
  bool operator()(const std::string &s1, const std::string &s2) const {
#ifdef _MSC_VER
    return _stricmp(s1.c_str(), s2.c_str()) < 0;
#else
    return strcasecmp(s1.c_str(), s2.c_str()) < 0;
#endif
  }
};

/// Alias template for a map data structure that has case insensitive string
/// comparision with a variable value type.
template <class T> using CaseInsensitiveMap = std::map<std::string, T, CaseInsensitiveStringComparator>;
} // namespace Kernel
} // namespace Mantid
