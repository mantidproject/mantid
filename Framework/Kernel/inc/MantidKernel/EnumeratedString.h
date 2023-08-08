// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

namespace Mantid {
namespace Kernel {
/** An enumerator based on a set of string values
 * This is to facilitate properties that are
 * fixed lists of strings.

@author Reece Boston, ORNL
@date 2023/08/02
*/

namespace {
// this function "uses" a variable so the compiler and linter won't complain
template <class T> void use(T) {}
} // namespace

template <class E, const std::string names[size_t(E::enum_count)]> class EnumeratedString {
  /**
   * @property class E an `enum`, the final value *must* be `enum_count`
   *              (i.e. `enum class Fruit {apple, orange, enum_count}`)
   * @tparam string names[] a static c-style array of string names for each enum
   * Note that no checking is done on the compatibility of `E` and `names`, or their validity.
   * The enum and string array *must* have same order.
   */
public:
  constexpr EnumeratedString() {
    // force a compiler error if no E::enum_count
    use<E>(E::enum_count); // Last element of enum MUST be enum_count
  }
  EnumeratedString(const E &e) {
    try {
      this->operator=(e);
    } catch (std::exception &err) {
      throw err;
    }
  }
  EnumeratedString(const std::string &s) {
    // only set values if valid string given
    try {
      this->operator=(s);
    } catch (std::exception &err) {
      throw err;
    }
  }

  EnumeratedString(const EnumeratedString &es) : value(es.value), name(es.name) {}

  // treat the object as either the enum, or a string
  constexpr operator E() const { return value; }
  constexpr operator std::string() const { return name; }
  // assign the object either by the enum, or by string
  constexpr EnumeratedString &operator=(E e) {
    if (size_t(e) < size_t(E::enum_count) && size_t(e) >= 0) {
      value = e;
      name = names[size_t(e)];
    } else {
      std::stringstream msg;
      msg << "Invalid enumerator " << size_t(e) << " for enumerated string " << typeid(E).name();
      throw std::runtime_error(msg.str());
    }
    return *this;
  }
  constexpr EnumeratedString &operator=(std::string s) {
    E e = findEFromString(s);
    if (e != E::enum_count) {
      value = e;
      name = s;
    } else {
      std::stringstream msg;
      msg << "Invalid string " << s << " for enumerated string " << typeid(E).name();
      throw std::runtime_error(msg.str());
    }
    return *this;
  }
  // for comparison of the object to either enums or strings
  constexpr bool operator==(E e) const { return value == e; }
  constexpr bool operator!=(E e) const { return value != e; }
  constexpr bool operator==(std::string s) const { return name == s; }
  constexpr bool operator!=(std::string s) const { return name != s; }
  constexpr bool operator==(const char *s) const { return name == std::string(s); }
  constexpr bool operator!=(const char *s) const { return name != std::string(s); }
  constexpr bool operator==(EnumeratedString es) const { return value == es.value; }
  constexpr bool operator!=(EnumeratedString es) const { return value != es.value; }
  constexpr const char *c_str() const { return name.c_str(); }

private:
  E value;
  std::string name;

  // given a string, find the corresponding enum value
  constexpr E findEFromString(std::string s) {
    E e = E(0);
    for (; size_t(e) < size_t(E::enum_count); e = E(size_t(e) + 1))
      if (s == names[size_t(e)])
        break;
    return e;
  }
};

} // namespace Kernel
} // namespace Mantid
