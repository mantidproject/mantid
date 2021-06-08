// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"

#include <string>
#include <vector>

namespace Mantid::API {

class MANTID_API_DLL InstrumentFileFinder {
public:
  /// Search instrument directories for Parameter file,
  /// return full path name if found, else "".
  static std::string getParameterPath(const std::string &instName, const std::string &dirHint = "");

  /// Get the IDF using the instrument name and date
  static std::string getInstrumentFilename(const std::string &instrumentName, const std::string &date = "");

  /// Utility to retrieve a resource file (IDF, Parameters, ..)
  static std::vector<std::string> getResourceFilenames(const std::string &prefix,
                                                       const std::vector<std::string> &fileFormats,
                                                       const std::vector<std::string> &directoryNames,
                                                       const std::string &date);

  /// Utility to retrieve the validity dates for the given IDF
  static void getValidFromTo(const std::string &IDFfilename, std::string &outValidFrom, std::string &outValidTo);

private:
  static std::string lookupIPF(const std::string &dir, std::string filename);
};

} // Namespace Mantid::API
