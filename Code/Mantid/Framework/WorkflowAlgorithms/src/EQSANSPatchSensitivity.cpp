//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/EQSANSPatchSensitivity.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSPatchSensitivity)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void EQSANSPatchSensitivity::init() {
  declareProperty(new WorkspaceProperty<>("Workspace", "", Direction::InOut),
                  "Input sensitivity workspace to be patched");
  declareProperty(
      new WorkspaceProperty<>("PatchWorkspace", "", Direction::Input),
      "Workspace defining the patch. Masked detectors will be patched.");
  declareProperty("UseLinearRegression", true, "If true, a linear regression "
                                               "will be used instead of "
                                               "computing the average");
  declareProperty("OutputMessage", "", Direction::Output);
}

void EQSANSPatchSensitivity::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("Workspace");
  MatrixWorkspace_sptr patchWS = getProperty("PatchWorkspace");
  bool useRegression = getProperty("UseLinearRegression");
  const int nx_pixels = (int)(inputWS->getInstrument()->getNumberParameter(
      "number-of-x-pixels")[0]);
  const int ny_pixels = (int)(inputWS->getInstrument()->getNumberParameter(
      "number-of-y-pixels")[0]);

  const int numberOfSpectra = static_cast<int>(inputWS->getNumberHistograms());
  // Need to get hold of the parameter map
  Geometry::ParameterMap &pmap = inputWS->instrumentParameters();

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
      if (iDet > numberOfSpectra) {
        g_log.notice() << "Got an invalid detector ID " << iDet << std::endl;
        continue;
      }

      IDetector_const_sptr det = patchWS->getDetector(iDet);
      // If this detector is a monitor, skip to the next one
      if (det->isMonitor())
        continue;

      const MantidVec &YValues = inputWS->readY(iDet);
      const MantidVec &YErrors = inputWS->readE(iDet);

      // If this detector is masked, skip to the next one
      if (det->isMasked())
        patched_ids.push_back(iDet);
      else {
        IDetector_const_sptr sensitivityDet = inputWS->getDetector(iDet);
        if (!sensitivityDet->isMasked()) {
          double yPosition = det->getPos().Y();
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
      for (size_t k = 0; k < patched_ids.size(); k++) {
        const Geometry::ComponentID det =
            inputWS->getDetector(patched_ids[k])->getComponentID();
        try {
          if (det) {
            MantidVec &YValues = inputWS->dataY(patched_ids[k]);
            MantidVec &YErrors = inputWS->dataE(patched_ids[k]);
            if (useRegression) {
              YValues[0] = alpha + beta * det->getPos().Y();
              YErrors[0] = error;
            } else {
              YValues[0] = average;
              YErrors[0] = error;
            }

            pmap.addBool(det, "masked", false);
          }
        } catch (Kernel::Exception::NotFoundError &e) {
          g_log.warning() << e.what() << " Found while setting mask bit"
                          << std::endl;
        }
      }
    }
  }
  /*
  This rebuild request call, gives the workspace the opportunity to rebuild the
  nearest neighbours map
  and therefore pick up any detectors newly masked with this algorithm.
  */
  inputWS->rebuildNearestNeighbours();

  // Call Calculate efficiency to renormalize
  progress(0.91, "Renormalizing");
  IAlgorithm_sptr effAlg = createChildAlgorithm("CalculateEfficiency");
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
