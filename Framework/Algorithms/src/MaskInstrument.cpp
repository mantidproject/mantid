// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MaskInstrument.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaskInstrument)

/// Algorithms name for identification. @see Algorithm::name
const std::string MaskInstrument::name() const { return "MaskInstrument"; }

/// Algorithm's version for identification. @see Algorithm::version
int MaskInstrument::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MaskInstrument::category() const {
  return "Transforms\\Masking";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MaskInstrument::summary() const {
  return "Mask detectors in the instrument WITHOUT clearing data in associated "
         "spectra.";
}

void MaskInstrument::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(
      "InputWorkspace", "The input workspace", Direction::Input));
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                            Direction::Output),
      "Name of the output workspace (can be same as InputWorkspace)");
  declareProperty(std::make_unique<ArrayProperty<detid_t>>("DetectorIDs"),
                  "List of detector IDs to mask");
}

void MaskInstrument::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
    setProperty("OutputWorkspace", outputWS);
  }

  const std::vector<detid_t> detectorIds = getProperty("DetectorIDs");
  auto &detectorInfo = outputWS->mutableDetectorInfo();
  for (const auto &id : detectorIds)
    detectorInfo.setMasked(detectorInfo.indexOf(id), true);
}

} // namespace Algorithms
} // namespace Mantid
