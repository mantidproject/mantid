#include "MantidMDAlgorithms/AndMD.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AndMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
AndMD::AndMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
AndMD::~AndMD() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string AndMD::name() const { return "AndMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int AndMD::version() const { return 1; }

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and operand
void
AndMD::execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                      Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) {
  out->operator&=(*operand);
}

} // namespace Mantid
} // namespace MDAlgorithms
