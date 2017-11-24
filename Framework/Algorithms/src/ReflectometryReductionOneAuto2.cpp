#include "MantidAlgorithms/ReflectometryReductionOneAuto2.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/BoostOptionalToAlgorithmProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace Algorithms {

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryReductionOneAuto2)

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::name
const std::string ReflectometryReductionOneAuto2::name() const {
  return "ReflectometryReductionOneAuto";
}

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryReductionOneAuto2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryReductionOneAuto2::category() const {
  return "Reflectometry\\ISIS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometryReductionOneAuto2::summary() const {
  return "Reduces a single TOF/Lambda reflectometry run into a mod Q vs I/I0 "
         "workspace attempting to pick instrument parameters for missing "
         "properties";
}

/** Validate transmission runs
*
* @return :: result of the validation as a map
*/
std::map<std::string, std::string>
ReflectometryReductionOneAuto2::validateInputs() {

  std::map<std::string, std::string> results;

  // Validate transmission runs only if our input workspace is a group
  if (!checkGroups())
    return results;

  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      getPropertyValue("InputWorkspace"));
  if (!group)
    return results;

  // First transmission run
  const std::string firstStr = getPropertyValue("FirstTransmissionRun");
  if (!firstStr.empty()) {
    auto firstTransGroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(firstStr);
    // If it is not a group, we don't need to validate its size
    if (!firstTransGroup)
      return results;

    const bool polarizationCorrections =
        getPropertyValue("PolarizationAnalysis") != "None";

    if (group->size() != firstTransGroup->size() && !polarizationCorrections) {
      // If they are not the same size then we cannot associate a transmission
      // group member with every input group member.
      results["FirstTransmissionRun"] =
          "FirstTransmissionRun group must be the "
          "same size as the InputWorkspace group "
          "when polarization analysis is 'None'.";
    }
  }

  // The same for the second transmission run
  const std::string secondStr = getPropertyValue("SecondTransmissionRun");
  if (!secondStr.empty()) {
    auto secondTransGroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(secondStr);
    // If it is not a group, we don't need to validate its size
    if (!secondTransGroup)
      return results;
    const bool polarizationCorrections =
        getPropertyValue("PolarizationAnalysis") != "None";

    if (group->size() != secondTransGroup->size() && !polarizationCorrections) {
      results["SecondTransmissionRun"] =
          "SecondTransmissionRun group must be the "
          "same size as the InputWorkspace group "
          "when polarization analysis is 'None'.";
    }
  }

  return results;
}

