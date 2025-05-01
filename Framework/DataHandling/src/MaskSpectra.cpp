// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/MaskSpectra.h"
#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"

namespace Mantid::DataHandling {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaskSpectra)

/// Algorithms name for identification. @see Algorithm::name
const std::string MaskSpectra::name() const { return "MaskSpectra"; }

/// Algorithm's version for identification. @see Algorithm::version
int MaskSpectra::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MaskSpectra::category() const { return "Transforms\\Masking"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MaskSpectra::summary() const {
  return "Mask (zero) spectra and the underlying detectors in a workspace.";
}

void MaskSpectra::init() {
  declareWorkspaceInputProperties<MatrixWorkspace, static_cast<int>(IndexType::SpectrumNum) |
                                                       static_cast<int>(IndexType::WorkspaceIndex)>(
      "InputWorkspace", "The input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace");
}

void MaskSpectra::exec() {
  std::shared_ptr<MatrixWorkspace> inputWS;
  Indexing::SpectrumIndexSet indexSet;
  std::tie(inputWS, indexSet) = getWorkspaceAndIndices<MatrixWorkspace>("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
    setProperty("OutputWorkspace", outputWS);
  }

  auto &spectrumInfo = outputWS->mutableSpectrumInfo();
  Progress prog(this, 0.0, 1.0, indexSet.size());
  for (const auto i : indexSet) {
    outputWS->getSpectrum(i).clearData();
    if (spectrumInfo.hasDetectors(i))
      spectrumInfo.setMasked(i, true);
    prog.report();
  }
}

} // namespace Mantid::DataHandling
