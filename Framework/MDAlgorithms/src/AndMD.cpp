// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/AndMD.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AndMD)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string AndMD::name() const { return "AndMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int AndMD::version() const { return 1; }

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and operand
void AndMD::execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                           Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) {
  out->operator&=(*operand);
}

} // namespace MDAlgorithms
} // namespace Mantid
