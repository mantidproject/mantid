//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ApplyTransmissionCorrection.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyTransmissionCorrection)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void ApplyTransmissionCorrection::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                          Direction::Input, wsValidator),
                  "Workspace to apply the transmission correction to");
  declareProperty(new WorkspaceProperty<>("TransmissionWorkspace", "",
                                          Direction::Output,
                                          PropertyMode::Optional),
                  "Workspace containing the transmission values [optional]");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Workspace to store the corrected data in");

  // Alternatively, the user can specify a transmission that will ba applied to
  // all wavelength bins
  declareProperty(
      "TransmissionValue", EMPTY_DBL(),
      "Transmission value to apply to all wavelengths. If specified, "
      "TransmissionWorkspace will not be used.");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("TransmissionError", 0.0, mustBePositive,
                  "The error on the transmission value (default 0.0)");

  declareProperty(
      "ThetaDependent", true,
      "If true, a theta-dependent transmission correction will be applied.");
}

void ApplyTransmissionCorrection::exec() {
  // Check whether we only need to divided the workspace by
  // the transmission.
  const bool thetaDependent = getProperty("ThetaDependent");

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const double trans_value = getProperty("TransmissionValue");
  const double trans_error = getProperty("TransmissionError");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  MantidVec trans(inputWS->readY(0).size(), trans_value);
  MantidVec dtrans(inputWS->readY(0).size(), trans_error);
  MantidVec &TrIn = trans;
  MantidVec &ETrIn = dtrans;

  if (isEmpty(trans_value)) {
    // Get the transmission workspace
    MatrixWorkspace_const_sptr transWS = getProperty("TransmissionWorkspace");

    // Check that the two input workspaces are consistent (same number of X
    // bins)
    if (transWS->readY(0).size() != inputWS->readY(0).size()) {
      g_log.error() << "Input and transmission workspaces have a different "
                       "number of wavelength bins" << std::endl;
      throw std::invalid_argument("Input and transmission workspaces have a "
                                  "different number of wavelength bins");
    }

    TrIn = transWS->readY(0);
    ETrIn = transWS->readE(0);
  }

  const int numHists = static_cast<int>(inputWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, numHists);

  // Create a Workspace2D to match the intput workspace
  MatrixWorkspace_sptr corrWS = WorkspaceFactory::Instance().create(inputWS);

  // Loop through the spectra and apply correction
  PARALLEL_FOR2(inputWS, corrWS)
  for (int i = 0; i < numHists; i++) {
    PARALLEL_START_INTERUPT_REGION

    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
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
    // If no detector found, skip onto the next spectrum
    if (!det)
      continue;

    // Copy over the X data
    corrWS->dataX(i) = inputWS->readX(i);

    // Skip if we have a monitor or if the detector is masked.
    if (det->isMonitor() || det->isMasked())
      continue;

    // Compute theta-dependent transmission term for each wavelength bin
    MantidVec &YOut = corrWS->dataY(i);
    MantidVec &EOut = corrWS->dataE(i);

    const double exp_term =
        (1.0 / cos(inputWS->detectorTwoTheta(det)) + 1.0) / 2.0;
    for (int j = 0; j < (int)inputWS->readY(0).size(); j++) {
      if (!thetaDependent) {
        YOut[j] = 1.0 / TrIn[j];
        EOut[j] = std::fabs(ETrIn[j] * TrIn[j] * TrIn[j]);
      } else {
        EOut[j] = std::fabs(ETrIn[j] * exp_term / pow(TrIn[j], exp_term + 1.0));
        YOut[j] = 1.0 / pow(TrIn[j], exp_term);
      }
    }

    progress.report("Applying Transmission Correction");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  outputWS = inputWS * corrWS;
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
