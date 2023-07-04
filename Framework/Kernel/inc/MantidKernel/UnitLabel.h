// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
/**
  Define a
*/
#include "MantidKernel/DllConfig.h"
#include <string>

namespace Mantid {
namespace Kernel {
/**
 * A base-class for the a class that is able to return
 * unit labels in different representations.
 */
class MANTID_KERNEL_DLL UnitLabel {
public:
  UnitLabel() = delete;

  /// Type that contains a plain-text string
  using AsciiString = std::string;
  /// Type that can hold a unicode string. This may vary per-platform depending
  /// on the
  /// width of the built-in std::wstring
  using Utf8String = std::wstring;

  /// Constructor giving labels as ascii, unicode, and latex respectively
  UnitLabel(AsciiString ascii, Utf8String unicode, AsciiString latex);
  /// Constructor creating all labels from the ascii string
  UnitLabel(const AsciiString &ascii);
  /// Constructor creating all labels using a C-style string
  UnitLabel(const char *ascii);

  /// Equality operator with other label
  bool operator==(const UnitLabel &rhs) const;
  /// Equality operator with std::string
  bool operator==(const std::string &rhs) const;
  /// Equality operator with c-style string
  bool operator==(const char *rhs) const;
  /// Equality operator with std::wstring
  bool operator==(const std::wstring &rhs) const;

  /// Inqquality operator with other label
  bool operator!=(const UnitLabel &rhs) const;
  /// Inequality operator with std::string
  bool operator!=(const std::string &rhs) const;
  /// Inequality operator with c-style string
  bool operator!=(const char *rhs) const;
  /// Inequality operator with std::wstring
  bool operator!=(const std::wstring &rhs) const;

  /// Return an ascii label for unit
  const AsciiString &ascii() const;
  /// Return a utf-8 encoded label for unit
  const Utf8String &utf8() const;
  /// Return an ascii latex compatible label for unit
  const AsciiString &latex() const;

  /// Implicit conversion to std::string
  operator std::string() const;

private:
  /// Value of plain-text label
  std::string m_ascii;
  /// Value of utf-8 encoded string
  std::wstring m_utf8;
  /// Value of latex label
  std::string m_latex;
};

} // namespace Kernel
} // namespace Mantid
