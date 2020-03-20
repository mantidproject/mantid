// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/AttenuationProfile.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>

namespace Mantid {
namespace Kernel {

/**
 * Construct an attenuation profile object
 * @param inputFileName :: The name of the file containing the attenuation
 * profile data. Filename can be a full path or just file name
 * @param searchPath :: Path to search for the input file
 */
AttenuationProfile::AttenuationProfile(const std::string inputFileName,
                                       const std::string searchPath) {
  Poco::Path suppliedFileName(inputFileName);
  Poco::Path inputFilePath;
  std::string fileExt = suppliedFileName.getExtension();
  std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), toupper);

  if (fileExt == "DAT") {
    if (suppliedFileName.isRelative()) {
      bool useSearchDirectories = true;

      if (!searchPath.empty()) {
        inputFilePath =
            Poco::Path(Poco::Path(searchPath).parent(), inputFileName);
        if (Poco::File(inputFilePath).exists()) {
          useSearchDirectories = false;
        }
      }
      if (useSearchDirectories) {
        // ... and if that doesn't work look in the search directories
        std::string foundFile =
            Mantid::Kernel::ConfigService::Instance().getFullPath(inputFileName,
                                                                  false, 0);
        if (!foundFile.empty()) {
          inputFilePath = Poco::Path(foundFile);
        } else {
          inputFilePath = suppliedFileName;
        }
      }
    } else {
      inputFilePath = suppliedFileName;
    }
    std::ifstream input(inputFilePath.toString(), std::ios_base::in);
    if (input) {
      std::string line;
      while (std::getline(input, line)) {
        double lambda, alpha, error;
        if (std::stringstream(line) >> lambda >> alpha >> error) {
          m_Interpolator.addPoint(lambda, 1000 * alpha);
        }
      }
      input.close();
    } else {
      throw Exception::FileError("Error reading attenuation profile file",
                                 inputFileName);
    }
  } else {
    throw Exception::FileError("Attenuation profile file must be a .DAT file",
                               inputFileName);
  }
}

/**
 * Returns the attenuation coefficient for the supplied wavelength
 * @param lambda The wavelength the attenuation coefficient is required for
 * @returns Attenuation coefficient ie alpha in I/I0=exp(-alpha*distance)
 */
double
AttenuationProfile::getAttenuationCoefficient(const double lambda) const {
  return m_Interpolator.value(lambda);
}
} // namespace Kernel
} // namespace Mantid