// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** ALCLatestFileFinder : Utility to find most recent file in a directory
 */
class MANTIDQT_MUONINTERFACE_DLL ALCLatestFileFinder {
public:
  /// Constructor - takes filename of first run
  explicit ALCLatestFileFinder(const std::string &firstRunFile) : m_firstRunFileName(firstRunFile) {};

  /// Find most recent file in same directory as first run
  std::string getMostRecentFile() const;

protected:
  /// Check validity of filename
  bool isValid(const std::string &path) const;

private:
  /// Filename of first run
  std::string m_firstRunFileName;
};

} // namespace CustomInterfaces
} // namespace MantidQt