/** Initialize the algorithm's properties.
*/
void ReflectometryReductionOneAuto2::init() {

  // Input ws
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input, PropertyMode::Mandatory),
      "Input run in TOF or wavelength");

  // Reduction type
  initReductionProperties();

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
  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "ProcessingInstructions", "", Direction::Input),
                  "Grouping pattern of workspace indices to yield only the"
                  " detectors of interest. See GroupDetectors for syntax.");

  // Theta
  declareProperty("ThetaIn", Mantid::EMPTY_DBL(), "Angle in degrees",
                  Direction::Input);

  // Detector position correction type
  const std::vector<std::string> correctionType{"VerticalShift",
                                                "RotateAroundSample"};
  auto correctionTypeValidator = boost::make_shared<CompositeValidator>();
  correctionTypeValidator->add(
      boost::make_shared<MandatoryValidator<std::string>>());
  correctionTypeValidator->add(
      boost::make_shared<StringListValidator>(correctionType));
  declareProperty(
      "DetectorCorrectionType", correctionType[0], correctionTypeValidator,
      "Whether detectors should be shifted vertically or rotated around the "
      "sample position.",
      Direction::Input);

  // Wavelength limits
  declareProperty("WavelengthMin", Mantid::EMPTY_DBL(),
                  "Wavelength Min in angstroms", Direction::Input);
  declareProperty("WavelengthMax", Mantid::EMPTY_DBL(),
                  "Wavelength Max in angstroms", Direction::Input);

  // Direct beam
  initDirectBeamProperties();

  // Monitor properties
  initMonitorProperties();
  // Normalization by integrated monitors
  declareProperty("NormalizeByIntegratedMonitors", true,
                  "Normalize by dividing by the integrated monitors.");

  // Init properties for transmission normalization
  initTransmissionProperties();

  // Init properties for algorithmic corrections
  initAlgorithmicProperties(true);

  // Momentum transfer properties
  initMomentumTransferProperties();

  // Polarization correction
  std::vector<std::string> propOptions = {"None", "PA", "PNR"};
  declareProperty("PolarizationAnalysis", "None",
                  boost::make_shared<StringListValidator>(propOptions),
                  "Polarization analysis mode.");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("CPp", Direction::Input),
      "Effective polarizing power of the polarizing system. "
      "Expressed as a ratio 0 < Pp < 1");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("CAp", Direction::Input),
      "Effective polarizing power of the analyzing system. "
      "Expressed as a ratio 0 < Ap < 1");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("CRho", Direction::Input),
      "Ratio of efficiencies of polarizer spin-down to polarizer "
      "spin-up. This is characteristic of the polarizer flipper. "
      "Values are constants for each term in a polynomial "
      "expression.");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("CAlpha", Direction::Input),
      "Ratio of efficiencies of analyzer spin-down to analyzer "
      "spin-up. This is characteristic of the analyzer flipper. "
      "Values are factors for each term in a polynomial "
      "expression.");
  setPropertyGroup("PolarizationAnalysis", "Polarization Corrections");
  setPropertyGroup("CPp", "Polarization Corrections");
  setPropertyGroup("CAp", "Polarization Corrections");
  setPropertyGroup("CRho", "Polarization Corrections");
  setPropertyGroup("CAlpha", "Polarization Corrections");

  // Init properties for diagnostics
  initDebugProperties();

  // Output workspace in Q
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspaceBinned", "", Direction::Output),
                  "Output workspace in Q (rebinned workspace)");

  // Output workspace in Q (unbinned)
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output workspace in Q (native binning)");

  // Output workspace in wavelength
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspaceWavelength", "", Direction::Output),
                  "Output workspace in wavelength");
}

/** Execute the algorithm.
*/
void ReflectometryReductionOneAuto2::exec() {

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  auto instrument = inputWS->getInstrument();

  IAlgorithm_sptr alg = createChildAlgorithm("ReflectometryReductionOne");
  alg->initialize();

  // Mandatory properties
  alg->setProperty("SummationType", getPropertyValue("SummationType"));
  alg->setProperty("ReductionType", getPropertyValue("ReductionType"));
  alg->setProperty("Diagnostics", getPropertyValue("Diagnostics"));
  double wavMin = checkForMandatoryInstrumentDefault<double>(
      this, "WavelengthMin", instrument, "LambdaMin");
  alg->setProperty("WavelengthMin", wavMin);
  double wavMax = checkForMandatoryInstrumentDefault<double>(
      this, "WavelengthMax", instrument, "LambdaMax");
  alg->setProperty("WavelengthMax", wavMax);

  const auto instructions =
      populateProcessingInstructions(alg, instrument, inputWS);

  // Now that we know the detectors of interest, we can move them if necessary
  // (i.e. if theta is given). If not, we calculate theta from the current
  // detector positions
  double theta;
  if (!getPointerToProperty("ThetaIn")->isDefault()) {
    theta = getProperty("ThetaIn");
    inputWS = correctDetectorPositions(instructions, inputWS);
  } else {
    // Calculate theta
    theta = calculateTheta(instructions, inputWS);
  }
  alg->setProperty("ThetaIn", theta);

  // Optional properties

  populateDirectBeamProperties(alg);
  populateMonitorProperties(alg, instrument);
  alg->setPropertyValue("NormalizeByIntegratedMonitors",
                        getPropertyValue("NormalizeByIntegratedMonitors"));
  bool transRunsFound = populateTransmissionProperties(alg);
  if (transRunsFound)
    alg->setProperty("StrictSpectrumChecking",
                     getPropertyValue("StrictSpectrumChecking"));
  else
    populateAlgorithmicCorrectionProperties(alg, instrument);

  alg->setProperty("InputWorkspace", inputWS);
  alg->execute();

  MatrixWorkspace_sptr IvsLam = alg->getProperty("OutputWorkspaceWavelength");
  MatrixWorkspace_sptr IvsQ = alg->getProperty("OutputWorkspace");

  std::vector<double> params;
  MatrixWorkspace_sptr IvsQB = rebinAndScale(IvsQ, theta, params);

  setProperty("OutputWorkspaceWavelength", IvsLam);
  setProperty("OutputWorkspace", IvsQ);
  setProperty("OutputWorkspaceBinned", IvsQB);

  // Set other properties so they can be updated in the Reflectometry interface
  setProperty("ThetaIn", theta);
  if (!params.empty()) {
    if (params.size() == 3) {
      setProperty("MomentumTransferMin", params[0]);
      setProperty("MomentumTransferStep", -params[1]);
      setProperty("MomentumTransferMax", params[2]);
    } else {
      setProperty("MomentumTransferMin", IvsQ->x(0).front());
      setProperty("MomentumTransferMax", IvsQ->x(0).back());
      setProperty("MomentumTransferStep", -params[0]);
    }
  }
  if (getPointerToProperty("ScaleFactor")->isDefault())
    setProperty("ScaleFactor", 1.0);
}

