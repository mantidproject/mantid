// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/Property.h"
/**Helper class which provides the Collimation Length for SANS instruments
 */
namespace Mantid {
namespace Algorithms {
class MANTID_ALGORITHMS_DLL SANSCollimationLengthEstimator {
public:
  double provideCollimationLength(const Mantid::API::MatrixWorkspace_sptr &workspace);

private:
  double getCollimationLengthWithGuides(const Mantid::API::MatrixWorkspace_sptr &inOutWS, const double L1,
                                        const double collimationLengthCorrection) const;
  double getGuideValue(const Mantid::Kernel::Property *prop) const;
};
} // namespace Algorithms
} // namespace Mantid
