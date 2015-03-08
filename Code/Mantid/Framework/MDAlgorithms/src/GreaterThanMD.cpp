#include "MantidMDAlgorithms/GreaterThanMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GreaterThanMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
GreaterThanMD::GreaterThanMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
GreaterThanMD::~GreaterThanMD() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string GreaterThanMD::name() const { return "GreaterThanMD"; };

/// Algorithm's version for identification. @see Algorithm::version
int GreaterThanMD::version() const { return 1; };

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and operand
void GreaterThanMD::execHistoHisto(
    Mantid::DataObjects::MDHistoWorkspace_sptr out,
    Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) {
  out->greaterThan(*operand);
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and a scalar on the RHS
void GreaterThanMD::execHistoScalar(
    Mantid::DataObjects::MDHistoWorkspace_sptr out,
    Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) {
  out->greaterThan(scalar->dataY(0)[0]);
}

} // namespace Mantid
} // namespace MDAlgorithms
