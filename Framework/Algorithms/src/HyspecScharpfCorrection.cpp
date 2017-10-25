#include "MantidAlgorithms/HyspecScharpfCorrection.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HyspecScharpfCorrection)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string HyspecScharpfCorrection::name() const { return "HyspecScharpfCorrection"; }

/// Algorithm's version for identification. @see Algorithm::version
int HyspecScharpfCorrection::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string HyspecScharpfCorrection::category() const {
  return "CorrectionFunctions\\SpecialCorrections; Inelastic\\Corrections";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string HyspecScharpfCorrection::summary() const {
  return "Apply polarization factor as part of getting the spin incoherent scattering";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void HyspecScharpfCorrection::init() {
  auto wsValidator = boost::make_shared<Mantid::Kernel::CompositeValidator>();
  wsValidator->add<Mantid::API::WorkspaceUnitValidator>("DeltaE");
  wsValidator->add<Mantid::API::InstrumentValidator>();
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "",
                                                             Direction::Input, wsValidator),
      "An input workspace.");

  auto angleValidator = boost::make_shared<Mantid::Kernel::BoundedValidator<double>>();
  angleValidator->setLower(-180.0);
  angleValidator->setUpper(180.0);
  declareProperty("PolarizationAngle", EMPTY_DBL(), angleValidator,
                  "In plane angle between polatrization and incident beam"
                  "Must be between -180 and +180 degrees");
  auto precisionValidator = boost::make_shared<Mantid::Kernel::BoundedValidator<double>>();
  precisionValidator->setLower(0.0);
  precisionValidator->setUpper(1.0);
  declareProperty("Precision", 1e-5, precisionValidator, "If cosine of twice the "
                  "Scharpf angle is closer to 0 than the precision, the intensities "
                  "and errors will be set to 0");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "",
                                                             Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void HyspecScharpfCorrection::exec() {
    // Get the workspaces
    inputWS = this->getProperty("InputWorkspace");
    outputWS = this->getProperty("OutputWorkspace");
    angle = getProperty("PolarizationAngle");
    angle *=  M_PI / 180.;
    precision = getProperty("Precision");
    if (inputWS->run().hasProperty("Ei")) {
      Ei = boost::lexical_cast<double>(inputWS->run().getProperty("Ei")->value());
    } else {
      throw std::invalid_argument(
          "No Ei value has been set or stored within the run information.");
    }

    // Check if it is an event workspace
    Mantid::DataObjects::EventWorkspace_const_sptr eventW =
        boost::dynamic_pointer_cast<const Mantid::DataObjects::EventWorkspace>(inputWS);
    if (eventW != nullptr) {
      this->execEvent();
      return;
    }


    // If input and output workspaces are not the same, create a new workspace for
    // the output
    if (outputWS != inputWS) {
      outputWS = API::WorkspaceFactory::Instance().create(inputWS);
    }

    const auto &spectrumInfo = inputWS->spectrumInfo();

    // Get number of spectra in this workspace
    const int64_t numberOfSpectra = static_cast<int64_t>(inputWS->getNumberHistograms());
    Mantid::Kernel::V3D samplePos= spectrumInfo.samplePosition();
    API::Progress prog(this, 0.0, 1.0, numberOfSpectra);
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
    for (int64_t i = 0; i < numberOfSpectra; ++i) {
      PARALLEL_START_INTERUPT_REGION
      auto &yOut = outputWS->mutableY(i);
      auto &eOut = outputWS->mutableE(i);

      const auto &xIn = inputWS->points(i); // get the centers
      auto &yIn = inputWS->y(i);
      auto &eIn = inputWS->e(i);
      // Copy the energy transfer axis
      outputWS->setSharedX(i, inputWS->sharedX(i));

      prog.report();
      // continue if no detectors, if monitor, or is masked
      if ((!spectrumInfo.hasDetectors(i)) || spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i)) {
          continue;
      }
      //get detector info and calculate the in plane angle
      Mantid::Kernel::V3D detPos = spectrumInfo.position(i);
      double thetaPlane = std::atan2((detPos-samplePos).X(), (detPos-samplePos).Z());
      size_t spectrumSize = xIn.size();
      for(size_t j = 0; j < spectrumSize; ++j) {
          double factor = 0.;
          if(xIn[j] < Ei) {
            double kfki = std::sqrt(1. - xIn[j]/Ei); // k_f/k_i
            // angle between in plane Q and z axis
            double angleQ = std::atan2(kfki * std::sin(thetaPlane), 1. - kfki * std::cos(thetaPlane));
            // Scarpf agle = angle - angleQ
            factor = std::cos(2.*(angle - angleQ));
            //set intensity to 0 if the Scarpf angle is close to 45 degrees
            if(std::abs(factor) > precision){
                factor = 1./factor;
            }
            else {
                factor = 0.;
            }
          }
          yOut[j] = yIn[j] * factor;
          eOut[j] = eIn[j] * factor;
      }
      PARALLEL_END_INTERUPT_REGION
    } // end for i
    PARALLEL_CHECK_INTERUPT_REGION
  this->setProperty("OutputWorkspace", outputWS);
}

/** Execute for events
 */
void HyspecScharpfCorrection::execEvent() {
    g_log.notice()<<"events\n";
}

} // namespace Algorithms
} // namespace Mantid
