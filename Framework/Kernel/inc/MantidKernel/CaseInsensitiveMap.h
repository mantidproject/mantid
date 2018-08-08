#ifndef MANTID_KERNEL_CASE_INSENSITIVE_MAP_H
#define MANTID_KERNEL_CASE_INSENSITIVE_MAP_H

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
template <class T>
using CaseInsensitiveMap =
    std::map<std::string, T, CaseInsensitiveStringComparator>;
} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_CASE_INSENSITIVE_MAP_H
