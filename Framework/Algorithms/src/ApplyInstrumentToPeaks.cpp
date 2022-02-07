// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/ApplyInstrumentToPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid::Algorithms {

DECLARE_ALGORITHM(ApplyInstrumentToPeaks)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

void ApplyInstrumentToPeaks::init() {
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input peaks workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InstrumentWorkspace", "", Direction::Input,
                                                                 PropertyMode::Optional),
                  "Workspace from which the instrument will be copied from. If none is provided then the instrument on "
                  "the input workspace is used.");
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Output peaks workspace.");
}

void ApplyInstrumentToPeaks::exec() {
  /// Peak workspace to integrate
  PeaksWorkspace_sptr inputWS = getProperty("InputWorkspace");

  /// Output peaks workspace, create if needed
  PeaksWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS)
    outputWS = inputWS->clone();

  Mantid::Geometry::Instrument_const_sptr instrument;

  Workspace_sptr instWS = this->getProperty("InstrumentWorkspace");
  if (instWS) {
    ExperimentInfo_sptr ei = std::dynamic_pointer_cast<ExperimentInfo>(instWS);
    if (!ei)
      throw std::invalid_argument("Wrong type of workspace");
    instrument = ei->getInstrument();
    outputWS->setInstrument(instrument);
  } else {
    instrument = outputWS->getInstrument();
  }

  Units::Energy energyUnit;
  for (int i = 0; i < outputWS->getNumberPeaks(); ++i) {
    Peak &peak = outputWS->getPeak(i);

    const auto tof = peak.getTOF();

    peak.setInstrument(instrument);
    peak.setDetectorID(peak.getDetectorID());

    energyUnit.initialize(peak.getL1(), 0, {{UnitParams::l2, peak.getL2()}});
    const double energy = energyUnit.singleFromTOF(tof);
    peak.setInitialEnergy(energy);
    peak.setFinalEnergy(energy);
  }

  setProperty("OutputWorkspace", outputWS);
}
} // namespace Mantid::Algorithms
