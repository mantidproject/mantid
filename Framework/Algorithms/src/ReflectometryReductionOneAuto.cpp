#include "MantidAlgorithms/BoostOptionalToAlgorithmProperty.h"
#include "MantidAlgorithms/ReflectometryReductionOneAuto.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include <boost/optional.hpp>

/*Anonymous namespace*/
namespace {
/**
* Helper free function to calculate MomentumTransfer from lambda and theta
* @param lambda : Value in wavelength
* @param theta  : Value in Degrees
* @return MomentumTransfer
* @
*/
double calculateQ(double lambda, double theta) {
  if (lambda == 0.0)
    throw std::runtime_error("Minimum/Maximum value of the IvsLambda Workspace "
                             "is 0. Cannot calculate Q");
  double thetaInRad = theta * M_PI / 180;
  return (4 * M_PI * sin(thetaInRad)) / lambda;
}
}
/*end of Anonymous namespace*/
namespace Mantid {
namespace Algorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryReductionOneAuto)

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::name
const std::string ReflectometryReductionOneAuto::name() const {
  return "ReflectometryReductionOneAuto";
}

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryReductionOneAuto::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryReductionOneAuto::category() const {
  return "Reflectometry\\ISIS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometryReductionOneAuto::summary() const {
  return "Reduces a single TOF/Lambda reflectometry run into a mod Q vs I/I0 "
         "workspace. Performs transmission corrections.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void ReflectometryReductionOneAuto::init() {
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input, PropertyMode::Mandatory),
      "Input run in TOF or Lambda");

  std::vector<std::string> analysis_modes{"PointDetectorAnalysis",
                                          "MultiDetectorAnalysis"};
  auto analysis_mode_validator =
      boost::make_shared<StringListValidator>(analysis_modes);

  declareProperty(
      make_unique<ArrayProperty<int>>("RegionOfDirectBeam", Direction::Input),
      "Indices of the spectra a pair (lower, upper) that mark the ranges that "
      "correspond to the direct beam in multi-detector mode.");

  declareProperty("AnalysisMode", analysis_modes[0], analysis_mode_validator,
                  "Analysis Mode to Choose", Direction::Input);

  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "FirstTransmissionRun", "", Direction::Input, PropertyMode::Optional),
      "First transmission run workspace in TOF or Wavelength");

  auto tof_validator = boost::make_shared<WorkspaceUnitValidator>("TOF");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "SecondTransmissionRun", "", Direction::Input,
                      PropertyMode::Optional, tof_validator),
                  "Second transmission run workspace in TOF");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output workspace in wavelength q");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspaceWavelength", "", Direction::Output),
                  "Output workspace in wavelength");

  declareProperty(
      make_unique<ArrayProperty<double>>(
          "Params", boost::make_shared<RebinParamsValidator>(true)),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "These parameters are used for stitching together transmission runs. "
      "Values are in wavelength (angstroms). This input is only needed if a "
      "SecondTransmission run is provided.");

  declareProperty("StartOverlap", Mantid::EMPTY_DBL(), "Overlap in Q.",
                  Direction::Input);

  declareProperty("EndOverlap", Mantid::EMPTY_DBL(), "End overlap in Q.",
                  Direction::Input);
  declareProperty("ScaleFactor", 1.0,
                  "Factor you wish to scale Q workspace by.", Direction::Input);
  auto index_bounds = boost::make_shared<BoundedValidator<int>>();
  index_bounds->setLower(0);

  declareProperty(make_unique<PropertyWithValue<int>>(
                      "I0MonitorIndex", Mantid::EMPTY_INT(), Direction::Input),
                  "I0 monitor workspace index. Optional.");
  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "ProcessingInstructions", "", Direction::Input),
                  "Grouping pattern of workspace indices to yield only the"
                  " detectors of interest. See GroupDetectors for syntax.");
  declareProperty("WavelengthMin", Mantid::EMPTY_DBL(),
                  "Wavelength Min in angstroms", Direction::Input);
  declareProperty("WavelengthMax", Mantid::EMPTY_DBL(),
                  "Wavelength Max in angstroms", Direction::Input);
  declareProperty("WavelengthStep", Mantid::EMPTY_DBL(),
                  "Wavelength step in angstroms", Direction::Input);
  declareProperty("MomentumTransferMinimum", Mantid::EMPTY_DBL(),
                  "Minimum Q value in IvsQ "
                  "Workspace. Used for Rebinning "
                  "the IvsQ Workspace",
                  Direction::Input);
  declareProperty("MomentumTransferStep", Mantid::EMPTY_DBL(),
                  "Resolution value in IvsQ Workspace. Used for Rebinning the "
                  "IvsQ Workspace. This value will be made minus to apply "
                  "logarithmic rebinning. If you wish to have linear "
                  "bin-widths then please provide a negative DQQ",
                  Direction::Input);
  declareProperty("MomentumTransferMaximum", Mantid::EMPTY_DBL(),
                  "Maximum Q value in IvsQ "
                  "Workspace. Used for Rebinning "
                  "the IvsQ Workspace",
                  Direction::Input);
  declareProperty("MonitorBackgroundWavelengthMin", Mantid::EMPTY_DBL(),
                  "Monitor wavelength background min in angstroms",
                  Direction::Input);
  declareProperty("MonitorBackgroundWavelengthMax", Mantid::EMPTY_DBL(),
                  "Monitor wavelength background max in angstroms",
                  Direction::Input);
  declareProperty("MonitorIntegrationWavelengthMin", Mantid::EMPTY_DBL(),
                  "Monitor integral min in angstroms", Direction::Input);
  declareProperty("MonitorIntegrationWavelengthMax", Mantid::EMPTY_DBL(),
                  "Monitor integral max in angstroms", Direction::Input);
  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "DetectorComponentName", "", Direction::Input),
                  "Name of the detector component i.e. point-detector. If "
                  "these are not specified, the algorithm will attempt lookup "
                  "using a standard naming convention.");
  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "SampleComponentName", "", Direction::Input),
                  "Name of the sample component i.e. some-surface-holder. If "
                  "these are not specified, the algorithm will attempt lookup "
                  "using a standard naming convention.");

  declareProperty("ThetaIn", Mantid::EMPTY_DBL(), "Final theta in degrees",
                  Direction::Input);
  declareProperty("ThetaOut", Mantid::EMPTY_DBL(),
                  "Calculated final theta in degrees.", Direction::Output);

  declareProperty("NormalizeByIntegratedMonitors", true,
                  "Normalize by dividing by the integrated monitors.");

  declareProperty("CorrectDetectorPositions", true,
                  "Correct detector positions using ThetaIn (if given)");

  declareProperty("StrictSpectrumChecking", true,
                  "Strict checking between spectrum numbers in input "
                  "workspaces and transmission workspaces.");
  std::vector<std::string> correctionAlgorithms = {
      "None", "AutoDetect", "PolynomialCorrection", "ExponentialCorrection"};
  declareProperty("CorrectionAlgorithm", "AutoDetect",
                  boost::make_shared<StringListValidator>(correctionAlgorithms),
                  "The type of correction to perform.");

  declareProperty(make_unique<ArrayProperty<double>>("Polynomial"),
                  "Coefficients to be passed to the PolynomialCorrection"
                  " algorithm.");

  declareProperty(
      make_unique<PropertyWithValue<double>>("C0", 0.0, Direction::Input),
      "C0 value to be passed to the ExponentialCorrection algorithm.");

  declareProperty(
      make_unique<PropertyWithValue<double>>("C1", 0.0, Direction::Input),
      "C1 value to be passed to the ExponentialCorrection algorithm.");

  setPropertyGroup("CorrectionAlgorithm", "Polynomial Corrections");
  setPropertyGroup("Polynomial", "Polynomial Corrections");
  setPropertyGroup("C0", "Polynomial Corrections");
  setPropertyGroup("C1", "Polynomial Corrections");

  setPropertySettings(
      "Polynomial",
      Kernel::make_unique<Kernel::EnabledWhenProperty>(
          "CorrectionAlgorithm", IS_EQUAL_TO, "PolynomialCorrection"));
  setPropertySettings(
      "C0", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                "CorrectionAlgorithm", IS_EQUAL_TO, "ExponentialCorrection"));
  setPropertySettings(
      "C1", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                "CorrectionAlgorithm", IS_EQUAL_TO, "ExponentialCorrection"));

  // Polarization correction inputs --------------
  std::vector<std::string> propOptions;
  propOptions.push_back(noPolarizationCorrectionMode());
  propOptions.push_back(pALabel());
  propOptions.push_back(pNRLabel());

  declareProperty("PolarizationAnalysis", noPolarizationCorrectionMode(),
                  boost::make_shared<StringListValidator>(propOptions),
                  "What Polarization mode will be used?\n"
                  "None: No correction\n"
                  "PNR: Polarized Neutron Reflectivity mode\n"
                  "PA: Full Polarization Analysis PNR-PA");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(cppLabel(), Direction::Input),
      "Effective polarizing power of the polarizing system. "
      "Expressed as a ratio 0 < Pp < 1");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(cApLabel(), Direction::Input),
      "Effective polarizing power of the analyzing system. "
      "Expressed as a ratio 0 < Ap < 1");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(crhoLabel(), Direction::Input),
      "Ratio of efficiencies of polarizer spin-down to polarizer "
      "spin-up. This is characteristic of the polarizer flipper. "
      "Values are constants for each term in a polynomial "
      "expression.");
  declareProperty(Kernel::make_unique<ArrayProperty<double>>(cAlphaLabel(),
                                                             Direction::Input),
                  "Ratio of efficiencies of analyzer spin-down to analyzer "
                  "spin-up. This is characteristic of the analyzer flipper. "
                  "Values are factors for each term in a polynomial "
                  "expression.");
  setPropertyGroup("PolarizationAnalysis", "Polarization Corrections");
  setPropertyGroup(cppLabel(), "Polarization Corrections");
  setPropertyGroup(cApLabel(), "Polarization Corrections");
  setPropertyGroup(crhoLabel(), "Polarization Corrections");
  setPropertyGroup(cAlphaLabel(), "Polarization Corrections");
  setPropertySettings(cppLabel(),
                      Kernel::make_unique<Kernel::EnabledWhenProperty>(
                          "PolarizationAnalysis", IS_NOT_EQUAL_TO,
                          noPolarizationCorrectionMode()));
  setPropertySettings(cApLabel(),
                      Kernel::make_unique<Kernel::EnabledWhenProperty>(
                          "PolarizationAnalysis", IS_NOT_EQUAL_TO,
                          noPolarizationCorrectionMode()));
  setPropertySettings(crhoLabel(),
                      Kernel::make_unique<Kernel::EnabledWhenProperty>(
                          "PolarizationAnalysis", IS_NOT_EQUAL_TO,
                          noPolarizationCorrectionMode()));
  setPropertySettings(cAlphaLabel(),
                      Kernel::make_unique<Kernel::EnabledWhenProperty>(
                          "PolarizationAnalysis", IS_NOT_EQUAL_TO,
                          noPolarizationCorrectionMode()));
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void ReflectometryReductionOneAuto::exec() {
  MatrixWorkspace_sptr in_ws = getProperty("InputWorkspace");
  auto instrument = in_ws->getInstrument();

  // Get all the inputs.

  std::string output_workspace_name = getPropertyValue("OutputWorkspace");
  std::string output_workspace_lam_name =
      getPropertyValue("OutputWorkspaceWavelength");
  std::string analysis_mode = getPropertyValue("AnalysisMode");
  MatrixWorkspace_sptr first_ws = getProperty("FirstTransmissionRun");
  MatrixWorkspace_sptr second_ws = getProperty("SecondTransmissionRun");
  auto start_overlap = isSet<double>("StartOverlap");
  auto end_overlap = isSet<double>("EndOverlap");
  auto params = isSet<MantidVec>("Params");
  auto i0_monitor_index = checkForOptionalInstrumentDefault<int>(
      this, "I0MonitorIndex", instrument, "I0MonitorIndex");

  std::string processing_commands;
  if (this->getPointerToProperty("ProcessingInstructions")->isDefault()) {
    if (analysis_mode == "PointDetectorAnalysis") {
      std::vector<double> pointStart =
          instrument->getNumberParameter("PointDetectorStart");
      std::vector<double> pointStop =
          instrument->getNumberParameter("PointDetectorStop");

      if (pointStart.empty() || pointStop.empty())
        throw std::runtime_error(
            "If ProcessingInstructions is not specified, BOTH "
            "PointDetectorStart "
            "and PointDetectorStop must exist as instrument parameters.\n"
            "Please check if you meant to enter ProcessingInstructions or "
            "if your instrument parameter file is correct.");

      const int detStart = static_cast<int>(pointStart[0]);
      const int detStop = static_cast<int>(pointStop[0]);

      if (detStart == detStop) {
        // If the range given only specifies one detector, we pass along just
        // that one detector
        processing_commands = std::to_string(detStart);
      } else {
        // Otherwise, we create a range.
        processing_commands =
            std::to_string(detStart) + ":" + std::to_string(detStop);
      }
    } else {
      std::vector<double> multiStart =
          instrument->getNumberParameter("MultiDetectorStart");
      if (multiStart.empty())
        throw std::runtime_error(
            "If ProcessingInstructions is not specified, MultiDetectorStart"
            "must exist as an instrument parameter.\n"
            "Please check if you meant to enter ProcessingInstructions or "
            "if your instrument parameter file is correct.");
      processing_commands = std::to_string(static_cast<int>(multiStart[0])) +
                            ":" +
                            std::to_string(in_ws->getNumberHistograms() - 1);
    }
  } else {
    std::string processing_commands_temp =
        this->getProperty("ProcessingInstructions");
    processing_commands = processing_commands_temp;
  }

  double wavelength_min = checkForMandatoryInstrumentDefault<double>(
      this, "WavelengthMin", instrument, "LambdaMin");
  double wavelength_max = checkForMandatoryInstrumentDefault<double>(
      this, "WavelengthMax", instrument, "LambdaMax");
  auto wavelength_step = isSet<double>("WavelengthStep");
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

  auto detector_component_name = isSet<std::string>("DetectorComponentName");
  auto sample_component_name = isSet<std::string>("SampleComponentName");
  auto theta_in = isSet<double>("ThetaIn");
  auto region_of_direct_beam = isSet<std::vector<int>>("RegionOfDirectBeam");

  bool correct_positions = this->getProperty("CorrectDetectorPositions");
  bool strict_spectrum_checking = this->getProperty("StrictSpectrumChecking");
  bool norm_by_int_mons = getProperty("NormalizeByIntegratedMonitors");
  const std::string correction_algorithm = getProperty("CorrectionAlgorithm");

  // Pass the arguments and execute the main algorithm.

  IAlgorithm_sptr refRedOne =
      createChildAlgorithm("ReflectometryReductionOne", -1, -1, true, 1);
  refRedOne->initialize();
  if (refRedOne->isInitialized()) {
    refRedOne->setProperty("InputWorkspace", in_ws);
    refRedOne->setProperty("AnalysisMode", analysis_mode);
    refRedOne->setProperty("OutputWorkspace", output_workspace_name);
    refRedOne->setProperty("OutputWorkspaceWavelength",
                           output_workspace_lam_name);
    refRedOne->setProperty("NormalizeByIntegratedMonitors", norm_by_int_mons);

    if (i0_monitor_index.is_initialized()) {
      if (i0_monitor_index.get() >= 0)
        refRedOne->setProperty("I0MonitorIndex", i0_monitor_index.get());
      else
        throw std::invalid_argument(
            "I0MonitorIndex must be an integer greater than or equal to 0");
    }
    refRedOne->setProperty("ProcessingInstructions", processing_commands);
    refRedOne->setProperty("WavelengthMin", wavelength_min);
    refRedOne->setProperty("WavelengthMax", wavelength_max);
    if (wavelength_back_min.is_initialized())
      refRedOne->setProperty("MonitorBackgroundWavelengthMin",
                             wavelength_back_min.get());
    if (wavelength_back_max.is_initialized())
      refRedOne->setProperty("MonitorBackgroundWavelengthMax",
                             wavelength_back_max.get());
    if (wavelength_integration_min.is_initialized())
      refRedOne->setProperty("MonitorIntegrationWavelengthMin",
                             wavelength_integration_min.get());
    if (wavelength_integration_max.is_initialized())
      refRedOne->setProperty("MonitorIntegrationWavelengthMax",
                             wavelength_integration_max.get());
    refRedOne->setProperty("CorrectDetectorPositions", correct_positions);
    refRedOne->setProperty("StrictSpectrumChecking", strict_spectrum_checking);
    if (correction_algorithm == "PolynomialCorrection") {
      // Copy across the polynomial
      refRedOne->setProperty("CorrectionAlgorithm", "PolynomialCorrection");
      refRedOne->setProperty("Polynomial", getPropertyValue("Polynomial"));
    } else if (correction_algorithm == "ExponentialCorrection") {
      // Copy across c0 and c1
      refRedOne->setProperty("CorrectionAlgorithm", "ExponentialCorrection");
      refRedOne->setProperty("C0", getPropertyValue("C0"));
      refRedOne->setProperty("C1", getPropertyValue("C1"));
    } else if (correction_algorithm == "AutoDetect") {
      // Figure out what to do from the instrument
      try {
        auto inst = in_ws->getInstrument();

        const std::vector<std::string> corrVec =
            inst->getStringParameter("correction");
        const std::string correctionStr = !corrVec.empty() ? corrVec[0] : "";

        if (correctionStr.empty())
          throw std::runtime_error(
              "'correction' instrument parameter was not found.");

        const std::vector<std::string> polyVec =
            inst->getStringParameter("polynomial");
        const std::string polyStr = !polyVec.empty() ? polyVec[0] : "";

        const std::vector<std::string> c0Vec = inst->getStringParameter("C0");
        const std::string c0Str = !c0Vec.empty() ? c0Vec[0] : "";

        const std::vector<std::string> c1Vec = inst->getStringParameter("C1");
        const std::string c1Str = !c1Vec.empty() ? c1Vec[0] : "";

        if (correctionStr == "polynomial" && polyStr.empty())
          throw std::runtime_error(
              "'polynomial' instrument parameter was not found.");

        if (correctionStr == "exponential" && (c0Str.empty() || c1Str.empty()))
          throw std::runtime_error(
              "'C0' or 'C1' instrument parameter was not found.");

        if (correctionStr == "polynomial") {
          refRedOne->setProperty("CorrectionAlgorithm", "PolynomialCorrection");
          refRedOne->setProperty("Polynomial", polyStr);
        } else if (correctionStr == "exponential") {
          refRedOne->setProperty("CorrectionAlgorithm",
                                 "ExponentialCorrection");
          refRedOne->setProperty("C0", c0Str);
          refRedOne->setProperty("C1", c1Str);
        }

      } catch (std::runtime_error &e) {
        g_log.warning() << "Could not autodetect polynomial correction method. "
                           "Polynomial correction will not be performed. "
                           "Reason for failure: " << e.what() << '\n';
        refRedOne->setProperty("CorrectionAlgorithm", "None");
      }

    } else {
      // None was selected
      refRedOne->setProperty("CorrectionAlgorithm", "None");
    }

    if (first_ws) {
      refRedOne->setProperty("FirstTransmissionRun", first_ws);
    }

    if (second_ws) {
      refRedOne->setProperty("SecondTransmissionRun", second_ws);
    }

    if (start_overlap.is_initialized()) {
      refRedOne->setProperty("StartOverlap", start_overlap.get());
    }

    if (end_overlap.is_initialized()) {
      refRedOne->setProperty("EndOverlap", end_overlap.get());
    }

    if (params.is_initialized()) {
      refRedOne->setProperty("Params", params.get());
    }

    if (wavelength_step.is_initialized()) {
      refRedOne->setProperty("WavelengthStep", wavelength_step.get());
    }

    if (region_of_direct_beam.is_initialized()) {
      refRedOne->setProperty("RegionOfDirectBeam", region_of_direct_beam.get());
    }

    if (detector_component_name.is_initialized()) {
      refRedOne->setProperty("DetectorComponentName",
                             detector_component_name.get());
    }

    if (sample_component_name.is_initialized()) {
      refRedOne->setProperty("SampleComponentName",
                             sample_component_name.get());
    }

    if (theta_in.is_initialized()) {
      refRedOne->setProperty("ThetaIn", theta_in.get());
    }
    double scaleFactor = getProperty("ScaleFactor");
    if (scaleFactor != 1.0) {
      refRedOne->setProperty("ScaleFactor", scaleFactor);
    }
    auto momentumTransferMinimum = isSet<double>("MomentumTransferMinimum");
    auto momentumTransferStep = isSet<double>("MomentumTransferStep");
    auto momentumTransferMaximum = isSet<double>("MomentumTransferMaximum");

    if (momentumTransferStep.is_initialized()) {
      refRedOne->setProperty("MomentumTransferStep",
                             momentumTransferStep.get());
    }
    if (momentumTransferMinimum.is_initialized())
      refRedOne->setProperty("MomentumTransferMinimum",
                             momentumTransferMinimum.get());
    if (momentumTransferMaximum.is_initialized())
      refRedOne->setProperty("MomentumTransferMaximum",
                             momentumTransferMaximum.get());
    if (theta_in.is_initialized()) {
      if (!momentumTransferMinimum.is_initialized())
        momentumTransferMinimum = calculateQ(wavelength_max, theta_in.get());
      if (!momentumTransferStep.is_initialized()) {
        IAlgorithm_sptr calcResAlg =
            AlgorithmManager::Instance().create("CalculateResolution");
        calcResAlg->setProperty("Workspace", in_ws);
        calcResAlg->setProperty("TwoTheta", theta_in.get());
        calcResAlg->execute();
        if (!calcResAlg->isExecuted())
          throw std::runtime_error(
              "CalculateResolution failed. Please manually "
              "enter a value in the dQ/Q column.");
        double resolution = calcResAlg->getProperty("Resolution");
        momentumTransferStep = resolution;
      }
      if (!momentumTransferMaximum.is_initialized())
        momentumTransferMaximum = calculateQ(wavelength_min, theta_in.get());
      refRedOne->setProperty("MomentumTransferMinimum",
                             momentumTransferMinimum.get());
      refRedOne->setProperty("MomentumTransferStep",
                             momentumTransferStep.get());
      refRedOne->setProperty("MomentumTransferMaximum",
                             momentumTransferMaximum.get());
    }
    refRedOne->execute();
    if (!refRedOne->isExecuted()) {
      throw std::runtime_error(
          "ReflectometryReductionOne did not execute sucessfully");
    }

    MatrixWorkspace_sptr new_IvsQ1 = refRedOne->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr new_IvsLam1 =
        refRedOne->getProperty("OutputWorkspaceWavelength");
    double thetaOut1 = refRedOne->getProperty("ThetaOut");
    setProperty("OutputWorkspace", new_IvsQ1);
    setProperty("OutputWorkspaceWavelength", new_IvsLam1);
    setProperty("ThetaOut", thetaOut1);
    // set properties so they can be retrieved by GenericDataProcesser if
    // necessary.
    setProperty("MomentumTransferMinimum",
                boost::lexical_cast<double>(
                    refRedOne->getPropertyValue("MomentumTransferMinimum")));
    setProperty("MomentumTransferStep",
                boost::lexical_cast<double>(
                    refRedOne->getPropertyValue("MomentumTransferStep")));
    setProperty("MomentumTransferMaximum",
                boost::lexical_cast<double>(
                    refRedOne->getPropertyValue("MomentumTransferMaximum")));
    if (theta_in.is_initialized())
      setProperty("ThetaIn", theta_in.get());
    else
      setProperty("ThetaIn", thetaOut1 / 2.);

  } else {
    throw std::runtime_error(
        "ReflectometryReductionOne could not be initialised");
  }
}

