#ifndef CASE_INSENSITIVE_MAP_H
#define CASE_INSENSITIVE_MAP_H

#include <boost/algorithm/string/predicate.hpp>
#include <map>

namespace Mantid {
namespace Kernel {

/** Functor to provide a case insensitive string comparator.
 */
struct CaseInsensitiveStringComparator {
  bool operator()(const std::string &s1, const std::string &s2) const {
    return boost::iequals(s1, s2);
  }
};

/// Alias template for a map data structure that has case insensitive string
/// comparision with a variable value type.
template <class T>
using CaseInsensitiveMap =
    std::map<std::string, T, CaseInsensitiveStringComparator>;
}
}

#endif // CASE_INSENSITIVE_MAP_H
