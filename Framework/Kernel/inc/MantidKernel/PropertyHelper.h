// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#include <memory>
#endif

#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/WarningSuppressions.h"

#include <type_traits>

namespace Mantid {
namespace Kernel {

// --------------------- convert values to strings
/// Convert values to strings.
template <typename T> std::string toString(const T &value) { return boost::lexical_cast<std::string>(value); }

/// Throw an exception if a shared pointer is converted to a string.
template <typename T> std::string toString(const std::shared_ptr<T> &value) {
  UNUSED_ARG(value);
  throw boost::bad_lexical_cast();
}

/// Specialization for a property of type std::vector.
template <typename T> std::string toString(const std::vector<T> &value, const std::string &delimiter = ",") {
  std::stringstream result;
  std::size_t vsize = value.size();
  for (std::size_t i = 0; i < vsize; ++i) {
    result << value[i];
    if (i + 1 != vsize)
      result << delimiter;
  }
  return result.str();
}

/// Specialization for a property of type std::vector<std::vector>.
template <typename T>
std::string toString(const std::vector<std::vector<T>> &value, const std::string &outerDelimiter = ",",
                     const std::string &innerDelimiter = "+") {
  std::stringstream result;
  std::size_t vsize = value.size();
  for (std::size_t i = 0; i < vsize; ++i) {
    std::size_t innervsize = value[i].size();
    for (std::size_t j = 0; j < innervsize; ++j) {
      result << value[i][j];
      if (j + 1 != innervsize)
        result << innerDelimiter;
    }

    if (i + 1 != vsize)
      result << outerDelimiter;
  }
  return result.str();
}

// --------------------- convert values to pretty strings
/// Convert values to pretty strings.
template <typename T> std::string toPrettyString(const T &value, size_t maxLength = 0, bool collapseLists = true) {
  UNUSED_ARG(collapseLists);
  return Strings::shorten(boost::lexical_cast<std::string>(value), maxLength);
}

/// Throw an exception if a shared pointer is converted to a pretty string.
template <typename T>
std::string toPrettyString(const std::shared_ptr<T> &value, size_t maxLength = 0, bool collapseLists = true) {
  UNUSED_ARG(value);
  UNUSED_ARG(maxLength);
  UNUSED_ARG(collapseLists);
  throw boost::bad_lexical_cast();
}

/** Specialization for a property of type std::vector of non integral types.
 *   This will catch Vectors of char, double, float etc.
 *   This simply concatenates the values using a delimiter
 */
template <typename T>
std::string toPrettyString(
    const std::vector<T> &value, size_t maxLength = 0, bool collapseLists = true, const std::string &delimiter = ",",
    const std::string &unusedDelimiter = "+",
    typename std::enable_if<!(std::is_integral<T>::value && std::is_arithmetic<T>::value)>::type * = nullptr) {
  UNUSED_ARG(unusedDelimiter);
  UNUSED_ARG(collapseLists);
  return Strings::shorten(Strings::join(value.begin(), value.end(), delimiter), maxLength);
}

/** Specialization for a property of type std::vector of integral types.
 *   This will catch Vectors of int, long, long long etc
 *   including signed and unsigned types of these.
 *   This concatenates the values using a delimiter,
 *   adjacent items that are precisely 1 away from each other
 *   will be compressed into a list syntax e.g. 1-5.
 */
template <typename T>
std::string
toPrettyString(const std::vector<T> &value, size_t maxLength = 0, bool collapseLists = true,
               const std::string &delimiter = ",", const std::string &listDelimiter = "-",
               typename std::enable_if<std::is_integral<T>::value && std::is_arithmetic<T>::value>::type * = nullptr) {
  std::string retVal;
  if (collapseLists) {
    retVal = Strings::joinCompress(value.begin(), value.end(), delimiter, listDelimiter);
  } else {
    retVal = Strings::join(value.begin(), value.end(), delimiter);
  }
  return Strings::shorten(retVal, maxLength);
}

GNU_DIAG_OFF("unused-function")

/** Explicit specialization for a property of type std::vector<bool>.
 *   This will catch Vectors of char, double, float etc.
 *   This simply concatenates the values using a delimiter
 */

template <>
inline std::string toPrettyString(const std::vector<bool> &value, size_t maxLength, bool collapseLists,
                                  const std::string &delimiter, const std::string &unusedDelimiter,
                                  typename std::enable_if<std::is_same<bool, bool>::value>::type *) {
  UNUSED_ARG(unusedDelimiter);
  UNUSED_ARG(collapseLists);
  return Strings::shorten(Strings::join(value.begin(), value.end(), delimiter), maxLength);
}

GNU_DIAG_ON("unused-function")

/// Specialization for a property of type std::vector<std::vector>.
template <typename T>
std::string toPrettyString(const std::vector<std::vector<T>> &value, size_t maxLength = 0, bool collapseLists = true,
                           const std::string &outerDelimiter = ",", const std::string &innerDelimiter = "+") {
  UNUSED_ARG(collapseLists);
  return Strings::shorten(toString<T>(value, outerDelimiter, innerDelimiter), maxLength);
}

/// Specialization for any type, should be appropriate for properties with a
/// single value.
template <typename T> int findSize(const T &) { return 1; }

/// Specialization for properties that are of type vector.
template <typename T> int findSize(const std::vector<T> &value) { return static_cast<int>(value.size()); }

// ------------- Convert strings to values
template <typename T> inline void appendValue(const std::string &strvalue, std::vector<T> &value) {
  // try to split the string
  std::size_t pos = strvalue.find(':');
  std::size_t numChar = std::string::npos; // go to the end of the string
  T step{1};
  bool dashSeparator = false;
  if (pos == std::string::npos) {
    pos = strvalue.find('-', 1);
    if (pos != std::string::npos)
      dashSeparator = true;
  } else {
    const auto posStep = strvalue.find(':', pos + 1);
    if (posStep != std::string::npos) {
      step = boost::lexical_cast<T>(strvalue.substr(posStep + 1));
      numChar = posStep - pos - 1;
    }
  }

  // just convert the whole thing into a value
  if (pos == std::string::npos) {
    value.emplace_back(boost::lexical_cast<T>(strvalue));
    return;
  }

  if (step == static_cast<T>(0))
    throw std::logic_error("Step size must be non-zero");

  // convert the input string into boundaries and run through a list
  auto start = boost::lexical_cast<T>(strvalue.substr(0, pos));
  auto stop = boost::lexical_cast<T>(strvalue.substr(pos + 1, numChar));

  if ((start > stop) && (dashSeparator)) {
    std::swap(start, stop);
  }

  if (start <= stop) {
    if (start + step < start)
      throw std::logic_error("Step size is negative with increasing limits");
    for (auto i = start; i <= stop;) {
      value.emplace_back(i);
      // done inside the loop because gcc7 doesn't like i+=step for short
      // unsigned int
      i = static_cast<T>(i + step);
    }
  } else {
    if (start + step >= start)
      throw std::logic_error("Step size is positive with decreasing limits");
    for (auto i = start; i >= stop;) {
      value.emplace_back(i);
      // done inside the loop because gcc7 doesn't like i+=step for short
      // unsigned int
      i = static_cast<T>(i + step);
    }
  }
}

template <typename T> void toValue(const std::string &strvalue, T &value) { value = boost::lexical_cast<T>(strvalue); }

template <typename T> void toValue(const std::string &, std::shared_ptr<T> &) { throw boost::bad_lexical_cast(); }

namespace detail {
// vector<int> specializations
template <typename T> void toValue(const std::string &strvalue, std::vector<T> &value, std::true_type) {
  using tokenizer = Mantid::Kernel::StringTokenizer;
  tokenizer values(strvalue, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
  value.clear();
  value.reserve(values.count());
  for (const auto &token : values) {
    appendValue(token, value);
  }
}

template <typename T> void toValue(const std::string &strvalue, std::vector<T> &value, std::false_type) {
  // Split up comma-separated properties
  using tokenizer = Mantid::Kernel::StringTokenizer;
  tokenizer values(strvalue, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);

  value.clear();
  value.reserve(values.count());
  std::transform(values.cbegin(), values.cend(), std::back_inserter(value),
                 [](const std::string &str) { return boost::lexical_cast<T>(str); });
}

// bool and char don't make sense as types to generate a range of values.
// This is similar to std::is_integral<T>, but bool and char are std::false_type
template <class T> struct is_range_type : public std::false_type {};
template <class T> struct is_range_type<const T> : public is_range_type<T> {};
template <class T> struct is_range_type<volatile const T> : public is_range_type<T> {};
template <class T> struct is_range_type<volatile T> : public is_range_type<T> {};

template <> struct is_range_type<unsigned short> : public std::true_type {};
template <> struct is_range_type<unsigned int> : public std::true_type {};
template <> struct is_range_type<unsigned long> : public std::true_type {};
template <> struct is_range_type<unsigned long long> : public std::true_type {};

template <> struct is_range_type<short> : public std::true_type {};
template <> struct is_range_type<int> : public std::true_type {};
template <> struct is_range_type<long> : public std::true_type {};
template <> struct is_range_type<long long> : public std::true_type {};
} // namespace detail
template <typename T> void toValue(const std::string &strvalue, std::vector<T> &value) {
  detail::toValue(strvalue, value, detail::is_range_type<T>());
}

template <typename T>
void toValue(const std::string &strvalue, std::vector<std::vector<T>> &value, const std::string &outerDelimiter = ",",
             const std::string &innerDelimiter = "+") {
  using tokenizer = Mantid::Kernel::StringTokenizer;
  tokenizer tokens(strvalue, outerDelimiter, tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);

  value.clear();
  value.reserve(tokens.count());

  for (const auto &token : tokens) {
    tokenizer values(token, innerDelimiter, tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
    std::vector<T> vect;
    vect.reserve(values.count());
    std::transform(values.begin(), values.end(), std::back_inserter(vect),
                   [](const std::string &str) { return boost::lexical_cast<T>(str); });
    value.emplace_back(std::move(vect));
  }
}

/*Used specifically to retrieve a vector of type T populated with values
 * given to it from strvalue parameter, Using toValue method.
 (See constructor used specifically for vector assignments)
 */
template <typename T> T extractToValueVector(const std::string &strvalue) {
  T valueVec;
  toValue(strvalue, valueVec);
  return valueVec;
}

//------------------------------------------------------------------------------------------------
// Templated += operator functions for specific types
template <typename T> inline void addingOperator(T &lhs, const T &rhs) {
  // The cast here (and the expansion of the compound operator which that
  // necessitates) is because if this function is created for a template
  // type narrower than an int, the compiler will expande the operands to
  // ints which leads to a compiler warning when it's assigned back to the
  // narrower type.
  lhs = static_cast<T>(lhs + rhs);
}

template <typename T> inline void addingOperator(std::vector<T> &lhs, const std::vector<T> &rhs) {
  // This concatenates the two
  if (&lhs != &rhs) {
    lhs.insert(lhs.end(), rhs.begin(), rhs.end());
  } else {
    std::vector<T> rhs_copy(rhs);
    lhs.insert(lhs.end(), rhs_copy.begin(), rhs_copy.end());
  }
}

template <> inline void addingOperator(bool &, const bool &) {
  throw Exception::NotImplementedError("PropertyWithValue.h: += operator not implemented for type bool");
}

template <> inline void addingOperator(OptionalBool &, const OptionalBool &) {
  throw Exception::NotImplementedError("PropertyWithValue.h: += operator not implemented for type OptionalBool");
}

template <typename T> inline void addingOperator(std::shared_ptr<T> &, const std::shared_ptr<T> &) {
  throw Exception::NotImplementedError("PropertyWithValue.h: += operator not implemented for std::shared_ptr");
}

template <typename T> inline std::vector<std::string> determineAllowedValues(const T &, const IValidator &validator) {
  return validator.allowedValues();
}

template <> inline std::vector<std::string> determineAllowedValues(const OptionalBool &, const IValidator &) {
  auto enumMap = OptionalBool::enumToStrMap();
  std::vector<std::string> values;
  values.reserve(enumMap.size());
  std::transform(enumMap.cbegin(), enumMap.cend(), std::back_inserter(values),
                 [](const std::pair<OptionalBool::Value, std::string> &str) { return str.second; });
  return values;
}

} // namespace Kernel
} // namespace Mantid
