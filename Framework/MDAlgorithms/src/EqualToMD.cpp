// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/EqualToMD.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EqualToMD)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string EqualToMD::name() const { return "EqualToMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int EqualToMD::version() const { return 1; }

//----------------------------------------------------------------------------------------------
/// Extra properties
void EqualToMD::initExtraProperties() {
  declareProperty("Tolerance", 1e-5, "Tolerance when performing the == comparison. Default 10^-5.");
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and operand
void EqualToMD::execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                               Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) {
  double tolerance = getProperty("Tolerance");
  out->equalTo(*operand, tolerance);
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and a scalar on the RHS
void EqualToMD::execHistoScalar(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                                Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) {
  double tolerance = getProperty("Tolerance");
  out->equalTo(scalar->y(0)[0], tolerance);
}

} // namespace Mantid::MDAlgorithms
