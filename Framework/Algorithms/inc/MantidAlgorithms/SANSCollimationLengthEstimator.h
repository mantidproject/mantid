// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SANSCOLLISIONLENGTHESTIMATOR_H
#define MANTID_ALGORITHMS_SANSCOLLISIONLENGTHESTIMATOR_H

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/System.h"
/**Helper class which provides the Collimation Length for SANS instruments
 */
namespace Mantid {
namespace Algorithms {
class DLLExport SANSCollimationLengthEstimator {
public:
  double provideCollimationLength(Mantid::API::MatrixWorkspace_sptr workspace);

private:
  double getCollimationLengthWithGuides(
      Mantid::API::MatrixWorkspace_sptr inOutWS, const double L1,
      const double collimationLengthCorrection) const;
  double getGuideValue(Mantid::Kernel::Property *prop) const;
};
} // namespace Algorithms
} // namespace Mantid
#endif
