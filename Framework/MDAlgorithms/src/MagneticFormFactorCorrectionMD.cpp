// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/MagneticFormFactorCorrectionMD.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace MDAlgorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using Mantid::Kernel::StringListValidator;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MagneticFormFactorCorrectionMD)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string MagneticFormFactorCorrectionMD::name() const { return "MagneticFormFactorCorrectionMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int MagneticFormFactorCorrectionMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MagneticFormFactorCorrectionMD::category() const { return "MDAlgorithms"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MagneticFormFactorCorrectionMD::summary() const {
  return "Apply magnetic form factor correction to MD events by dividing signal with F(Q)^2";
}
const std::vector<std::string> MagneticFormFactorCorrectionMD::seeAlso() const { return {"MagFormFactorCorrection"}; }
// //----------------------------------------------------------------------------------------------
// /** Initialize the algorithm's properties.
//  */
void MagneticFormFactorCorrectionMD::init() {
  QTransform::init();
  std::vector<std::string> keys = Mantid::PhysicalConstants::getMagneticIonList();
  declareProperty("IonName", "Cu2", std::make_shared<StringListValidator>(keys),
                  "The name of the ion: an element symbol with a number "
                  "indicating the valence, e.g. Fe2 for Fe2+ / Fe(II)");
}

// //----------------------------------------------------------------------------------------------
// /** Execute the algorithm.
//  */
void MagneticFormFactorCorrectionMD::exec() {
  ion = Mantid::PhysicalConstants::getMagneticIon(getProperty("IonName"));
  QTransform::exec();
}

// implement correction method
double MagneticFormFactorCorrectionMD::correction(const double q2) const {
  const auto ff = ion.analyticalFormFactor(q2);
  return 1. / (ff * ff);
}

} // namespace MDAlgorithms
} // namespace Mantid
