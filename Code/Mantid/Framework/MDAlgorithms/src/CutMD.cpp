#include "MantidMDAlgorithms/CutMD.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/System.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CutMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CutMD::CutMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CutMD::~CutMD() {}

//----------------------------------------------------------------------------------------------

void CutMD::init() {
  declareProperty(new WorkspaceProperty<IMDWorkspace>("InputWorkspace", "",
                                                      Direction::Input),
                  "MDWorkspace to slice");

  declareProperty(
      new WorkspaceProperty<ITableWorkspace>("Projection", "", Direction::Input,
                                             PropertyMode::Optional),
      "Projection");

  declareProperty(new ArrayProperty<double>("P1Bin"), "Projection 1 binning.");
  declareProperty(new ArrayProperty<double>("P2Bin"), "Projection 2 binning.");
  declareProperty(new ArrayProperty<double>("P3Bin"), "Projection 3 binning.");
  declareProperty(new ArrayProperty<double>("P4Bin"), "Projection 4 binning.");
  declareProperty(new ArrayProperty<double>("P5Bin"), "Projection 5 binning.");

  declareProperty(new WorkspaceProperty<IMDWorkspace>("OutputWorkspace", "",
                                                      Direction::Output),
                  "Output cut workspace");
  declareProperty("NoPix", false, "If False creates a full MDEventWorkspaces "
                                  "as output. True to create an "
                                  "MDHistoWorkspace as output. This is DND "
                                  "only in Horace terminology.");
  declareProperty("CheckAxes", true,
                  "Check that the axis look to be correct, and abort if not.");
}
void CutMD::exec() {}

} // namespace Mantid
} // namespace MDAlgorithms
