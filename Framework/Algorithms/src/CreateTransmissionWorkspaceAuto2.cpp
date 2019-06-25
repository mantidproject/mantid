// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateTransmissionWorkspaceAuto2.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/BoostOptionalToAlgorithmProperty.h"
#include "MantidAlgorithms/ReflectometryWorkflowBase2.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateTransmissionWorkspaceAuto2)

//----------------------------------------------------------------------------------------------
/// Sets documentation strings for this algorithm
const std::string CreateTransmissionWorkspaceAuto2::summary() const {
  return "Creates a transmission run workspace in Wavelength from input TOF "
         "workspaces.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateTransmissionWorkspaceAuto2::init() {

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "FirstTransmissionRun", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("TOF")),
                  "Input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "SecondTransmissionRun", "", Direction::Input,
                      PropertyMode::Optional,
                      boost::make_shared<WorkspaceUnitValidator>("TOF")),
                  "Second transmission run workspace in TOF.");

  // Analysis mode
  const std::vector<std::string> analysisMode{"PointDetectorAnalysis",
                                              "MultiDetectorAnalysis"};
  auto analysisModeValidator =
      boost::make_shared<StringListValidator>(analysisMode);
  declareProperty("AnalysisMode", analysisMode[0], analysisModeValidator,
                  "Analysis mode. This property is only used when "
                  "ProcessingInstructions is not set.",
                  Direction::Input);

  // Processing instructions
  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "ProcessingInstructions", "", Direction::Input),
                  "Grouping pattern of spectrum numbers to yield only the"
                  " detectors of interest. See GroupDetectors for syntax.");

  // Wavelength range
  declareProperty("WavelengthMin", Mantid::EMPTY_DBL(),
                  "Wavelength Min in angstroms", Direction::Input);
  declareProperty("WavelengthMax", Mantid::EMPTY_DBL(),
                  "Wavelength Max in angstroms", Direction::Input);

  // Monitor properties
  initMonitorProperties();

  // Properties for stitching transmission runs
  initStitchProperties();

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output transmission workspace in wavelength.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateTransmissionWorkspaceAuto2::exec() {

  IAlgorithm_sptr alg = createChildAlgorithm("CreateTransmissionWorkspace");
  alg->initialize();

  // First transmission run
  MatrixWorkspace_sptr firstWS = getProperty("FirstTransmissionRun");

  // Transmission properties
  populateTransmissionProperties(alg);

  // Instrument
  auto instrument = firstWS->getInstrument();

  // Other mandatory properties
  double wavMin = checkForMandatoryInstrumentDefault<double>(
      this, "WavelengthMin", instrument, "LambdaMin");
  alg->setProperty("WavelengthMin", wavMin);
  double wavMax = checkForMandatoryInstrumentDefault<double>(
      this, "WavelengthMax", instrument, "LambdaMax");
  alg->setProperty("WavelengthMax", wavMax);

  // Monitor properties
  populateMonitorProperties(alg, instrument);

  // Processing instructions
  convertProcessingInstructions(instrument, firstWS);
  alg->setProperty("ProcessingInstructions", m_processingInstructions);

  alg->execute();
  MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

  setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
