// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/OrMD.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(OrMD)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string OrMD::name() const { return "OrMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int OrMD::version() const { return 1; }

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and operand
void OrMD::execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                          Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) {
  out->operator|=(*operand);
}

} // namespace Mantid::MDAlgorithms
