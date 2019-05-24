// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MaskBinsFromWorkspace.h"
#include "MantidAPI/Algorithm.tcc"
#include "MantidAPI/HistogramValidator.h"


namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaskBinsFromWorkspace)

using namespace Kernel;
using namespace API;

void MaskBinsFromWorkspace::init() {
  declareWorkspaceInputProperties<MatrixWorkspace>(
      "InputWorkspace",
      "The name of the input workspace. Must contain histogram data.",
      boost::make_shared<HistogramValidator>());
  declareWorkspaceInputProperties<MatrixWorkspace>(
          "MaskedWorkspace",
          "The name of the workspaces containing masked bins to copy over. Must contain histogram data.",
          boost::make_shared<HistogramValidator>());

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the Workspace containing the masked bins.");
}

/** Execution code.
 */
void MaskBinsFromWorkspace::exec() {
  MatrixWorkspace_sptr inputWS;
  std::tie(inputWS, indexSet) = getWorkspaceAndIndices<MatrixWorkspace>("InputWorkspace");
  MatrixWorkspace_sptr maskedWS = getProperty("MaskedWorkspace");

  // Only create the output workspace if it's different to the input one
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
    setProperty("OutputWorkspace", outputWS);
  }

  // We assume that MaskedWorkspace contains a masked 0th spectra.
  // The masks flags attached to this spectrum are copied over to every spectrum
  // in the input workspace
  if (maskedWS->hasMaskedBins(0)) {
    const auto maskedBins = maskedWS->maskedBins(0);
    for (const auto &wi : indexSet) {
      for (const auto &maskedBin : maskedBins) {
        outputWS->flagMasked(wi, maskedBin.first, maskedBin.second);
      }
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
