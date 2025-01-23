// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/LessThanMD.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LessThanMD)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LessThanMD::name() const { return "LessThanMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int LessThanMD::version() const { return 1; }

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and operand
void LessThanMD::execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                                Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) {
  out->lessThan(*operand);
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and a scalar on the RHS
void LessThanMD::execHistoScalar(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                                 Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) {
  out->lessThan(scalar->y(0)[0]);
}

} // namespace Mantid::MDAlgorithms
