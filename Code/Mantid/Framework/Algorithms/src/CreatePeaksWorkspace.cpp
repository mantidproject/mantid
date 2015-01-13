#include "MantidAlgorithms/CreatePeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreatePeaksWorkspace)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CreatePeaksWorkspace::CreatePeaksWorkspace() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CreatePeaksWorkspace::~CreatePeaksWorkspace() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreatePeaksWorkspace::init() {
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>(
          "InstrumentWorkspace", "", Direction::Input, PropertyMode::Optional),
      "An optional input workspace containing the default instrument for peaks "
      "in this workspace.");
  declareProperty("NumberOfPeaks", 1,
                  "Number of dummy peaks to initially create.");
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreatePeaksWorkspace::exec() {
  MatrixWorkspace_sptr instWS = getProperty("InstrumentWorkspace");

  PeaksWorkspace_sptr out(new PeaksWorkspace());
  setProperty("OutputWorkspace", out);
  int NumberOfPeaks = getProperty("NumberOfPeaks");

  if (instWS) {
    out->setInstrument(instWS->getInstrument());
    // Create some default peaks
    for (int i = 0; i < NumberOfPeaks; i++) {
      out->addPeak(Peak(out->getInstrument(),
                        out->getInstrument()->getDetectorIDs(true)[0], 1.0));
    }
  }
}

} // namespace Mantid
} // namespace Algorithms
