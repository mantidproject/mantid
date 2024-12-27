// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/PowerMD.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PowerMD)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string PowerMD::name() const { return "PowerMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int PowerMD::version() const { return 1; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/// Optional method to be subclassed to add properties
void PowerMD::initExtraProperties() {
  declareProperty("Exponent", 2.0, "Power to which to raise the values. Default 2.0.");
}

//----------------------------------------------------------------------------------------------
/// Check the inputs and throw if the algorithm cannot be run
void PowerMD::checkInputs() {
  if (!m_in_histo)
    throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm on a MDEventWorkspace
void PowerMD::execEvent(Mantid::API::IMDEventWorkspace_sptr /*out*/) {
  throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// PowerMD::Run the algorithm with a MDHistoWorkspace
void PowerMD::execHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out) {
  double exponent = getProperty("Exponent");
  out->power(exponent);
}

} // namespace Mantid::MDAlgorithms