/** Returns the detectors of interest, specified via processing instructions.
* Note that this returns the names of the parent detectors of the first and
* last spectrum indices in the processing instructions. It is assumed that all
* the interim detectors have the same parent.
*
* @param instructions :: processing instructions defining detectors of interest
* @param inputWS :: the input workspace
* @return :: the names of the detectors of interest
*/
std::vector<std::string> ReflectometryReductionOneAuto2::getDetectorNames(
    const std::string &instructions, MatrixWorkspace_sptr inputWS) {

  std::vector<std::string> wsIndices;
  boost::split(wsIndices, instructions, boost::is_any_of(":,-+"));
  // vector of comopnents
  std::vector<std::string> detectors;

  try {
    for (const auto wsIndex : wsIndices) {

      size_t index = boost::lexical_cast<size_t>(wsIndex);

      auto detector = inputWS->getDetector(index);
      auto parent = detector->getParent();

      if (parent) {
        auto parentType = parent->type();
        auto detectorName = (parentType == "Instrument") ? detector->getName()
                                                         : parent->getName();
        detectors.push_back(detectorName);
      }
    }
  } catch (boost::bad_lexical_cast &) {
    throw std::runtime_error("Invalid processing instructions: " +
                             instructions);
  }

  return detectors;
}

/** Correct an instrument component by shifting it vertically or
* rotating it around the sample.
*
* @param instructions :: processing instructions defining the detectors of
* interest
* @param inputWS :: the input workspace
* @return :: the corrected workspace
*/
MatrixWorkspace_sptr ReflectometryReductionOneAuto2::correctDetectorPositions(
    const std::string &instructions, MatrixWorkspace_sptr inputWS) {

  auto detectorsOfInterest = getDetectorNames(instructions, inputWS);

  // Detectors of interest may be empty. This happens for instance when we input
  // a workspace that was previously reduced using this algorithm. In this case,
  // we shouldn't correct the detector positions
  if (detectorsOfInterest.empty())
    return inputWS;

  const std::set<std::string> detectorSet(detectorsOfInterest.begin(),
                                          detectorsOfInterest.end());

  const double theta = getProperty("ThetaIn");
  const std::string correctionType = getProperty("DetectorCorrectionType");

  MatrixWorkspace_sptr corrected = inputWS;

  for (const auto &detector : detectorSet) {
    IAlgorithm_sptr alg =
        createChildAlgorithm("SpecularReflectionPositionCorrect");
    alg->setProperty("InputWorkspace", corrected);
    alg->setProperty("TwoTheta", theta * 2);
    alg->setProperty("DetectorCorrectionType", correctionType);
    alg->setProperty("DetectorComponentName", detector);
    alg->execute();
    corrected = alg->getProperty("OutputWorkspace");
  }

  return corrected;
}

/** Calculate the theta value of the detector of interest specified via
* processing instructions
*
* @param instructions :: processing instructions defining the detectors of
* interest
* @param inputWS :: the input workspace
* @return :: the angle of the detector (only the first detector is considered)
*/
double
ReflectometryReductionOneAuto2::calculateTheta(const std::string &instructions,
                                               MatrixWorkspace_sptr inputWS) {

  const auto detectorsOfInterest = getDetectorNames(instructions, inputWS);

  // Detectors of interest may be empty. This happens for instance when we input
  // a workspace that was previously reduced using this algorithm. In this case,
  // we can't calculate theta
  if (detectorsOfInterest.empty())
    return 0.0;

  IAlgorithm_sptr alg =
      createChildAlgorithm("SpecularReflectionCalculateTheta");
  alg->setProperty("InputWorkspace", inputWS);
  alg->setProperty("DetectorComponentName", detectorsOfInterest[0]);
  alg->execute();
  const double theta = alg->getProperty("TwoTheta");
  // First factor 0.5 detector position, which isexpected to be at 2 * theta
  // Second factor 0.5 comes from SpecularReflectionCalculateTheta, which
  // outputs 2 * theta
  return theta * 0.5 * 0.5;
}

