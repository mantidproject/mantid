#include "MantidAlgorithms/HyspecScharpfCorrection.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"


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
    if (inputWS->run().hasProperty("Ei")) {
      Ei = boost::lexical_cast<double>(inputWS->run().getProperty("Ei").value());
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

    // Get number of spectra in this workspace
    const int numberOfSpectra = inputWS->getNumberHistograms();

    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
    for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i) {
      PARALLEL_START_INTERUPT_REGION
      auto &yOut = outputWS->mutableY(i);
      auto &eOut = outputWS->mutableE(i);
      auto &xout = outputWS->mutableX(i);
      const auto &xIn = inputWS->points(i);
      auto &yIn = inputWS->y(i);
      auto &eIn = inputWS->e(i);

      prog.report();
      PARALLEL_END_INTERUPT_REGION
    } // end for i
    PARALLEL_CHECK_INTERUPT_REGION

}

/** Execute for events
 */
void HyspecScharpfCorrection::execEvent() {
    g_log.notice()<<"events\n";
}

} // namespace Algorithms
} // namespace Mantid
