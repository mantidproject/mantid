// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source,
//     Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Interpolation.h"
#include <vector>

namespace Mantid {
namespace Kernel {

class Material;

class MANTID_KERNEL_DLL AttenuationProfile {
public:
  AttenuationProfile(){};
  AttenuationProfile(const std::string &inputFileName, const std::string &searchPath,
                     Material *extrapolationMaterial = nullptr, double extrapolationMaxX = 100);
  double getAttenuationCoefficient(const double x) const;

  void setAttenuationCoefficient(const double x, const double atten);

private:
  Kernel::Interpolation m_Interpolator;
};

} // namespace Kernel
} // namespace Mantid