/** Set direct beam properties
*
* @param alg :: ReflectometryReductionOne algorithm
*/
void ReflectometryReductionOneAuto2::populateDirectBeamProperties(
    IAlgorithm_sptr alg) {

  alg->setPropertyValue("RegionOfDirectBeam",
                        getPropertyValue("RegionOfDirectBeam"));
}

/** Set algorithmic correction properties
*
* @param alg :: ReflectometryReductionOne algorithm
* @param instrument :: The instrument attached to the workspace
*/
void ReflectometryReductionOneAuto2::populateAlgorithmicCorrectionProperties(
    IAlgorithm_sptr alg, Instrument_const_sptr instrument) {

  // With algorithmic corrections, monitors should not be integrated, see below

  const std::string correctionAlgorithm = getProperty("CorrectionAlgorithm");

  if (correctionAlgorithm == "PolynomialCorrection") {
    alg->setProperty("NormalizeByIntegratedMonitors", false);
    alg->setProperty("CorrectionAlgorithm", "PolynomialCorrection");
    alg->setPropertyValue("Polynomial", getPropertyValue("Polynomial"));

  } else if (correctionAlgorithm == "ExponentialCorrection") {
    alg->setProperty("NormalizeByIntegratedMonitors", false);
    alg->setProperty("CorrectionAlgorithm", "ExponentialCorrection");
    alg->setProperty("C0", getPropertyValue("C0"));
    alg->setProperty("C1", getPropertyValue("C1"));

  } else if (correctionAlgorithm == "AutoDetect") {
    // Figure out what to do from the instrument
    try {
      const auto corrVec = instrument->getStringParameter("correction");
      if (corrVec.empty()) {
        throw std::runtime_error(
            "Could not find parameter 'correction' in "
            "parameter file. Cannot auto detect the type of "
            "correction.");
      }

      const std::string correctionStr = corrVec[0];

      if (correctionStr == "polynomial") {
        const auto polyVec = instrument->getStringParameter("polystring");
        if (polyVec.empty())
          throw std::runtime_error("Could not find parameter 'polystring' in "
                                   "parameter file. Cannot apply polynomial "
                                   "correction.");
        alg->setProperty("CorrectionAlgorithm", "PolynomialCorrection");
        alg->setProperty("Polynomial", polyVec[0]);
      } else if (correctionStr == "exponential") {
        const auto c0Vec = instrument->getStringParameter("C0");
        if (c0Vec.empty())
          throw std::runtime_error(
              "Could not find parameter 'C0' in parameter "
              "file. Cannot apply exponential correction.");
        const auto c1Vec = instrument->getStringParameter("C1");
        if (c1Vec.empty())
          throw std::runtime_error(
              "Could not find parameter 'C1' in parameter "
              "file. Cannot apply exponential correction.");
        alg->setProperty("C0", c0Vec[0]);
        alg->setProperty("C1", c1Vec[0]);
      }
      alg->setProperty("NormalizeByIntegratedMonitors", false);
    } catch (std::runtime_error &e) {
      g_log.error() << e.what()
                    << ". Polynomial correction will not be performed.";
      alg->setProperty("CorrectionAlgorithm", "None");
    }
  } else {
    alg->setProperty("CorrectionAlgorithm", "None");
  }
}

