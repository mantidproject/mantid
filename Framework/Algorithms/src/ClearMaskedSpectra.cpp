// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ClearMaskedSpectra.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ClearMaskedSpectra)

/// Algorithms name for identification. @see Algorithm::name
const std::string ClearMaskedSpectra::name() const {
  return "ClearMaskedSpectra";
}

/// Algorithm's version for identification. @see Algorithm::version
int ClearMaskedSpectra::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ClearMaskedSpectra::category() const {
  return "Transforms\\Masking";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ClearMaskedSpectra::summary() const {
  return "Clear counts and/or events in all fully masked spectra.";
}

void ClearMaskedSpectra::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(
      "InputWorkspace", "The input workspace", Direction::Input));
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                               Direction::Output),
      "Name of the output workspace (can be same as InputWorkspace)");
}

void ClearMaskedSpectra::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
    setProperty("OutputWorkspace", outputWS);
  }

  const auto &spectrumInfo = outputWS->spectrumInfo();
  for (size_t i = 0; i < spectrumInfo.size(); ++i)
    if (spectrumInfo.hasDetectors(i) && spectrumInfo.isMasked(i))
      outputWS->getSpectrum(i).clearData();

  if (auto event = dynamic_cast<DataObjects::EventWorkspace *>(outputWS.get()))
    event->clearMRU();
}

} // namespace Algorithms
} // namespace Mantid
