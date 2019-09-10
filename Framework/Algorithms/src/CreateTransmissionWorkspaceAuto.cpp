// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*WIKI*
 Facade over [[CreateTransmissionWorkspace]]. Pull numeric parameters out of the
 instrument parameters where possible. You can override any of these
 automatically
 applied defaults by providing your own value for the input.

 See [[CreateTransmissionWorkspace]] for more information on the wrapped
 algorithm.
 *WIKI*/

#include "MantidAlgorithms/CreateTransmissionWorkspaceAuto.h"
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/RebinParamsValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateTransmissionWorkspaceAuto)

//----------------------------------------------------------------------------------------------
/// Sets documentation strings for this algorithm
const std::string CreateTransmissionWorkspaceAuto::summary() const {
  return "Creates a transmission run workspace in Wavelength from input TOF "
         "workspaces.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateTransmissionWorkspaceAuto::init() {

  std::vector<std::string> analysis_modes;
  analysis_modes.push_back("PointDetectorAnalysis");
  analysis_modes.push_back("MultiDetectorAnalysis");
  declareProperty("AnalysisMode", analysis_modes.at(0),
                  boost::make_shared<StringListValidator>(analysis_modes),
                  "Analysis Mode to Choose", Direction::Input);

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "FirstTransmissionRun", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("TOF")),
                  "Input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "SecondTransmissionRun", "", Direction::Input,
                      PropertyMode::Optional,
                      boost::make_shared<WorkspaceUnitValidator>("TOF")),
                  "Second transmission run workspace in TOF.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output transmission workspace in wavelength.");

  declareProperty(
      std::make_unique<ArrayProperty<double>>(
          "Params", boost::make_shared<RebinParamsValidator>(true)),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "These parameters are used for stitching together transmission runs. "
      "Values are in wavelength (angstroms). This input is only needed if a "
      "SecondTransmission run is provided.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "Start wavelength for stitching transmission runs together");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>(
          "EndOverlap", Mantid::EMPTY_DBL(), Direction::Input),
      "End wavelength (angstroms) for stitching transmission runs together");
  declareProperty(std::make_unique<PropertyWithValue<int>>(
                      "I0MonitorIndex", Mantid::EMPTY_INT(), Direction::Input),
                  "I0 monitor workspace index. Optional.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "ProcessingInstructions", "", Direction::Input),
                  "Grouping pattern on workspace indexes to yield only "
                  "the detectors of interest. See GroupDetectors for details.");

  declareProperty("WavelengthMin", Mantid::EMPTY_DBL(),
                  "Wavelength Min in angstroms", Direction::Input);
  declareProperty("WavelengthMax", Mantid::EMPTY_DBL(),
                  "Wavelength Max in angstroms", Direction::Input);
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MonitorBackgroundWavelengthMin", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Monitor wavelength background min in angstroms");
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MonitorBackgroundWavelengthMax", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Monitor wavelength background max in angstroms");
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MonitorIntegrationWavelengthMin", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Monitor integral min in angstroms");
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MonitorIntegrationWavelengthMax", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Monitor integral max in angstroms");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateTransmissionWorkspaceAuto::exec() {
  MatrixWorkspace_sptr firstWS = this->getProperty("FirstTransmissionRun");
  auto instrument = firstWS->getInstrument();

  // Get all the inputs.

  std::string outputWorkspaceName = this->getPropertyValue("OutputWorkspace");
  std::string analysis_mode = this->getPropertyValue("AnalysisMode");

  MatrixWorkspace_sptr secondWS = this->getProperty("SecondTransmissionRun");

  auto start_overlap = isSet<double>("StartOverlap");
  auto end_overlap = isSet<double>("EndOverlap");
  auto params = isSet<std::vector<double>>("Params");
  auto i0_monitor_index = checkForOptionalInstrumentDefault<int>(
      this, "I0MonitorIndex", instrument, "I0MonitorIndex");

  std::string processing_commands;
  if (this->getPointerToProperty("ProcessingInstructions")->isDefault()) {
    if (analysis_mode == "PointDetectorAnalysis") {
      const int start = static_cast<int>(
          instrument->getNumberParameter("PointDetectorStart")[0]);
      const int stop = static_cast<int>(
          instrument->getNumberParameter("PointDetectorStop")[0]);
      if (start == stop) {
        processing_commands = std::to_string(start);
      } else {
        processing_commands =
            std::to_string(start) + ":" + std::to_string(stop);
      }
    } else {
      processing_commands =
          std::to_string(static_cast<int>(
              instrument->getNumberParameter("MultiDetectorStart")[0])) +
          ":" + std::to_string(firstWS->getNumberHistograms() - 1);
    }
  } else {
    std::string processing_commands_temp =
        this->getProperty("ProcessingInstructions");
    processing_commands = processing_commands_temp;
  }

  auto wavelength_min = checkForMandatoryInstrumentDefault<double>(
      this, "WavelengthMin", instrument, "LambdaMin");
  auto wavelength_max = checkForMandatoryInstrumentDefault<double>(
      this, "WavelengthMax", instrument, "LambdaMax");
  auto wavelength_back_min = checkForOptionalInstrumentDefault<double>(
      this, "MonitorBackgroundWavelengthMin", instrument,
      "MonitorBackgroundMin");
  auto wavelength_back_max = checkForOptionalInstrumentDefault<double>(
      this, "MonitorBackgroundWavelengthMax", instrument,
      "MonitorBackgroundMax");
  auto wavelength_integration_min = checkForOptionalInstrumentDefault<double>(
      this, "MonitorIntegrationWavelengthMin", instrument,
      "MonitorIntegralMin");
  auto wavelength_integration_max = checkForOptionalInstrumentDefault<double>(
      this, "MonitorIntegrationWavelengthMax", instrument,
      "MonitorIntegralMax");

  // construct the algorithm

  IAlgorithm_sptr algCreateTransWS =
      createChildAlgorithm("CreateTransmissionWorkspace", -1, -1, true, 1);
  algCreateTransWS->setRethrows(true);
  algCreateTransWS->initialize();

  if (algCreateTransWS->isInitialized()) {

    algCreateTransWS->setProperty("FirstTransmissionRun", firstWS);

    if (secondWS) {
      algCreateTransWS->setProperty("SecondTransmissionRun", secondWS);
    }

    algCreateTransWS->setProperty("OutputWorkspace", outputWorkspaceName);

    if (start_overlap.is_initialized()) {
      algCreateTransWS->setProperty("StartOverlap", start_overlap.get());
    }
    if (end_overlap.is_initialized()) {
      algCreateTransWS->setProperty("EndOverlap", end_overlap.get());
    }
    if (params.is_initialized()) {
      algCreateTransWS->setProperty("Params", params.get());
    }
    if (i0_monitor_index.is_initialized()) {
      algCreateTransWS->setProperty("I0MonitorIndex", i0_monitor_index.get());
    } else {
      algCreateTransWS->setProperty("I0MonitorIndex", Mantid::EMPTY_INT());
    }
    algCreateTransWS->setProperty("ProcessingInstructions",
                                  processing_commands);
    algCreateTransWS->setProperty("WavelengthMin", wavelength_min);
    algCreateTransWS->setProperty("WavelengthMax", wavelength_max);
    if (wavelength_back_min.is_initialized()) {
      algCreateTransWS->setProperty("MonitorBackgroundWavelengthMin",
                                    wavelength_back_min.get());
    }
    if (wavelength_back_max.is_initialized()) {
      algCreateTransWS->setProperty("MonitorBackgroundWavelengthMax",
                                    wavelength_back_max.get());
    }
    if (wavelength_integration_min.is_initialized()) {
      algCreateTransWS->setProperty("MonitorIntegrationWavelengthMin",
                                    wavelength_integration_min.get());
    }
    if (wavelength_integration_max.is_initialized()) {
      algCreateTransWS->setProperty("MonitorIntegrationWavelengthMax",
                                    wavelength_integration_max.get());
    }

    algCreateTransWS->execute();
    if (!algCreateTransWS->isExecuted()) {
      throw std::runtime_error(
          "CreateTransmissionWorkspace did not execute sucessfully");
    } else {
      MatrixWorkspace_sptr outWS =
          algCreateTransWS->getProperty("OutputWorkspace");
      setProperty("OutputWorkspace", outWS);
    }
  } else {
    throw std::runtime_error(
        "CreateTransmissionWorkspace could not be initialised");
  }
}

template <typename T>
boost::optional<T>
CreateTransmissionWorkspaceAuto::isSet(std::string propName) const {
  auto algProperty = this->getPointerToProperty(propName);
  if (algProperty->isDefault()) {
    return boost::optional<T>();
  } else {
    T value = this->getProperty(propName);
    return boost::optional<T>(value);
  }
}
} // namespace Algorithms
} // namespace Mantid