/** Rebin and scale a workspace in Q.
*
* @param inputWS :: the workspace in Q
* @param theta :: the angle of this run
* @param params :: [output] rebin parameters
* @return :: the output workspace
*/
MatrixWorkspace_sptr
ReflectometryReductionOneAuto2::rebinAndScale(MatrixWorkspace_sptr inputWS,
                                              const double theta,
                                              std::vector<double> &params) {

  Property *qStepProp = getProperty("MomentumTransferStep");
  double qstep;
  if (!qStepProp->isDefault()) {
    qstep = getProperty("MomentumTransferStep");
    qstep = -qstep;
  } else {
    if (theta == 0.0) {
      throw std::runtime_error(
          "Theta determined from the detector positions is "
          "0.0. Please provide a value for theta manually "
          "or correct the detector position before running "
          "this algorithm.");
    }

    IAlgorithm_sptr calcRes = createChildAlgorithm("CalculateResolution");
    calcRes->setProperty("Workspace", inputWS);
    calcRes->setProperty("TwoTheta", theta);
    calcRes->execute();

    if (!calcRes->isExecuted()) {
      g_log.error("CalculateResolution failed. Workspace in Q will not be "
                  "rebinned. Please provide dQ/Q.");
      return inputWS;
    }
    qstep = calcRes->getProperty("Resolution");
    qstep = -qstep;
  }

  Property *qMin = getProperty("MomentumTransferMin");
  Property *qMax = getProperty("MomentumTransferMax");
  if (!qMin->isDefault() && !qMax->isDefault()) {
    double qmin = getProperty("MomentumTransferMin");
    double qmax = getProperty("MomentumTransferMax");
    params.push_back(qmin);
    params.push_back(qstep);
    params.push_back(qmax);
  } else {
    params.push_back(qstep);
  }

  // Rebin
  IAlgorithm_sptr algRebin = createChildAlgorithm("Rebin");
  algRebin->initialize();
  algRebin->setProperty("InputWorkspace", inputWS);
  algRebin->setProperty("OutputWorkspace", inputWS);
  algRebin->setProperty("Params", params);
  algRebin->execute();
  MatrixWorkspace_sptr IvsQ = algRebin->getProperty("OutputWorkspace");

  // Scale (optional)
  Property *scaleProp = getProperty("ScaleFactor");
  if (!scaleProp->isDefault()) {
    double scaleFactor = getProperty("ScaleFactor");
    IAlgorithm_sptr algScale = createChildAlgorithm("Scale");
    algScale->initialize();
    algScale->setProperty("InputWorkspace", IvsQ);
    algScale->setProperty("OutputWorkspace", IvsQ);
    algScale->setProperty("Factor", 1.0 / scaleFactor);
    algScale->execute();
    IvsQ = algScale->getProperty("OutputWorkspace");
  }

  return IvsQ;
}

/** Check if input workspace is a group
*/
bool ReflectometryReductionOneAuto2::checkGroups() {

  const std::string wsName = getPropertyValue("InputWorkspace");

  try {
    auto ws =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);
    if (ws)
      return true;
  } catch (...) {
  }
  return false;
}

