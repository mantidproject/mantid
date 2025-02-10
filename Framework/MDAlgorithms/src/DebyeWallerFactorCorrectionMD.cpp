// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/DebyeWallerFactorCorrectionMD.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace MDAlgorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyWithValue;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DebyeWallerFactorCorrectionMD)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string DebyeWallerFactorCorrectionMD::name() const { return "DebyeWallerFactorCorrectionMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int DebyeWallerFactorCorrectionMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DebyeWallerFactorCorrectionMD::category() const { return "MDAlgorithms"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string DebyeWallerFactorCorrectionMD::summary() const {
  return "Apply Debye-Waller factor correction to MD events by dividing signal with exp(-q2 * u2 / 3)";
}
const std::vector<std::string> DebyeWallerFactorCorrectionMD::seeAlso() const { return {"DebyWallerFactorCorrection"}; }
// //----------------------------------------------------------------------------------------------
// /** Initialize the algorithm's properties.
//  */
void DebyeWallerFactorCorrectionMD::init() {
  QTransform::init();
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty(std::make_unique<PropertyWithValue<double>>("MeanSquaredDisplacement", EMPTY_DBL(),
                                                              std::move(mustBePositive), Direction::Input),
                  "Mandatory: Mean squared displacement <u^2>. Often obtained from Rietveld refinement.");
}

// //----------------------------------------------------------------------------------------------
// /** Execute the algorithm.
//  */
void DebyeWallerFactorCorrectionMD::exec() { QTransform::exec(); }

// implement correction method
double DebyeWallerFactorCorrectionMD::correction(const double q2) const {
  const double u2 = getProperty("MeanSquaredDisplacement");
  const double inverse_DWF = exp(u2 * q2 / 3.0);
  return inverse_DWF;
}

} // namespace MDAlgorithms
} // namespace Mantid
