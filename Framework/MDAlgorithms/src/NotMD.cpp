// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/NotMD.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(NotMD)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string NotMD::name() const { return "NotMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int NotMD::version() const { return 1; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/// Check the inputs and throw if the algorithm cannot be run
void NotMD::checkInputs() {
  if (!m_in_histo)
    throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm on a MDEventWorkspace
void NotMD::execEvent(Mantid::API::IMDEventWorkspace_sptr /*out*/) {
  throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// NotMD::Run the algorithm with a MDHistoWorkspace
void NotMD::execHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out) { out->operatorNot(); }

} // namespace Mantid::MDAlgorithms