template <typename T>
boost::optional<T>
ReflectometryReductionOneAuto::isSet(std::string propName) const {
  auto algProperty = this->getPointerToProperty(propName);
  if (algProperty->isDefault()) {
    return boost::optional<T>();
  } else {
    T value = this->getProperty(propName);
    return boost::optional<T>(value);
  }
}

bool ReflectometryReductionOneAuto::checkGroups() {
  std::string wsName = getPropertyValue("InputWorkspace");

  try {
    auto ws =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);
    if (ws)
      return true;
  } catch (...) {
  }
  return false;
}
/**
 * Sum over transmission group workspaces to produce one
 * workspace.
 * @param transGroup : The transmission group to be processed
 * @return A workspace pointer containing the sum of transmission workspaces.
 */
Mantid::API::Workspace_sptr
ReflectometryReductionOneAuto::sumOverTransmissionGroup(
    WorkspaceGroup_sptr &transGroup) {
  // Handle transmission runs

  // we clone the first member of transmission group as to
  // avoid addition in place which would affect the original
  // workspace member.
  //
  // We used .release because clone() will return a unique_ptr.
  // we need to release the ownership of the pointer so that it
  // can be cast into a shared_ptr of type Workspace.
  Workspace_sptr transmissionRunSum(transGroup->getItem(0)->clone());

  // make a variable to store the overall total of the summation
  MatrixWorkspace_sptr total;
  // set up and initialize plus algorithm.
  auto plusAlg = this->createChildAlgorithm("Plus");
  plusAlg->setChild(true);
  // plusAlg->setRethrows(true);
  plusAlg->initialize();
  // now accumalate the group members
  for (size_t item = 1; item < transGroup->size(); ++item) {
    plusAlg->setProperty("LHSWorkspace", transmissionRunSum);
    plusAlg->setProperty("RHSWorkspace", transGroup->getItem(item));
    plusAlg->setProperty("OutputWorkspace", transmissionRunSum);
    plusAlg->execute();
    total = plusAlg->getProperty("OutputWorkspace");
  }
  return total;
}

