//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ApplyTransmissionCorrection.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"

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
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "Workspace to apply the transmission correction to");
  declareProperty(make_unique<WorkspaceProperty<>>("TransmissionWorkspace", "",
                                                   Direction::Output,
                                                   PropertyMode::Optional),
                  "Workspace containing the transmission values [optional]");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
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

class ApplyTransmissionCorrectionToSpectrum {
public:
  ApplyTransmissionCorrectionToSpectrum(const std::vector<double> &TrIn,
                                        const std::vector<double> &ETrIn,
                                        const bool thetaDependent)
      : m_TrIn(TrIn), m_ETrIn(ETrIn), m_thetaDependent(thetaDependent) {}

  void operator()(const std::vector<double> &inX, bool isMonitor, bool isMasked,
                  double twoTheta, std::vector<double> &outX,
                  std::vector<double> &outY, std::vector<double> &outE) const {
    // Copy over the X data
    outX = inX;

    // Skip if we have a monitor or if the detector is masked.
    if (isMonitor || isMasked)
      return;

    // Compute theta-dependent transmission term for each wavelength bin
    const double exp_term = (1.0 / cos(twoTheta) + 1.0) / 2.0;
    for (int j = 0; j < static_cast<int>(inX.size() - 1); j++) {
      if (!m_thetaDependent) {
        outY[j] = 1.0 / m_TrIn[j];
        outE[j] = std::fabs(m_ETrIn[j] * m_TrIn[j] * m_TrIn[j]);
      } else {
        outE[j] =
            std::fabs(m_ETrIn[j] * exp_term / pow(m_TrIn[j], exp_term + 1.0));
        outY[j] = 1.0 / pow(m_TrIn[j], exp_term);
      }
    }
  }

private:
  const std::vector<double> &m_TrIn;
  const std::vector<double> &m_ETrIn;
  const bool m_thetaDependent;
};

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

  // Create a Workspace2D to match the intput workspace
  MatrixWorkspace_sptr corrWS = WorkspaceFactory::Instance().create(inputWS);

  ApplyTransmissionCorrectionToSpectrum apply(TrIn, ETrIn, thetaDependent);
  this->for_each(
      *inputWS, std::make_tuple(Getters::constX),
      std::make_tuple(Getters::isMonitor, Getters::isMasked, Getters::twoTheta),
      *corrWS, std::make_tuple(Getters::x, Getters::y, Getters::e), apply);

  outputWS = inputWS * corrWS;
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
