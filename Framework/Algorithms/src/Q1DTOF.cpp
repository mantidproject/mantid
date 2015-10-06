//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Q1DTOF.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Histogram1D.h"
#include <iostream>
#include <vector>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Q1DTOF)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void Q1DTOF::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  wsValidator->add<InstrumentValidator>();
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                          Direction::Input, wsValidator));
  declareProperty(new WorkspaceProperty<>("CorrectionWorkspace", "",
                                          Direction::Input, wsValidator));
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output));
  declareProperty(
      new ArrayProperty<double>("OutputBinning", new RebinParamsValidator));
}

void Q1DTOF::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr corrWS = getProperty("CorrectionWorkspace");

  // Calculate the output binning
  const std::vector<double> binParams = getProperty("OutputBinning");
  // XOut defines the output histogram, so its length is equal to the number of
  // bins + 1
  MantidVecPtr XOut;
  const int sizeOut =
      VectorHelper::createAxisFromRebinParams(binParams, XOut.access());

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS =
      WorkspaceFactory::Instance().create(inputWS, 1, sizeOut, sizeOut - 1);
  outputWS->getAxis(0)->unit() =
      UnitFactory::Instance().create("MomentumTransfer");
  outputWS->setYUnitLabel("1/cm");
  setProperty("OutputWorkspace", outputWS);

  // Set the X vector for the output workspace
  outputWS->setX(0, XOut);
  MantidVec &YOut = outputWS->dataY(0);
  MantidVec &EOut = outputWS->dataE(0);

  const int numSpec = static_cast<int>(inputWS->getNumberHistograms());

  // Set up the progress reporting object
  Progress progress(this, 0.0, 1.0, numSpec);

  const V3D sourcePos = inputWS->getInstrument()->getSource()->getPos();
  const V3D samplePos = inputWS->getInstrument()->getSample()->getPos();

  const int xLength = static_cast<int>(inputWS->readX(0).size());
  const double fmp = 4.0 * M_PI;

  // Count histogram for normalization
  std::vector<double> XNorm(sizeOut - 1, 0.0);
  std::vector<double> ENorm(sizeOut - 1, 0.0);

  // Beam line axis, to compute scattering angle
  V3D beamLine = samplePos - sourcePos;

  PARALLEL_FOR3(inputWS, outputWS, corrWS)
  for (int i = 0; i < numSpec; i++) {
    PARALLEL_START_INTERUPT_REGION

    // Get the pixel relating to this spectrum
    IDetector_const_sptr det;
    try {
      det = corrWS->getDetector(i);
    } catch (Exception::NotFoundError &) {
      g_log.warning() << "Spectrum index " << i
                      << " has no detector assigned to it - discarding"
                      << std::endl;
      // Catch if no detector. Next line tests whether this happened - test
      // placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a
      // catch
      // in an openmp block.
    }
    // If no detector found or if it's masked or a monitor, skip onto the next
    // spectrum
    if (!det || det->isMonitor() || det->isMasked())
      continue;

    // Get the current spectrum for both input workspaces
    const MantidVec &XIn = inputWS->readX(i);
    const MantidVec &YIn = inputWS->readY(i);
    const MantidVec &EIn = inputWS->readE(i);
    // const MantidVec& XCorr = corrWS->readX(i);
    const MantidVec &YCorr = corrWS->readY(i);
    const MantidVec &ECorr = corrWS->readE(i);

    // Calculate the Q values for the current spectrum
    V3D pos = det->getPos();
    double sinTheta = sin(pos.angle(beamLine) / 2.0);
    double factor = fmp * sinTheta;

    // Loop over all xLength-1 detector channels
    // Note: xLength -1, because X is a histogram and has a number of boundaries
    // equal to the number of detector channels + 1.
    for (int j = 0; j < xLength - 1; j++) {
      double q = factor * 2.0 / (XIn[j] + XIn[j + 1]);
      int iq = 0;

      // Bin assignment depends on whether we have log or linear bins
      if (binParams[1] > 0.0) {
        iq = (int)floor((q - binParams[0]) / binParams[1]);
      } else {
        iq = (int)floor(log(q / binParams[0]) / log(1.0 - binParams[1]));
      }

      if (iq >= 0 && iq < sizeOut - 1 && YCorr[j] > 0.0) {

        PARALLEL_CRITICAL(iq) /* Write to shared memory - must protect */
        {
          YOut[iq] += YIn[j];
          EOut[iq] += EIn[j] * EIn[j];
          XNorm[iq] += 1.0 / YCorr[j];
          ENorm[iq] += 1.0 / (YCorr[j] * YCorr[j] * YCorr[j] * YCorr[j]) *
                       ECorr[j] * ECorr[j];
        }
      }
    }
    progress.report("Computing I(Q)");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Normalize according to the chosen weighting scheme
  for (int i = 0; i < sizeOut - 1; i++) {
    YOut[i] /= XNorm[i];
    EOut[i] =
        sqrt(EOut[i] + YOut[i] * YOut[i] / (XNorm[i] * XNorm[i]) * ENorm[i]) /
        XNorm[i];
  }
}

} // namespace Algorithms
} // namespace Mantid