bool ReflectometryReductionOneAuto::processGroups() {
  // isPolarizationCorrectionOn is used to decide whether
  // we should process our Transmission WorkspaceGroup members
  // as individuals (not multiperiod) when PolarizationCorrection is off,
  // or sum over all of the workspaces in the group
  // and used that sum as our TransmissionWorkspace when PolarizationCorrection
  // is on.
  const bool isPolarizationCorrectionOn =
      this->getPropertyValue("PolarizationAnalysis") !=
      noPolarizationCorrectionMode();

  // this algorithm effectively behaves as MultiPeriodGroupAlgorithm
  m_usingBaseProcessGroups = true;

  // Get our input workspace group
  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      getPropertyValue("InputWorkspace"));
  // Get name of IvsQ workspace
  const std::string outputIvsQ = this->getPropertyValue("OutputWorkspace");
  // Get name of IvsLam workspace
  const std::string outputIvsLam =
      this->getPropertyValue("OutputWorkspaceWavelength");

  // Create a copy of ourselves
  Algorithm_sptr alg = this->createChildAlgorithm(
      this->name(), -1, -1, this->isLogging(), this->version());
  alg->setChild(false);
  alg->setRethrows(true);

  // Copy all the non-workspace properties over
  std::vector<Property *> props = this->getProperties();
  for (auto &prop : props) {
    if (prop) {
      IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
      if (!wsProp)
        alg->setPropertyValue(prop->name(), prop->value());
    }
  }

  // Check if the transmission runs are groups or not
  const std::string firstTrans = this->getPropertyValue("FirstTransmissionRun");
  WorkspaceGroup_sptr firstTransG;
  if (!firstTrans.empty()) {
    auto firstTransWS =
        AnalysisDataService::Instance().retrieveWS<Workspace>(firstTrans);
    firstTransG = boost::dynamic_pointer_cast<WorkspaceGroup>(firstTransWS);

    if (!firstTransG) {
      // we only have one transmission workspace, so we use it as it is.
      alg->setProperty("FirstTransmissionRun", firstTrans);
    } else if (group->size() != firstTransG->size() &&
               !isPolarizationCorrectionOn) {
      // if they are not the same size then we cannot associate a transmission
      // group workspace member with every input group workpspace member.
      throw std::runtime_error("FirstTransmissionRun WorkspaceGroup must be "
                               "the same size as the InputWorkspace "
                               "WorkspaceGroup");
    }
  }

  const std::string secondTrans =
      this->getPropertyValue("SecondTransmissionRun");
  WorkspaceGroup_sptr secondTransG;
  if (!secondTrans.empty()) {
    auto secondTransWS =
        AnalysisDataService::Instance().retrieveWS<Workspace>(secondTrans);
    secondTransG = boost::dynamic_pointer_cast<WorkspaceGroup>(secondTransWS);

    if (!secondTransG)
      // we only have one transmission workspace, so we use it as it is.
      alg->setProperty("SecondTransmissionRun", secondTrans);

    else if (group->size() != secondTransG->size() &&
             !isPolarizationCorrectionOn) {
      // if they are not the same size then we cannot associate a transmission
      // group workspace member with every input group workpspace member.
      throw std::runtime_error("SecondTransmissionRun WorkspaceGroup must be "
                               "the same size as the InputWorkspace "
                               "WorkspaceGroup");
    }
  }
  std::vector<std::string> IvsQGroup, IvsLamGroup;

  // Execute algorithm over each group member (or period, if this is
  // multiperiod)
  size_t numMembers = group->size();
  for (size_t i = 0; i < numMembers; ++i) {
    const std::string IvsQName = outputIvsQ + "_" + std::to_string(i + 1);
    const std::string IvsLamName = outputIvsLam + "_" + std::to_string(i + 1);

    // If our transmission run is a group and PolarizationCorrection is on
    // then we sum our transmission group members.
    //
    // This is done inside of the for loop to avoid the wrong workspace being
    // used when these arguments are passed through to the exec() method.
    // If this is not set in the loop, exec() will fetch the first workspace
    // from the specified Transmission Group workspace that the user entered.
    if (firstTransG && isPolarizationCorrectionOn) {
      auto firstTransmissionSum = sumOverTransmissionGroup(firstTransG);
      alg->setProperty("FirstTransmissionRun", firstTransmissionSum);
    }
    if (secondTransG && isPolarizationCorrectionOn) {
      auto secondTransmissionSum = sumOverTransmissionGroup(secondTransG);
      alg->setProperty("SecondTransmissionRun", secondTransmissionSum);
    }

    // Otherwise, if polarization correction is off, we process them
    // using one transmission group member at a time.
    if (firstTransG && !isPolarizationCorrectionOn) // polarization off
      alg->setProperty("FirstTransmissionRun",
                       firstTransG->getItem(i)->getName());
    if (secondTransG && !isPolarizationCorrectionOn) // polarization off
      alg->setProperty("SecondTransmissionRun",
                       secondTransG->getItem(i)->getName());

    alg->setProperty("InputWorkspace", group->getItem(i)->getName());
    alg->setProperty("OutputWorkspace", IvsQName);
    alg->setProperty("OutputWorkspaceWavelength", IvsLamName);
    alg->execute();

    MatrixWorkspace_sptr tempFirstTransWS =
        alg->getProperty("FirstTransmissionRun");

    IvsQGroup.push_back(IvsQName);
    IvsLamGroup.push_back(IvsLamName);

    // We use the first group member for our thetaout value
    if (i == 0)
      this->setPropertyValue("ThetaOut", alg->getPropertyValue("ThetaOut"));
  }

  // Group the IvsQ and IvsLam workspaces
  Algorithm_sptr groupAlg = this->createChildAlgorithm("GroupWorkspaces");
  groupAlg->setChild(false);
  groupAlg->setRethrows(true);

  groupAlg->setProperty("InputWorkspaces", IvsLamGroup);
  groupAlg->setProperty("OutputWorkspace", outputIvsLam);
  groupAlg->execute();

  groupAlg->setProperty("InputWorkspaces", IvsQGroup);
  groupAlg->setProperty("OutputWorkspace", outputIvsQ);
  groupAlg->execute();

  // If this is a multiperiod workspace and we have polarization corrections
  // enabled
  if (isPolarizationCorrectionOn) {
    if (group->isMultiperiod()) {
      // Perform polarization correction over the IvsLam group
      Algorithm_sptr polAlg =
          this->createChildAlgorithm("PolarizationCorrection");
      polAlg->setChild(false);
      polAlg->setRethrows(true);

      polAlg->setProperty("InputWorkspace", outputIvsLam);
      polAlg->setProperty("OutputWorkspace", outputIvsLam);
      polAlg->setProperty("PolarizationAnalysis",
                          this->getPropertyValue("PolarizationAnalysis"));
      polAlg->setProperty("CPp", this->getPropertyValue(cppLabel()));
      polAlg->setProperty("CRho", this->getPropertyValue(crhoLabel()));
      polAlg->setProperty("CAp", this->getPropertyValue(cApLabel()));
      polAlg->setProperty("CAlpha", this->getPropertyValue(cAlphaLabel()));
      polAlg->execute();

      // Now we've overwritten the IvsLam workspaces, we'll need to recalculate
      // the IvsQ ones
      alg->setProperty("FirstTransmissionRun", "");
      alg->setProperty("SecondTransmissionRun", "");
      for (size_t i = 0; i < numMembers; ++i) {
        const std::string IvsQName = outputIvsQ + "_" + std::to_string(i + 1);
        const std::string IvsLamName =
            outputIvsLam + "_" + std::to_string(i + 1);
        alg->setProperty("InputWorkspace", IvsLamName);
        alg->setProperty("OutputWorkspace", IvsQName);
        alg->setProperty("CorrectionAlgorithm", "None");
        alg->setProperty("OutputWorkspaceWavelength", IvsLamName);
        alg->execute();
      }
    } else {
      g_log.warning("Polarization corrections can only be performed on "
                    "multiperiod workspaces.");
    }
  }

  // We finished successfully
  // set the values of these properties so they can be retrieved by the
  // Interface.
  this->setProperty("MomentumTransferMinimum",
                    boost::lexical_cast<double>(
                        alg->getPropertyValue("MomentumTransferMinimum")));
  this->setProperty("MomentumTransferStep",
                    boost::lexical_cast<double>(
                        alg->getPropertyValue("MomentumTransferStep")));
  this->setProperty("MomentumTransferMaximum",
                    boost::lexical_cast<double>(
                        alg->getPropertyValue("MomentumTransferMaximum")));
  // setting output properties.
  this->setPropertyValue("OutputWorkspace", outputIvsQ);
  this->setPropertyValue("OutputWorkspaceWavelength", outputIvsLam);
  return true;
}
} // namespace Algorithms
} // namespace Mantid