/** Process groups. Groups are processed differently depending on transmission
 * runs and polarization analysis. If transmission run is a matrix workspace, it
 * will be applied to each of the members in the input workspace group. If
 * transmission run is a workspace group, the behaviour is different depending
 * on polarization analysis. If polarization analysis is off (i.e.
 * 'PolarizationAnalysis' is set to 'None') each item in the transmission group
 * is associated with the corresponding item in the input workspace group. If
 * polarization analysis is on (i.e. 'PolarizationAnalysis' is 'PA' or 'PNR')
 * items in the transmission group will be summed to produce a matrix workspace
 * that will be applied to each of the items in the input workspace group. See
 * documentation of this algorithm for more details.
*/
bool ReflectometryReductionOneAuto2::processGroups() {
  // this algorithm effectively behaves as MultiPeriodGroupAlgorithm
  m_usingBaseProcessGroups = true;

  // Get our input workspace group
  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      getPropertyValue("InputWorkspace"));
  // Get name of IvsQ workspace (native binning)
  const std::string outputIvsQ = getPropertyValue("OutputWorkspace");
  // Get name of IvsQ (native binning) workspace
  const std::string outputIvsQBinned =
      getPropertyValue("OutputWorkspaceBinned");
  // Get name of IvsLam workspace
  const std::string outputIvsLam =
      getPropertyValue("OutputWorkspaceWavelength");

  // Create a copy of ourselves
  Algorithm_sptr alg =
      createChildAlgorithm(name(), -1, -1, isLogging(), version());
  alg->setChild(false);
  alg->setRethrows(true);

  // Copy all the non-workspace properties over
  const std::vector<Property *> props = getProperties();
  for (auto &prop : props) {
    if (prop) {
      IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
      if (!wsProp)
        alg->setPropertyValue(prop->name(), prop->value());
    }
  }

  const bool polarizationAnalysisOn =
      getPropertyValue("PolarizationAnalysis") != "None";

  // Check if the transmission runs are groups or not

  const std::string firstTrans = getPropertyValue("FirstTransmissionRun");
  WorkspaceGroup_sptr firstTransG;
  MatrixWorkspace_sptr firstTransSum;
  if (!firstTrans.empty()) {
    auto firstTransWS =
        AnalysisDataService::Instance().retrieveWS<Workspace>(firstTrans);
    firstTransG = boost::dynamic_pointer_cast<WorkspaceGroup>(firstTransWS);
    if (!firstTransG) {
      alg->setProperty("FirstTransmissionRun", firstTrans);
    } else if (polarizationAnalysisOn) {
      firstTransSum = sumTransmissionWorkspaces(firstTransG);
    }
  }
  const std::string secondTrans = getPropertyValue("SecondTransmissionRun");
  WorkspaceGroup_sptr secondTransG;
  MatrixWorkspace_sptr secondTransSum;
  if (!secondTrans.empty()) {
    auto secondTransWS =
        AnalysisDataService::Instance().retrieveWS<Workspace>(secondTrans);
    secondTransG = boost::dynamic_pointer_cast<WorkspaceGroup>(secondTransWS);
    if (!secondTransG) {
      alg->setProperty("SecondTransmissionRun", secondTrans);
    } else if (polarizationAnalysisOn) {
      secondTransSum = sumTransmissionWorkspaces(secondTransG);
    }
  }

  std::vector<std::string> IvsQGroup, IvsQUnbinnedGroup, IvsLamGroup;

  // Execute algorithm over each group member
  for (size_t i = 0; i < group->size(); ++i) {

    const std::string IvsQName = outputIvsQ + "_" + std::to_string(i + 1);
    const std::string IvsQBinnedName =
        outputIvsQBinned + "_" + std::to_string(i + 1);
    const std::string IvsLamName = outputIvsLam + "_" + std::to_string(i + 1);

    if (firstTransG) {
      if (!polarizationAnalysisOn)
        alg->setProperty("FirstTransmissionRun",
                         firstTransG->getItem(i)->getName());
      else
        alg->setProperty("FirstTransmissionRun", firstTransSum);
    }
    if (secondTransG) {
      if (!polarizationAnalysisOn)
        alg->setProperty("SecondTransmissionRun",
                         secondTransG->getItem(i)->getName());
      else
        alg->setProperty("SecondTransmissionRun", secondTransSum);
    }

    alg->setProperty("InputWorkspace", group->getItem(i)->getName());
    alg->setProperty("OutputWorkspace", IvsQName);
    alg->setProperty("OutputWorkspaceBinned", IvsQBinnedName);
    alg->setProperty("OutputWorkspaceWavelength", IvsLamName);
    alg->execute();

    IvsQGroup.push_back(IvsQName);
    IvsQUnbinnedGroup.push_back(IvsQBinnedName);
    IvsLamGroup.push_back(IvsLamName);
  }

  // Group the IvsQ and IvsLam workspaces
  Algorithm_sptr groupAlg = createChildAlgorithm("GroupWorkspaces");
  groupAlg->setChild(false);
  groupAlg->setRethrows(true);
  groupAlg->setProperty("InputWorkspaces", IvsLamGroup);
  groupAlg->setProperty("OutputWorkspace", outputIvsLam);
  groupAlg->execute();
  groupAlg->setProperty("InputWorkspaces", IvsQGroup);
  groupAlg->setProperty("OutputWorkspace", outputIvsQ);
  groupAlg->execute();
  groupAlg->setProperty("InputWorkspaces", IvsQUnbinnedGroup);
  groupAlg->setProperty("OutputWorkspace", outputIvsQBinned);
  groupAlg->execute();

  // Set other properties so they can be updated in the Reflectometry interface
  setPropertyValue("ThetaIn", alg->getPropertyValue("ThetaIn"));
  setPropertyValue("MomentumTransferMin",
                   alg->getPropertyValue("MomentumTransferMin"));
  setPropertyValue("MomentumTransferMax",
                   alg->getPropertyValue("MomentumTransferMax"));
  setPropertyValue("MomentumTransferStep",
                   alg->getPropertyValue("MomentumTransferStep"));
  setPropertyValue("ScaleFactor", alg->getPropertyValue("ScaleFactor"));

  if (!polarizationAnalysisOn) {
    // No polarization analysis. Reduction stops here
    setPropertyValue("OutputWorkspace", outputIvsQ);
    setPropertyValue("OutputWorkspaceBinned", outputIvsQBinned);
    setPropertyValue("OutputWorkspaceWavelength", outputIvsLam);
    return true;
  }

  if (!group->isMultiperiod()) {
    g_log.warning("Polarization corrections can only be performed on "
                  "multiperiod workspaces.");
    setPropertyValue("OutputWorkspace", outputIvsQ);
    setPropertyValue("OutputWorkspaceBinned", outputIvsQBinned);
    setPropertyValue("OutputWorkspaceWavelength", outputIvsLam);
    return true;
  }

  Algorithm_sptr polAlg = createChildAlgorithm("PolarizationCorrection");
  polAlg->setChild(false);
  polAlg->setRethrows(true);
  polAlg->setProperty("InputWorkspace", outputIvsLam);
  polAlg->setProperty("OutputWorkspace", outputIvsLam);
  polAlg->setProperty("PolarizationAnalysis",
                      getPropertyValue("PolarizationAnalysis"));
  polAlg->setProperty("CPp", getPropertyValue("CPp"));
  polAlg->setProperty("CRho", getPropertyValue("CRho"));
  polAlg->setProperty("CAp", getPropertyValue("CAp"));
  polAlg->setProperty("CAlpha", getPropertyValue("CAlpha"));
  polAlg->execute();

  // Now we've overwritten the IvsLam workspaces, we'll need to recalculate
  // the IvsQ ones
  alg->setProperty("FirstTransmissionRun", "");
  alg->setProperty("SecondTransmissionRun", "");
  alg->setProperty("CorrectionAlgorithm", "None");
  alg->setProperty("ThetaIn", Mantid::EMPTY_DBL());
  alg->setProperty("ProcessingInstructions", "0");
  for (size_t i = 0; i < group->size(); ++i) {
    const std::string IvsQName = outputIvsQ + "_" + std::to_string(i + 1);
    const std::string IvsQBinnedName =
        outputIvsQBinned + "_" + std::to_string(i + 1);
    const std::string IvsLamName = outputIvsLam + "_" + std::to_string(i + 1);
    alg->setProperty("InputWorkspace", IvsLamName);
    alg->setProperty("OutputWorkspace", IvsQName);
    alg->setProperty("OutputWorkspaceBinned", IvsQBinnedName);
    alg->setProperty("OutputWorkspaceWavelength", IvsLamName);
    alg->execute();
  }

  setPropertyValue("OutputWorkspace", outputIvsQ);
  setPropertyValue("OutputWorkspaceBinned", outputIvsQBinned);
  setPropertyValue("OutputWorkspaceWavelength", outputIvsLam);

  return true;
}

