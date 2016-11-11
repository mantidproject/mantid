#include "MantidAlgorithms/ReflectometryReductionOneAuto2.h"
#include "MantidAlgorithms/BoostOptionalToAlgorithmProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Property.h"

#include <boost/algorithm/string.hpp>

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
  std::string firstStr = getPropertyValue("FirstTransmissionRun");
  if (!firstStr.empty()) {
    auto firstTransWS =
        AnalysisDataService::Instance().retrieveWS<Workspace>(firstStr);
    auto firstTransGroup =
        boost::dynamic_pointer_cast<WorkspaceGroup>(firstTransWS);
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
  std::string secondStr = getPropertyValue("SecondTransmissionRun");
  if (!secondStr.empty()) {
    auto secondTransWS =
        AnalysisDataService::Instance().retrieveWS<Workspace>(secondStr);
    auto secondTransGroup =
        boost::dynamic_pointer_cast<WorkspaceGroup>(secondTransWS);
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

  // Analysis mode
  std::vector<std::string> analysisMode{"PointDetectorAnalysis",
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

  // Output workspace in Q
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output workspace in Q");

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

  double wavMin = checkForMandatoryInstrumentDefault<double>(
      this, "WavelengthMin", instrument, "LambdaMin");
  alg->setProperty("WavelengthMin", wavMin);
  double wavMax = checkForMandatoryInstrumentDefault<double>(
      this, "WavelengthMax", instrument, "LambdaMax");
  alg->setProperty("WavelengthMax", wavMax);

  auto instructions = populateProcessingInstructions(alg, instrument, inputWS);

  // Now that we know the detectors of interest, we can move them if necessary
  // (i.e. if theta is given)
  if (!getPointerToProperty("ThetaIn")->isDefault())
    inputWS = correctDetectorPositions(instructions, inputWS);

  // Optional properties

  populateDirectBeamProperties(alg);
  populateMonitorProperties(alg, instrument);
  populateTransmissionProperties(alg, instrument);
  populateMomentumTransferProperties(alg);

  alg->setPropertyValue("NormalizeByIntegratedMonitors",
                        getPropertyValue("NormalizeByIntegratedMonitors"));

  alg->setProperty("InputWorkspace", inputWS);
  alg->execute();

  MatrixWorkspace_sptr IvsLam = alg->getProperty("OutputWorkspaceWavelength");
  MatrixWorkspace_sptr IvsQ = alg->getProperty("OutputWorkspace");

  setProperty("OutputWorkspaceWavelength", IvsLam);
  setProperty("OutputWorkspace", IvsQ);
}

/** Correct an instrument component vertically.
*
* @param instructions :: processing instructions defining detectors of interest
* @param inputWS :: the input workspace
* @return :: the corrected workspace
*/
MatrixWorkspace_sptr ReflectometryReductionOneAuto2::correctDetectorPositions(
    const std::string &instructions, MatrixWorkspace_sptr inputWS) {

  // First we need to figure out which of the instrument components we need to
  // move
  // We know the detectors of interest, which are specified via processing
  // instructions
  // So we will move the parent component of the detectors of interest

  std::vector<std::string> wsIndices;
  boost::split(wsIndices, instructions, boost::is_any_of(":,-"));
  // Set of comopnents
  std::set<std::string> detectors;

  for (const auto wsIndex : wsIndices) {

    size_t index = boost::lexical_cast<size_t>(wsIndex);

    auto detector = inputWS->getDetector(index);
    auto parent = detector->getParent();

    if (parent) {
      auto parentType = parent->type();
      auto detectorName = (parentType == "Instrument") ? detector->getName()
                                                       : parent->getName();
      detectors.insert(detectorName);
    }
  }

  double theta = getProperty("ThetaIn");

  MatrixWorkspace_sptr corrected = inputWS;

  for (const auto &detector : detectors) {
    IAlgorithm_sptr alg =
        createChildAlgorithm("SpecularReflectionPositionCorrect");
    alg->setProperty("InputWorkspace", corrected);
    alg->setProperty("TwoTheta", theta * 2);
    alg->setProperty("DetectorComponentName", detector);
    alg->execute();
    corrected = alg->getProperty("OutputWorkspace");
  }

  return corrected;
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

/** Set momentum transfer properties and scale factor
*
* @param alg :: ReflectometryReductionOne algorithm
*/
void ReflectometryReductionOneAuto2::populateMomentumTransferProperties(
    IAlgorithm_sptr alg) {

  alg->setPropertyValue("MomentumTransferMin",
                        getPropertyValue("MomentumTransferMin"));
  alg->setPropertyValue("MomentumTransferMax",
                        getPropertyValue("MomentumTransferMax"));
  alg->setPropertyValue("MomentumTransferStep",
                        getPropertyValue("MomentumTransferStep"));
  alg->setPropertyValue("ScaleFactor", getPropertyValue("ScaleFactor"));
}

/** Set transmission properties
*
* @param alg :: ReflectometryReductionOne algorithm
* @param instrument :: the instrument attached to the workspace
*/
void ReflectometryReductionOneAuto2::populateTransmissionProperties(
    IAlgorithm_sptr alg, Instrument_const_sptr instrument) {

  // Transmission run(s)

  MatrixWorkspace_sptr firstWS = getProperty("FirstTransmissionRun");
  if (firstWS) {
    alg->setProperty("FirstTransmissionRun", firstWS);
    alg->setPropertyValue("StrictSpectrumChecking",
                          getPropertyValue("StrictSpectrumChecking"));
    MatrixWorkspace_sptr secondWS = getProperty("SecondTransmissionRun");
    if (secondWS) {
      alg->setProperty("SecondTransmissionRun", secondWS);
      alg->setPropertyValue("StartOverlap", getPropertyValue("StartOverlap"));
      alg->setPropertyValue("EndOverlap", getPropertyValue("EndOverlap"));
      alg->setPropertyValue("Params", getPropertyValue("Params"));
    }
    return;
  }

  // No transmission runs, try algorithmic corrections

  std::string correctionAlgorithm = getProperty("CorrectionAlgorithm");

  if (correctionAlgorithm == "PolynomialCorrection") {
    alg->setProperty("CorrectionAlgorithm", "PolynomialCorrection");
    alg->setPropertyValue("Polynomial", getPropertyValue("Polynomial"));

  } else if (correctionAlgorithm == "ExponentialCorrection") {
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
    } catch (std::runtime_error &e) {
      g_log.error() << e.what()
                    << ". Polynomial correction will not be performed.";
      alg->setProperty("CorrectionAlgorithm", "None");
    }
  } else {
    alg->setProperty("CorrectionAlgorithm", "None");
  }
}

/** Check if input workspace is a group
*/
bool ReflectometryReductionOneAuto2::checkGroups() {

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
  // Get name of IvsQ workspace
  const std::string outputIvsQ = getPropertyValue("OutputWorkspace");
  // Get name of IvsLam workspace
  const std::string outputIvsLam =
      getPropertyValue("OutputWorkspaceWavelength");

  // Create a copy of ourselves
  Algorithm_sptr alg =
      createChildAlgorithm(name(), -1, -1, isLogging(), version());
  alg->setChild(false);
  alg->setRethrows(true);

  // Copy all the non-workspace properties over
  std::vector<Property *> props = getProperties();
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
  if (!firstTrans.empty()) {
    auto firstTransWS =
        AnalysisDataService::Instance().retrieveWS<Workspace>(firstTrans);
    firstTransG = boost::dynamic_pointer_cast<WorkspaceGroup>(firstTransWS);
    if (!firstTransG) {
      alg->setProperty("FirstTransmissionRun", firstTrans);
    }
  }
  const std::string secondTrans = getPropertyValue("SecondTransmissionRun");
  WorkspaceGroup_sptr secondTransG;
  if (!secondTrans.empty()) {
    auto secondTransWS =
        AnalysisDataService::Instance().retrieveWS<Workspace>(secondTrans);
    secondTransG = boost::dynamic_pointer_cast<WorkspaceGroup>(secondTransWS);
    if (!secondTransG) {
      alg->setProperty("SecondTransmissionRun", secondTrans);
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
    if (firstTransG && polarizationAnalysisOn) {
      auto firstTransmissionSum = sumTransmissionWorkspaces(firstTransG);
      alg->setProperty("FirstTransmissionRun", firstTransmissionSum);
    }
    if (secondTransG && polarizationAnalysisOn) {
      auto secondTransmissionSum = sumTransmissionWorkspaces(secondTransG);
      alg->setProperty("SecondTransmissionRun", secondTransmissionSum);
    }

    // Otherwise, if polarization correction is off, we process them
    // using one transmission group member at a time.
    if (firstTransG && !polarizationAnalysisOn)
      alg->setProperty("FirstTransmissionRun", firstTransG->getItem(i)->name());
    if (secondTransG && !polarizationAnalysisOn)
      alg->setProperty("SecondTransmissionRun",
                       secondTransG->getItem(i)->name());

    alg->setProperty("InputWorkspace", group->getItem(i)->name());
    alg->setProperty("OutputWorkspace", IvsQName);
    alg->setProperty("OutputWorkspaceWavelength", IvsLamName);
    alg->execute();

    MatrixWorkspace_sptr tempFirstTransWS =
        alg->getProperty("FirstTransmissionRun");

    IvsQGroup.push_back(IvsQName);
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

  if (!polarizationAnalysisOn) {
    // No polarization analysis. Reduction stops here
    setPropertyValue("OutputWorkspace", outputIvsQ);
    setPropertyValue("OutputWorkspaceWavelength", outputIvsLam);
    return true;
  }

  if (!group->isMultiperiod()) {
    g_log.warning("Polarization corrections can only be performed on "
                  "multiperiod workspaces.");
    setPropertyValue("OutputWorkspace", outputIvsQ);
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
  for (size_t i = 0; i < numMembers; ++i) {
    const std::string IvsQName = outputIvsQ + "_" + std::to_string(i + 1);
    const std::string IvsLamName = outputIvsLam + "_" + std::to_string(i + 1);
    alg->setProperty("InputWorkspace", IvsLamName);
    alg->setProperty("OutputWorkspace", IvsQName);
    alg->setProperty("OutputWorkspaceWavelength", IvsLamName);
    alg->execute();
  }

  setPropertyValue("OutputWorkspace", outputIvsQ);
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

  std::string transSum = "trans_sum";
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
