// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace Mantid {
namespace Kernel {
/** An enumerator based on a set of string values
 * This is to facilitate properties that are
 * fixed lists of strings.

@author Dmitry Ganyushin, ORNL
@date 2024/09/17
*/

template <class E, const std::vector<std::string> *names,
          std::function<bool(const std::string &, const std::string &)> *stringComparator = &compareStrings>
class EnumeratedStringPropety : EnumeratedString {
  /**
   * @tparam class E an `enum`, the final value *must* be `enum_count`
   *              (i.e. `enum class Fruit {apple, orange, enum_count}`)
   * @tparam a pointer to a static vector of string names for each enum
   * The enum and string array *must* have same order.
   *
   * @tparam an optional pointer to a statically defined string comparator.
   */

public:
  EnumeratedStringProperty() {}

private:
};
} // namespace Kernel
} // namespace Mantid
