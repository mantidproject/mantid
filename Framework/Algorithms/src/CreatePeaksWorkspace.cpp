// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreatePeaksWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreatePeaksWorkspace)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

/** Initialize the algorithm's properties.
 */
void CreatePeaksWorkspace::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InstrumentWorkspace", "", Direction::Input,
                                                                 PropertyMode::Optional),
                  "An optional input workspace containing the default instrument for peaks "
                  "in this workspace.");
  declareProperty("NumberOfPeaks", 1, "Number of dummy peaks to initially create.");
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
  // explicit control of output peak workspace tyep
  // Full: standar peak workspace
  // Lean: LeanElasticPeakWorkspace
  const std::vector<std::string> peakworkspaceTypes{"Peak", "LeanElasticPeak"};
  declareProperty("OutputType", "Peak", std::make_shared<StringListValidator>(peakworkspaceTypes),
                  "Output peak workspace type, default to full peak workspace.");
}

/** Execute the algorithm.
 */
void CreatePeaksWorkspace::exec() {
  Workspace_sptr instWS = getProperty("InstrumentWorkspace");
  const std::string outputType = getProperty("OutputType");
  int NumberOfPeaks = getProperty("NumberOfPeaks");

  MultipleExperimentInfos_sptr instMDWS = std::dynamic_pointer_cast<MultipleExperimentInfos>(instWS);

  ExperimentInfo_sptr ei;

  IPeaksWorkspace_sptr out;
  // By default, we generate a PeakWorkspace unless user explicitly
  // requires a LeanElasticPeakWorkspace
  if (outputType == "Peak") {
    out = std::make_shared<PeaksWorkspace>();
    setProperty("OutputWorkspace", out);

    if (instMDWS != nullptr) {
      if (instMDWS->getNumExperimentInfo() > 0) {
        out->setInstrument(instMDWS->getExperimentInfo(0)->getInstrument());
        out->mutableRun().setGoniometer(instMDWS->getExperimentInfo(0)->run().getGoniometer().getR(), false);
      } else {
        throw std::invalid_argument("InstrumentWorkspace has no ExperimentInfo");
      }
    } else {
      ei = std::dynamic_pointer_cast<ExperimentInfo>(instWS);
      if (ei) {
        out->setInstrument(ei->getInstrument());
        out->mutableRun().setGoniometer(ei->run().getGoniometer().getR(), false);
      }
    }
    if (instMDWS || ei) {
      Progress progress(this, 0.0, 1.0, NumberOfPeaks);
      // Create some default Peaks
      for (int i = 0; i < NumberOfPeaks; i++) {
        out->addPeak(Peak(out->getInstrument(), out->getInstrument()->getDetectorIDs(true)[0], 1.0));
        progress.report();
      }
    }
  } else if (outputType == "LeanElasticPeak") {
    // use LeanElasticPeakWorkspace, which means no instrument related info
    out = std::make_shared<LeanElasticPeaksWorkspace>();
    setProperty("OutputWorkspace", out);

    if (instMDWS) {
      if (instMDWS->getNumExperimentInfo() > 0) {
        ei = std::dynamic_pointer_cast<ExperimentInfo>(instMDWS->getExperimentInfo(0));
      }
    } else {
      ei = std::dynamic_pointer_cast<ExperimentInfo>(instWS);
    }

    if (ei)
      out->copyExperimentInfoFrom(ei.get());

    Progress progress(this, 0.0, 1.0, NumberOfPeaks);
    for (int i = 0; i < NumberOfPeaks; i++) {
      out->addPeak(LeanElasticPeak());
      progress.report();
    }
  } else {
    throw std::invalid_argument("OutputType MUST be either Peak or LeanElasticPeak!");
  }
  // ALG END
}

} // namespace Mantid::Algorithms
