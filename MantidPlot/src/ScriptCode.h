// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SCRIPTCODE_H_
#define SCRIPTCODE_H_

#include <QString>

/**
 * Code objects represent the code as a string but also store an
 * optional offset that defines where they are within a larger
 * chunk of code. They can be created directly from strings
 * and are also implicitly convertible to strings
 */
class ScriptCode {
public:
  /// Empty code
  ScriptCode();
  /// Code from a C-string with zero offset
  explicit ScriptCode(const char *codeStr);
  /// Code from a C-string with a defined offset
  ScriptCode(const char *codeStr, const int offset);
  /// Code from a QString with zero offset
  explicit ScriptCode(const QString &codeStr);
  /// Code from a QString with a defined offset
  ScriptCode(const QString &codeStr, const int offset);

  /// Code string
  inline const std::string &codeString() const { return m_code; }
  /// Return the offset
  inline int offset() const { return m_offset; }
  /// Is the string empty
  inline bool isEmpty() const { return m_code.empty(); }

private:
  /// Ensure all code line endings are unix style
  void convertLineEndingsToUnix();
  /// The code string
  std::string m_code;
  /// The offset within a larger chunk of code
  int m_offset;
};

#endif /* SCRIPTCODE_H_ */
