#include "MantidMDAlgorithms/ExponentialMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExponentialMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ExponentialMD::ExponentialMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ExponentialMD::~ExponentialMD() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ExponentialMD::name() const { return "ExponentialMD"; };

/// Algorithm's version for identification. @see Algorithm::version
int ExponentialMD::version() const { return 1; };

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/// Check the inputs and throw if the algorithm cannot be run
void ExponentialMD::checkInputs() {
  if (!m_in_histo)
    throw std::runtime_error(this->name() +
                             " can only be run on a MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm on a MDEventWorkspace
void ExponentialMD::execEvent(Mantid::API::IMDEventWorkspace_sptr /*out*/) {
  throw std::runtime_error(this->name() +
                           " can only be run on a MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// ExponentialMD::Run the algorithm with a MDHistoWorkspace
void ExponentialMD::execHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out) {
  out->exp();
}

} // namespace Mantid
} // namespace MDAlgorithms
