// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreatePeaksWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/System.h"

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
      std::make_unique<WorkspaceProperty<Workspace>>(
          "InstrumentWorkspace", "", Direction::Input, PropertyMode::Optional),
      "An optional input workspace containing the default instrument for peaks "
      "in this workspace.");
  declareProperty("NumberOfPeaks", 1,
                  "Number of dummy peaks to initially create.");
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

/** Execute the algorithm.
 */
void CreatePeaksWorkspace::exec() {
  Workspace_sptr instWS = this->getProperty("InstrumentWorkspace");

  MultipleExperimentInfos_sptr instMDWS =
      boost::dynamic_pointer_cast<MultipleExperimentInfos>(instWS);

  ExperimentInfo_sptr ei;

  auto out = boost::make_shared<PeaksWorkspace>();
  setProperty("OutputWorkspace", out);
  int NumberOfPeaks = getProperty("NumberOfPeaks");

  if (instMDWS != nullptr) {
    if (instMDWS->getNumExperimentInfo() > 0) {
      out->setInstrument(instMDWS->getExperimentInfo(0)->getInstrument());
      out->mutableRun().setGoniometer(
          instMDWS->getExperimentInfo(0)->run().getGoniometer().getR(), false);
    } else {
      throw std::invalid_argument("InstrumentWorkspace has no ExperimentInfo");
    }
  } else {
    ei = boost::dynamic_pointer_cast<ExperimentInfo>(instWS);
    if (ei) {
      out->setInstrument(ei->getInstrument());
      out->mutableRun().setGoniometer(ei->run().getGoniometer().getR(), false);
    }
  }

  if (instMDWS || ei) {
    Progress progress(this, 0.0, 1.0, NumberOfPeaks);

    // Create some default peaks
    for (int i = 0; i < NumberOfPeaks; i++) {
      out->addPeak(Peak(out->getInstrument(),
                        out->getInstrument()->getDetectorIDs(true)[0], 1.0));
      progress.report();
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
