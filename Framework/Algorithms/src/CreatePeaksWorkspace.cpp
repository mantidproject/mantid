#include "MantidAlgorithms/CreatePeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreatePeaksWorkspace)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

/** Initialize the algorithm's properties.
 */
void CreatePeaksWorkspace::init() {
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InstrumentWorkspace", "", Direction::Input, PropertyMode::Optional),
      "An optional input workspace containing the default instrument for peaks "
      "in this workspace.");
  declareProperty("NumberOfPeaks", 1,
                  "Number of dummy peaks to initially create.");
  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

/** Execute the algorithm.
 */
void CreatePeaksWorkspace::exec() {
  MatrixWorkspace_sptr instWS = getProperty("InstrumentWorkspace");

  auto out = boost::make_shared<PeaksWorkspace>();
  setProperty("OutputWorkspace", out);
  int NumberOfPeaks = getProperty("NumberOfPeaks");

  if (instWS) {
    Progress progress(this, 0.0, 1.0, NumberOfPeaks);

    out->setInstrument(instWS->getInstrument());
    out->mutableRun().setGoniometer(instWS->run().getGoniometer().getR(),
                                    false);
    // Create some default peaks
    for (int i = 0; i < NumberOfPeaks; i++) {
      out->addPeak(Peak(out->getInstrument(),
                        out->getInstrument()->getDetectorIDs(true)[0], 1.0));
      progress.report();
    }
  }
}

} // namespace Mantid
} // namespace Algorithms
