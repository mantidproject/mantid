#include "MantidMDAlgorithms/XorMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(XorMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
XorMD::XorMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
XorMD::~XorMD() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string XorMD::name() const { return "XorMD"; };

/// Algorithm's version for identification. @see Algorithm::version
int XorMD::version() const { return 1; };

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and operand
void
XorMD::execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                      Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) {
  out->operator^=(*operand);
}

} // namespace Mantid
} // namespace MDAlgorithms
