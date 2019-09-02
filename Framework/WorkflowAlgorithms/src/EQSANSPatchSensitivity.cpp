// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/EQSANSPatchSensitivity.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSPatchSensitivity)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void EQSANSPatchSensitivity::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("Workspace", "", Direction::InOut),
      "Input sensitivity workspace to be patched");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("PatchWorkspace", "",
                                            Direction::Input),
      "Workspace defining the patch. Masked detectors will be patched.");
  declareProperty("UseLinearRegression", true,
                  "If true, a linear regression "
                  "will be used instead of "
                  "computing the average");
  declareProperty("OutputMessage", "", Direction::Output);
}

void EQSANSPatchSensitivity::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("Workspace");
  MatrixWorkspace_sptr patchWS = getProperty("PatchWorkspace");
  bool useRegression = getProperty("UseLinearRegression");
  const int nx_pixels = static_cast<int>(
      inputWS->getInstrument()->getNumberParameter("number-of-x-pixels")[0]);
  const int ny_pixels = static_cast<int>(
      inputWS->getInstrument()->getNumberParameter("number-of-y-pixels")[0]);

  const auto numberOfSpectra = static_cast<int>(inputWS->getNumberHistograms());

  auto &inSpectrumInfo = inputWS->mutableSpectrumInfo();
  const auto &spectrumInfo = patchWS->spectrumInfo();
  // Loop over all tubes and patch as necessary
  for (int i = 0; i < nx_pixels; i++) {
    std::vector<int> patched_ids;
    int nUnmasked = 0;
    double totalUnmasked = 0.0;
    double errorUnmasked = 0.0;

    double sumXY = 0.0;
    double sumX = 0.0;
    double sumX2 = 0.0;
    double sumY = 0.0;

    progress(0.9 * i / nx_pixels, "Processing patch");

    for (int j = 0; j < ny_pixels; j++) {
      // EQSANS-specific: get detector ID from pixel coordinates
      int iDet = ny_pixels * i + j;
      if (iDet >= numberOfSpectra) {
        g_log.notice() << "Got an invalid detector ID " << iDet << '\n';
        continue;
      }

      // If this detector is a monitor, skip to the next one
      if (spectrumInfo.isMonitor(iDet))
        continue;

      const MantidVec &YValues = inputWS->readY(iDet);
      const MantidVec &YErrors = inputWS->readE(iDet);

      // If this detector is masked, skip to the next one
      if (spectrumInfo.isMasked(iDet))
        patched_ids.push_back(iDet);
      else {
        if (!inSpectrumInfo.isMasked(iDet)) {
          double yPosition = spectrumInfo.position(iDet).Y();
          totalUnmasked += YErrors[0] * YErrors[0] * YValues[0];
          errorUnmasked += YErrors[0] * YErrors[0];
          nUnmasked++;

          sumXY += yPosition * YValues[0];
          sumX += yPosition;
          sumX2 += yPosition * yPosition;
          sumY += YValues[0];
        }
      }
    }

    if (nUnmasked > 0 && errorUnmasked > 0) {
      sumXY /= nUnmasked;
      sumX /= nUnmasked;
      sumX2 /= nUnmasked;
      sumY /= nUnmasked;
      double beta = (sumXY - sumX * sumY) / (sumX2 - sumX * sumX);
      double alpha = sumY - beta * sumX;
      double error = sqrt(errorUnmasked) / nUnmasked;
      double average = totalUnmasked / errorUnmasked;

      // Apply patch
      progress(0.91, "Applying patch");
      for (auto patched_id : patched_ids) {
        if (!inSpectrumInfo.hasDetectors(patched_id)) {
          g_log.warning() << "Spectrum " << patched_id
                          << " has no detector, skipping (not clearing mask)\n";
          continue;
        }
        MantidVec &YValues = inputWS->dataY(patched_id);
        MantidVec &YErrors = inputWS->dataE(patched_id);
        if (useRegression) {
          YValues[0] = alpha + beta * inSpectrumInfo.position(patched_id).Y();
          YErrors[0] = error;
        } else {
          YValues[0] = average;
          YErrors[0] = error;
        }
        inSpectrumInfo.setMasked(patched_id, false);
      }
    }
  }

  // Call Calculate efficiency to renormalize
  progress(0.91, "Renormalizing");
  IAlgorithm_sptr effAlg =
      createChildAlgorithm("CalculateEfficiency", 0.91, 1, true, 1);
  effAlg->setProperty("InputWorkspace", inputWS);
  effAlg->setProperty("OutputWorkspace", inputWS);
  effAlg->execute();
  inputWS = effAlg->getProperty("OutputWorkspace");
  setProperty("Workspace", inputWS);

  setProperty("OutputMessage",
              "Applied wavelength-dependent sensitivity correction");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
