// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/LogarithmMD.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LogarithmMD)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LogarithmMD::name() const { return "LogarithmMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int LogarithmMD::version() const { return 1; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/// Optional method to be subclassed to add properties
void LogarithmMD::initExtraProperties() {
  declareProperty("Filler", 0.0,
                  "Some values in a workspace can normally be "
                  "zeros or may get negative values after "
                  "transformations\n"
                  "log(x) is not defined for such values, so "
                  "here is the value, that will be placed as "
                  "the result of log(x<=0) operation\n"
                  "Default value is 0");
  declareProperty("Natural", true,
                  "Switch to choose between natural or base "
                  "10 logarithm. Default true (natural).");
}

//----------------------------------------------------------------------------------------------
/// Check the inputs and throw if the algorithm cannot be run
void LogarithmMD::checkInputs() {
  if (!m_in_histo)
    throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm on a MDEventWorkspace
void LogarithmMD::execEvent(Mantid::API::IMDEventWorkspace_sptr /*out*/) {
  throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// LogarithmMD::Run the algorithm with a MDHistoWorkspace
void LogarithmMD::execHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out) {
  bool natural = getProperty("Natural");
  double filler = getProperty("Filler");
  if (natural)
    out->log(filler);
  else
    out->log10(filler);
}

} // namespace Mantid::MDAlgorithms
