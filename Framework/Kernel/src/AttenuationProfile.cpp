// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source,
//     Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/AttenuationProfile.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Material.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>

namespace Mantid {
namespace Kernel {

namespace {
constexpr auto LARGE_LAMBDA = 100; // Lambda likely to be beyond max lambda in
                                   // any measured spectra. In Angstroms
}

/**
 * Construct an attenuation profile object
 * @param inputFileName :: The name of the file containing the attenuation
 * profile data. Filename can be a full path or just file name
 * @param searchPath :: Path to search for the input file
 * @param extrapolationMaterial :: Material whose properties will be used to
 * extrapolate beyond the lambda range of the supplied profile
 */
AttenuationProfile::AttenuationProfile(const std::string &inputFileName,
                                       const std::string &searchPath,
                                       Material *extrapolationMaterial) {
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
      double minLambda = std::numeric_limits<double>::max();
      double maxLambda = std::numeric_limits<double>::min();
      while (std::getline(input, line)) {
        double lambda, alpha, error;
        if (std::stringstream(line) >> lambda >> alpha >> error) {
          minLambda = std::min(lambda, minLambda);
          maxLambda = std::max(lambda, maxLambda);
          m_Interpolator.addPoint(lambda, 1000 * alpha);
        }
      }
      input.close();
      // Assist the extrapolation outside the supplied lambda range to better
      // handle noisy data. Add two surrounding points using the attenuation
      // calculated from tabulated absorption\scattering cross sections
      if (m_Interpolator.containData() && extrapolationMaterial) {
        if ((minLambda > 0) &&
            (minLambda < std::numeric_limits<double>::max())) {
          m_Interpolator.addPoint(
              0, extrapolationMaterial->attenuationCoefficient(0));
        }
        if ((maxLambda < LARGE_LAMBDA) &&
            (maxLambda > std::numeric_limits<double>::min())) {
          m_Interpolator.addPoint(
              LARGE_LAMBDA,
              extrapolationMaterial->attenuationCoefficient(LARGE_LAMBDA));
        }
      }
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
 * (Angstroms)
 * @returns Attenuation coefficient ie alpha in I/I0=exp(-alpha*distance)
 */
double
AttenuationProfile::getAttenuationCoefficient(const double lambda) const {
  return m_Interpolator.value(lambda);
}
} // namespace Kernel
} // namespace Mantid