/**
* Sum transmission workspaces that belong to a workspace group
* @param transGroup : The transmission group containing the transmission runs
* @return :: A workspace pointer containing the sum of transmission workspaces
*/
MatrixWorkspace_sptr ReflectometryReductionOneAuto2::sumTransmissionWorkspaces(
    WorkspaceGroup_sptr &transGroup) {

  const std::string transSum = "trans_sum";
  Workspace_sptr sumWS = transGroup->getItem(0)->clone();

  /// For this step to appear in the history of the output workspaces I need to
  /// set child to false and work with the ADS
  auto plusAlg = createChildAlgorithm("Plus");
  plusAlg->setChild(false);
  plusAlg->initialize();

  for (size_t item = 1; item < transGroup->size(); item++) {
    plusAlg->setProperty("LHSWorkspace", sumWS);
    plusAlg->setProperty("RHSWorkspace", transGroup->getItem(item));
    plusAlg->setProperty("OutputWorkspace", transSum);
    plusAlg->execute();
    sumWS = AnalysisDataService::Instance().retrieve(transSum);
  }
  MatrixWorkspace_sptr result =
      boost::dynamic_pointer_cast<MatrixWorkspace>(sumWS);
  AnalysisDataService::Instance().remove(transSum);
  return result;
}

} // namespace Algorithms
} // namespace Mantid
