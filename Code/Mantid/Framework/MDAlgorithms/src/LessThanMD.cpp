#include "MantidMDAlgorithms/LessThanMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LessThanMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LessThanMD::LessThanMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LessThanMD::~LessThanMD() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LessThanMD::name() const { return "LessThanMD"; };

/// Algorithm's version for identification. @see Algorithm::version
int LessThanMD::version() const { return 1; };

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and operand
void LessThanMD::execHistoHisto(
    Mantid::MDEvents::MDHistoWorkspace_sptr out,
    Mantid::MDEvents::MDHistoWorkspace_const_sptr operand) {
  out->lessThan(*operand);
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and a scalar on the RHS
void LessThanMD::execHistoScalar(
    Mantid::MDEvents::MDHistoWorkspace_sptr out,
    Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) {
  out->lessThan(scalar->dataY(0)[0]);
}

} // namespace Mantid
} // namespace MDAlgorithms
