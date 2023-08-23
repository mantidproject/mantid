// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/** An enumerator based on a set of string values
 * This is to facilitate properties that are
 * fixed lists of strings.

@author Reece Boston, ORNL
@date 2023/08/02
*/

template <class E, const std::vector<std::string> *names> class EnumeratedString {
  /**
   * @tparam class E an `enum`, the final value *must* be `enum_count`
   *              (i.e. `enum class Fruit {apple, orange, enum_count}`)
   * @tparam a pointer to a static vector of string names for each enum
   * The enum and string array *must* have same order.
   */
public:
  EnumeratedString() { ensureCompatibleSize(); }
  EnumeratedString(const E e) {
    ensureCompatibleSize();
    try {
      this->operator=(e);
    } catch (std::exception &err) {
      throw err;
    }
  }
  EnumeratedString(const std::string s) {
    ensureCompatibleSize();
    // only set values if valid string given
    try {
      this->operator=(s);
    } catch (std::exception &err) {
      throw err;
    }
  }

  EnumeratedString(const EnumeratedString &es) : value(es.value), name(es.name) {}

  // treat the object as either the enum, or a string
  operator E() const { return value; }
  operator std::string() const { return name; }
  // assign the object either by the enum, or by string
  EnumeratedString &operator=(E e) {
    if (size_t(e) < names->size() && size_t(e) >= 0) {
      value = e;
      name = names->at(size_t(e));
    } else {
      std::stringstream msg;
      msg << "Invalid enumerator " << size_t(e) << " for enumerated string " << typeid(E).name();
      throw std::runtime_error(msg.str());
    }
    return *this;
  }
  EnumeratedString &operator=(std::string s) {
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
  bool operator==(const E e) const { return value == e; }
  bool operator!=(const E e) const { return value != e; }
  bool operator==(const std::string s) const { return name == s; }
  bool operator!=(const std::string s) const { return name != s; }
  bool operator==(const char *s) const { return name == std::string(s); }
  bool operator!=(const char *s) const { return name != std::string(s); }
  bool operator==(const EnumeratedString es) const { return value == es.value; }
  bool operator!=(const EnumeratedString es) const { return value != es.value; }
  const char *c_str() const { return name.c_str(); }
  static size_t size() { return names->size(); }

private:
  E value;
  std::string name;

  // given a string, find the corresponding enum value
  E findEFromString(const std::string s) {
    E e = E(0);
    for (; size_t(e) < names->size(); e = E(size_t(e) + 1))
      if (s == names->at(size_t(e)))
        break;
    return e;
  }

  void ensureCompatibleSize() {
    if (size_t(E::enum_count) != names->size()) {
      // std::stringstream msg;
      // msg << "Size of " << typeid(E).name() << " incompatible with vector of names: ";
      // msg << size_t(E::enum_count) << " vs. " << names->size() << std::endl;
      // throw std::runtime_error(msg.str());
      static_assert(names->size() == size_t(E::enum_count));
    }
  }
};

} // namespace Kernel
} // namespace Mantid
