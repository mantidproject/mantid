// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Interpolation.h"
#include <vector>

namespace Mantid {
namespace Kernel {

class MANTID_KERNEL_DLL AttenuationProfile {
public:
  AttenuationProfile(const std::string &inputFileName,
                     const std::string &searchPath);
  double getAttenuationCoefficient(const double lambda) const;

private:
  Kernel::Interpolation m_Interpolator;
};

} // namespace Kernel
} // namespace Mantid