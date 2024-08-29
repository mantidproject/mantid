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
#include <Poco/Path.h>
#include <filesystem>
#include <fstream>

namespace Mantid::Kernel {

/**
 * Construct an attenuation profile object
 * @param inputFileName :: The name of the file containing the attenuation
 * profile data. Filename can be a full path or just file name
 * @param searchPath :: Path to search for the input file
 * @param extrapolationMaxX :: X value at which a point will be inserted
 * from the tabulated attenuation data to assist with extrapolation
 * @param extrapolationMaterial :: Material whose properties will be used to
 * extrapolate beyond the lambda range of the supplied profile
 */
AttenuationProfile::AttenuationProfile(const std::string &inputFileName, const std::string &searchPath,
                                       Material *extrapolationMaterial, double extrapolationMaxX) {
  Poco::Path suppliedFileName(inputFileName);
  Poco::Path inputFilePath;
  std::string fileExt = suppliedFileName.getExtension();
  std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), toupper);

  if (fileExt == "DAT") {
    if (suppliedFileName.isRelative()) {
      bool useSearchDirectories = true;

      if (!searchPath.empty()) {
        inputFilePath = Poco::Path(Poco::Path(searchPath).parent(), inputFileName);
        if (std::filesystem::exists(inputFilePath.toString())) {
          useSearchDirectories = false;
        }
      }
      if (useSearchDirectories) {
        // ... and if that doesn't work look in the search directories
        std::string foundFile = Mantid::Kernel::ConfigService::Instance().getFullPath(inputFileName, false, 0);
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
      double minX = std::numeric_limits<double>::max();
      double maxX = std::numeric_limits<double>::lowest();
      while (std::getline(input, line)) {
        double x, alpha, error;
        if (std::stringstream(line) >> x >> alpha >> error) {
          minX = std::min(x, minX);
          maxX = std::max(x, maxX);
          m_Interpolator.addPoint(x, 1000 * alpha);
        }
      }
      input.close();
      // Assist the extrapolation outside the supplied x range to better
      // handle noisy data. Add two surrounding points using the attenuation
      // calculated from tabulated absorption\scattering cross sections
      if (m_Interpolator.containData() && extrapolationMaterial) {
        if ((minX > 0) && (minX < std::numeric_limits<double>::max())) {
          m_Interpolator.addPoint(0, extrapolationMaterial->attenuationCoefficient(0));
        }
        if ((maxX < extrapolationMaxX) && (maxX > std::numeric_limits<double>::lowest())) {
          m_Interpolator.addPoint(extrapolationMaxX, extrapolationMaterial->attenuationCoefficient(extrapolationMaxX));
        }
      }
    } else {
      throw Exception::FileError("Error reading attenuation profile file", inputFileName);
    }
  } else {
    throw Exception::FileError("Attenuation profile file must be a .DAT file", inputFileName);
  }
}

/**
 * Returns the attenuation coefficient for the supplied x value
 * @param x The x value (typically wavelength) that the attenuation coefficient
 * is required for (Angstroms)
 * @returns Attenuation coefficient ie alpha in I/I0=exp(-alpha*distance)
 */
double AttenuationProfile::getAttenuationCoefficient(const double x) const { return m_Interpolator.value(x); }
/**
 * Set the attenuation coefficient at x value
 * @param x The x value (typically wavelength) that the attenuation coefficient
 * is required for (Angstroms)
 * @param atten attenuation coefficient at x value
 */
void AttenuationProfile::setAttenuationCoefficient(const double x, const double atten) {
  m_Interpolator.addPoint(x, atten);
}
} // namespace Mantid::Kernel
