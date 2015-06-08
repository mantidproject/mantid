#include "MantidWorkflowAlgorithms/DgsConvertToEnergyTransfer.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidWorkflowAlgorithms/WorkflowAlgorithmHelpers.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace WorkflowAlgorithmHelpers;

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DgsConvertToEnergyTransfer)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
DgsConvertToEnergyTransfer::DgsConvertToEnergyTransfer() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
DgsConvertToEnergyTransfer::~DgsConvertToEnergyTransfer() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string DgsConvertToEnergyTransfer::name() const {
  return "DgsConvertToEnergyTransfer";
}

/// Algorithm's version for identification. @see Algorithm::version
int DgsConvertToEnergyTransfer::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DgsConvertToEnergyTransfer::category() const {
  return "Workflow\\Inelastic\\UsesPropertyManager";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DgsConvertToEnergyTransfer::init() {
  this->declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "A sample data workspace.");
  this->declareProperty(
      new WorkspaceProperty<>("InputMonitorWorkspace", "", Direction::Input,
                              PropertyMode::Optional),
      "A monitor workspace associated with the sample workspace.");
  this->declareProperty(
      "IncidentEnergyGuess", EMPTY_DBL(),
      "This is the starting point for the incident energy calculation.");
  this->declareProperty(new WorkspaceProperty<>("IntegratedDetectorVanadium",
                                                "", Direction::Input,
                                                PropertyMode::Optional),
                        "A workspace containing the "
                        "integrated detector vanadium.");
  this->declareProperty(
      new WorkspaceProperty<MatrixWorkspace>(
          "MaskWorkspace", "", Direction::Input, PropertyMode::Optional),
      "A mask workspace");
  this->declareProperty(
      new WorkspaceProperty<MatrixWorkspace>(
          "GroupingWorkspace", "", Direction::Input, PropertyMode::Optional),
      "A grouping workspace");
  this->declareProperty(
      "AlternateGroupingTag", "",
      "Allows modification to the OldGroupingFile property name");
  this->declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name for the output workspace.");
  this->declareProperty(
      new WorkspaceProperty<>("OutputTibWorkspace", "", Direction::Output),
      "The name for the output TIB workspace.");
  this->declareProperty("ReductionProperties", "__dgs_reduction_properties",
                        Direction::Input);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DgsConvertToEnergyTransfer::exec() {
  g_log.notice() << "Starting DgsConvertToEnergyTransfer" << std::endl;
  // Get the reduction property manager
  const std::string reductionManagerName =
      this->getProperty("ReductionProperties");
  boost::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    reductionManager =
        PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    throw std::runtime_error("DgsConvertToEnergyTransfer cannot run without a "
                             "reduction PropertyManager.");
  }

  MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = this->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr monWS = this->getProperty("InputMonitorWorkspace");

  // Make a monitor workspace name for SNS data
  std::string monWsName = inputWS->getName() + "_monitors";
  bool preserveEvents = false;

  // Calculate the initial energy and time zero
  const std::string facility = ConfigService::Instance().getFacility().name();
  g_log.notice() << "Processing for " << facility << std::endl;
  double eiGuess = this->getProperty("IncidentEnergyGuess");
  if (EMPTY_DBL() == eiGuess) {
    // SNS has a log called EnergyRequest that can be used to get the
    // incident energy guess.
    if ("SNS" == facility) {
      TimeSeriesProperty<double> *eiLog =
          inputWS->run().getTimeSeriesProperty<double>("EnergyRequest");
      eiGuess = eiLog->getStatistics().mean;
    } else {
      throw std::runtime_error("Incident energy guess MUST be given!");
    }
  }
  const bool useEiGuess =
      reductionManager->getProperty("UseIncidentEnergyGuess");
  const double tZeroGuess = reductionManager->getProperty("TimeZeroGuess");
  std::vector<double> etBinning =
      reductionManager->getProperty("EnergyTransferRange");

  // Create a default set of binning parameters: (-0.5Ei, 0.01Ei, 0.99Ei)
  if (etBinning.empty()) {
    double emin = -0.5 * eiGuess;
    double deltaE = 0.01 * eiGuess;
    double emax = 0.99 * eiGuess;
    etBinning.push_back(emin);
    etBinning.push_back(deltaE);
    etBinning.push_back(emax);
  }

  double incidentEnergy = 0.0;
  double monPeak = 0.0;
  specid_t eiMon1Spec =
      static_cast<specid_t>(reductionManager->getProperty("Monitor1SpecId"));
  specid_t eiMon2Spec =
      static_cast<specid_t>(reductionManager->getProperty("Monitor2SpecId"));

  if ("SNS" == facility) {
    // SNS wants to preserve events until the last
    preserveEvents = true;
    double tZero = 0.0;
    if (useEiGuess) {
      incidentEnergy = eiGuess;
      if (EMPTY_DBL() != tZeroGuess) {
        tZero = tZeroGuess;
      }
    } else {
      if (!monWS) {
        g_log.notice() << "Trying to determine file name" << std::endl;
        std::string runFileName =
            inputWS->run().getProperty("Filename")->value();
        if (runFileName.empty()) {
          throw std::runtime_error("Cannot find run filename, therefore cannot "
                                   "find the initial energy");
        }

        std::string loadAlgName("");
        std::string fileProp("");
        if (boost::ends_with(runFileName, "_event.nxs") ||
            boost::ends_with(runFileName, ".nxs.h5") ||
            boost::ends_with(runFileName, ".nxs")) {
          g_log.notice() << "Loading NeXus monitors" << std::endl;
          loadAlgName = "LoadNexusMonitors";
          fileProp = "Filename";
        }

        if (boost::ends_with(runFileName, "_neutron_event.dat")) {
          g_log.notice() << "Loading PreNeXus monitors" << std::endl;
          loadAlgName = "LoadPreNexusMonitors";
          boost::replace_first(runFileName, "_neutron_event.dat",
                               "_runinfo.xml");
          fileProp = "RunInfoFilename";
        }

        // Load the monitors
        IAlgorithm_sptr loadmon = this->createChildAlgorithm(loadAlgName);
        loadmon->setProperty(fileProp, runFileName);
        loadmon->setProperty("OutputWorkspace", monWsName);
        loadmon->executeAsChildAlg();
        Workspace_sptr monWSOutput = loadmon->getProperty("OutputWorkspace");
        // the algorithm can return a group workspace if the file is multi period
        monWS = boost::dynamic_pointer_cast<MatrixWorkspace>(monWSOutput);
        if ((monWSOutput) && (!monWS)) {
          //this was a group workspace - DGSReduction does not support multi period data yet
          throw Exception::NotImplementedError(
            "The file contains multi period data, support for this is not implemented in DGSReduction yet");
        }
      }

      // Calculate Ei
      IAlgorithm_sptr getei = this->createChildAlgorithm("GetEi");
      getei->setProperty("InputWorkspace", monWS);
      getei->setProperty("Monitor1Spec", eiMon1Spec);
      getei->setProperty("Monitor2Spec", eiMon2Spec);
      getei->setProperty("EnergyEstimate", eiGuess);
      getei->executeAsChildAlg();
      incidentEnergy = getei->getProperty("IncidentEnergy");
      tZero = getei->getProperty("Tzero");
    }

    g_log.notice() << "Adjusting for T0" << std::endl;
    IAlgorithm_sptr alg = this->createChildAlgorithm("ChangeBinOffset");
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("OutputWorkspace", outputWS);
    alg->setProperty("Offset", -tZero);
    alg->executeAsChildAlg();
    outputWS = alg->getProperty("OutputWorkspace");

    // Add T0 to sample logs
    IAlgorithm_sptr addLog = this->createChildAlgorithm("AddSampleLog");
    addLog->setProperty("Workspace", outputWS);
    addLog->setProperty("LogName", "CalculatedT0");
    addLog->setProperty("LogType", "Number");
    addLog->setProperty("LogText", boost::lexical_cast<std::string>(tZero));
    addLog->executeAsChildAlg();
  }
  // Do ISIS
  else {
    IAlgorithm_sptr getei = this->createChildAlgorithm("GetEi");
    getei->setProperty("InputWorkspace", inputWS);
    getei->setProperty("Monitor1Spec", eiMon1Spec);
    getei->setProperty("Monitor2Spec", eiMon2Spec);
    getei->setProperty("EnergyEstimate", eiGuess);
    getei->executeAsChildAlg();

    monPeak = getei->getProperty("FirstMonitorPeak");
    const specid_t monIndex =
        static_cast<const specid_t>(getei->getProperty("FirstMonitorIndex"));
    // Why did the old way get it from the log?
    incidentEnergy = getei->getProperty("IncidentEnergy");

    IAlgorithm_sptr cbo = this->createChildAlgorithm("ChangeBinOffset");
    cbo->setProperty("InputWorkspace", inputWS);
    cbo->setProperty("OutputWorkspace", outputWS);
    cbo->setProperty("Offset", -monPeak);
    cbo->executeAsChildAlg();
    outputWS = cbo->getProperty("OutputWorkspace");

    IDetector_const_sptr monDet = inputWS->getDetector(monIndex);
    V3D monPos = monDet->getPos();
    std::string srcName = inputWS->getInstrument()->getSource()->getName();

    IAlgorithm_sptr moveInstComp =
        this->createChildAlgorithm("MoveInstrumentComponent");
    moveInstComp->setProperty("Workspace", outputWS);
    moveInstComp->setProperty("ComponentName", srcName);
    moveInstComp->setProperty("X", monPos.X());
    moveInstComp->setProperty("Y", monPos.Y());
    moveInstComp->setProperty("Z", monPos.Z());
    moveInstComp->setProperty("RelativePosition", false);
    moveInstComp->executeAsChildAlg();
    outputWS = moveInstComp->getProperty("Workspace");
  }

  const double binOffset = -monPeak;

  if ("ISIS" == facility) {
    std::string detcalFile("");
    if (reductionManager->existsProperty("SampleDetCalFilename")) {
      detcalFile = reductionManager->getPropertyValue("SampleDetCalFilename");
    }
    // Try to get it from run object.
    else {
      detcalFile = inputWS->run().getProperty("Filename")->value();
    }
    if (!detcalFile.empty()) {
      const bool relocateDets =
          reductionManager->getProperty("RelocateDetectors");
      IAlgorithm_sptr loaddetinfo =
          this->createChildAlgorithm("LoadDetectorInfo");
      loaddetinfo->setProperty("Workspace", outputWS);
      loaddetinfo->setProperty("DataFilename", detcalFile);
      loaddetinfo->setProperty("RelocateDets", relocateDets);
      loaddetinfo->executeAsChildAlg();
      outputWS = loaddetinfo->getProperty("Workspace");
    } else {
      throw std::runtime_error(
          "Cannot find detcal filename in run object or as parameter.");
    }
  }

  // Subtract time-independent background if necessary
  const bool doTibSub = reductionManager->getProperty("TimeIndepBackgroundSub");
  if (doTibSub) {
    // Setup for later use
    IAlgorithm_sptr cnvToDist =
        this->createChildAlgorithm("ConvertToDistribution");

    // Set the binning parameters for the background region
    double tibTofStart = getDblPropOrParam("TibTofRangeStart", reductionManager,
                                           "bkgd-range-min", inputWS);
    tibTofStart += binOffset;
    double tibTofEnd = getDblPropOrParam("TibTofRangeEnd", reductionManager,
                                         "bkgd-range-max", inputWS);
    tibTofEnd += binOffset;
    const double tibTofWidth = tibTofEnd - tibTofStart;
    std::vector<double> params;
    params.push_back(tibTofStart);
    params.push_back(tibTofWidth);
    params.push_back(tibTofEnd);

    bool treatTibAsEvents = false;

    // Do we want to treat the TIB as events
    std::vector<std::string> backgroundType =
        inputWS->getInstrument()->getStringParameter(
            "treat-background-as-events");
    if (backgroundType.empty()) {
      // Set the default behaviour.
      treatTibAsEvents = false;
    } else {
      if ("yes" == backgroundType[0] || "true" == backgroundType[0]) {
        treatTibAsEvents = true;
      }
    }

    if ("SNS" == facility) {
      MatrixWorkspace_sptr bkgWS;

      // Do we want to treat the constant background as events ?
      if (treatTibAsEvents) {
        g_log.notice("TIB removal using event mode.");
        // Treat background as events
        IAlgorithm_sptr createBkg =
            this->createChildAlgorithm("CreateFlatEventWorkspace");
        createBkg->setProperty("InputWorkspace", outputWS);
        createBkg->setProperty("RangeStart", tibTofStart);
        createBkg->setProperty("RangeEnd", tibTofEnd);
        createBkg->executeAsChildAlg();
        bkgWS = createBkg->getProperty("OutputWorkspace");
      } else {
        g_log.notice("TIB removal using legacy mode.");
        // Create an original background workspace from a portion of the
        // result workspace.
        std::string origBkgWsName = "background_origin_ws";
        IAlgorithm_sptr rebin = this->createChildAlgorithm("Rebin");
        rebin->setProperty("InputWorkspace", outputWS);
        rebin->setProperty("OutputWorkspace", origBkgWsName);
        rebin->setProperty("Params", params);
        rebin->setProperty("PreserveEvents", false);
        rebin->executeAsChildAlg();
        MatrixWorkspace_sptr origBkgWS = rebin->getProperty("OutputWorkspace");

        // Convert result workspace to DeltaE since we have Et binning
        IAlgorithm_sptr cnvun = this->createChildAlgorithm("ConvertUnits");
        cnvun->setProperty("InputWorkspace", outputWS);
        cnvun->setProperty("OutputWorkspace", outputWS);
        cnvun->setProperty("Target", "DeltaE");
        cnvun->setProperty("EMode", "Direct");
        cnvun->setProperty("EFixed", incidentEnergy);
        cnvun->executeAsChildAlg();
        outputWS = cnvun->getProperty("OutputWorkspace");

        // Rebin to Et
        rebin->setProperty("InputWorkspace", outputWS);
        rebin->setProperty("OutputWorkspace", outputWS);
        rebin->setProperty("Params", etBinning);
        rebin->setProperty("PreserveEvents", false);
        rebin->executeAsChildAlg();
        outputWS = rebin->getProperty("OutputWorkspace");

        // Convert result workspace to TOF
        cnvun->setProperty("InputWorkspace", outputWS);
        cnvun->setProperty("OutputWorkspace", outputWS);
        cnvun->setProperty("Target", "TOF");
        cnvun->setProperty("EMode", "Direct");
        cnvun->setProperty("EFixed", incidentEnergy);
        cnvun->executeAsChildAlg();
        outputWS = cnvun->getProperty("OutputWorkspace");

        // Make result workspace a distribution
        cnvToDist->setProperty("Workspace", outputWS);
        cnvToDist->executeAsChildAlg();
        outputWS = cnvToDist->getProperty("Workspace");

        // Calculate the background
        IAlgorithm_sptr flatBg =
            this->createChildAlgorithm("CalculateFlatBackground");
        flatBg->setProperty("InputWorkspace", origBkgWS);
        flatBg->setProperty("StartX", tibTofStart);
        flatBg->setProperty("EndX", tibTofEnd);
        flatBg->setProperty("Mode", "Mean");
        flatBg->setProperty("OutputMode", "Return Background");
        flatBg->executeAsChildAlg();
        bkgWS = flatBg->getProperty("OutputWorkspace");

        // Remove unneeded original background workspace
        origBkgWS.reset();

        // Make background workspace a distribution
        cnvToDist->setProperty("Workspace", bkgWS);
        cnvToDist->executeAsChildAlg();
        bkgWS = cnvToDist->getProperty("Workspace");
      }

      // Subtract background from result workspace
      IAlgorithm_sptr minus = this->createChildAlgorithm("Minus");
      minus->setProperty("LHSWorkspace", outputWS);
      minus->setProperty("RHSWorkspace", bkgWS);
      minus->setProperty("OutputWorkspace", outputWS);
      minus->executeAsChildAlg();

      this->setProperty("OutputTibWorkspace", bkgWS);
    }
    // Do ISIS
    else {
      // Make result workspace a distribution
      cnvToDist->setProperty("Workspace", outputWS);
      cnvToDist->executeAsChildAlg();
      outputWS = cnvToDist->getProperty("Workspace");

      IAlgorithm_sptr flatBg =
          this->createChildAlgorithm("CalculateFlatBackground");
      flatBg->setProperty("InputWorkspace", outputWS);
      flatBg->setProperty("OutputWorkspace", outputWS);
      flatBg->setProperty("StartX", tibTofStart);
      flatBg->setProperty("EndX", tibTofEnd);
      flatBg->setProperty("Mode", "Mean");
      flatBg->executeAsChildAlg();
      outputWS = flatBg->getProperty("OutputWorkspace");
    }

    if (!treatTibAsEvents) {
      // Convert result workspace back to histogram
      IAlgorithm_sptr cnvFrDist =
          this->createChildAlgorithm("ConvertFromDistribution");
      cnvFrDist->setProperty("Workspace", outputWS);
      cnvFrDist->executeAsChildAlg();
      outputWS = cnvFrDist->getProperty("Workspace");
    }
  }

  // Normalise result workspace to incident beam parameter
  IAlgorithm_sptr norm = this->createChildAlgorithm("DgsPreprocessData");
  norm->setProperty("InputWorkspace", outputWS);
  norm->setProperty("OutputWorkspace", outputWS);
  norm->setProperty("InputMonitorWorkspace", monWS);
  norm->setProperty("TofRangeOffset", binOffset);
  norm->executeAsChildAlg();
  outputWS = norm->getProperty("OutputWorkspace");

  // Convert to energy transfer
  g_log.notice() << "Converting to energy transfer." << std::endl;
  IAlgorithm_sptr cnvun = this->createChildAlgorithm("ConvertUnits");
  cnvun->setProperty("InputWorkspace", outputWS);
  cnvun->setProperty("OutputWorkspace", outputWS);
  cnvun->setProperty("Target", "DeltaE");
  cnvun->setProperty("EMode", "Direct");
  cnvun->setProperty("EFixed", incidentEnergy);
  cnvun->executeAsChildAlg();
  outputWS = cnvun->getProperty("OutputWorkspace");

  g_log.notice() << "Rebinning data" << std::endl;
  IAlgorithm_sptr rebin = this->createChildAlgorithm("Rebin");
  rebin->setProperty("InputWorkspace", outputWS);
  rebin->setProperty("OutputWorkspace", outputWS);
  rebin->setProperty("Params", etBinning);
  rebin->setProperty("PreserveEvents", preserveEvents);
  rebin->executeAsChildAlg();
  outputWS = rebin->getProperty("OutputWorkspace");

  // Correct for detector efficiency
  if ("SNS" == facility) {
    // He3TubeEfficiency requires the workspace to be in wavelength
    cnvun->setProperty("InputWorkspace", outputWS);
    cnvun->setProperty("OutputWorkspace", outputWS);
    cnvun->setProperty("Target", "Wavelength");
    cnvun->executeAsChildAlg();
    outputWS = cnvun->getProperty("OutputWorkspace");

    // Do the correction
    IAlgorithm_sptr alg2 = this->createChildAlgorithm("He3TubeEfficiency");
    alg2->setProperty("InputWorkspace", outputWS);
    alg2->setProperty("OutputWorkspace", outputWS);
    alg2->executeAsChildAlg();
    outputWS = alg2->getProperty("OutputWorkspace");

    // Convert back to energy transfer
    cnvun->setProperty("InputWorkspace", outputWS);
    cnvun->setProperty("OutputWorkspace", outputWS);
    cnvun->setProperty("Target", "DeltaE");
    cnvun->executeAsChildAlg();
    outputWS = cnvun->getProperty("OutputWorkspace");
  }
  // Do ISIS
  else {
    IAlgorithm_sptr alg = this->createChildAlgorithm("DetectorEfficiencyCor");
    alg->setProperty("InputWorkspace", outputWS);
    alg->setProperty("OutputWorkspace", outputWS);
    alg->executeAsChildAlg();
    outputWS = alg->getProperty("OutputWorkspace");
  }

  const bool correctKiKf = reductionManager->getProperty("CorrectKiKf");
  if (correctKiKf) {
    // Correct for Ki/Kf
    IAlgorithm_sptr kikf = this->createChildAlgorithm("CorrectKiKf");
    kikf->setProperty("InputWorkspace", outputWS);
    kikf->setProperty("OutputWorkspace", outputWS);
    kikf->setProperty("EMode", "Direct");
    kikf->executeAsChildAlg();
    outputWS = kikf->getProperty("OutputWorkspace");
  }

  // Rebin to ensure consistency
  const bool sofphieIsDistribution =
      reductionManager->getProperty("SofPhiEIsDistribution");

  g_log.notice() << "Rebinning data" << std::endl;
  rebin->setProperty("InputWorkspace", outputWS);
  rebin->setProperty("OutputWorkspace", outputWS);
  if (sofphieIsDistribution) {
    rebin->setProperty("PreserveEvents", false);
  }
  rebin->executeAsChildAlg();
  outputWS = rebin->getProperty("OutputWorkspace");

  if (sofphieIsDistribution) {
    g_log.notice() << "Making distribution" << std::endl;
    IAlgorithm_sptr distrib =
        this->createChildAlgorithm("ConvertToDistribution");
    distrib->setProperty("Workspace", outputWS);
    distrib->executeAsChildAlg();
    outputWS = distrib->getProperty("Workspace");
  } else {
    // Discard events outside nominal bounds
    IAlgorithm_sptr crop = this->createChildAlgorithm("CropWorkspace");
    crop->setProperty("InputWorkspace", outputWS);
    crop->setProperty("OutputWorkspace", outputWS);
    crop->setProperty("XMin", etBinning[0]);
    crop->setProperty("XMax", etBinning[2]);
    crop->executeAsChildAlg();
    outputWS = crop->getProperty("OutputWorkspace");
  }

  // Normalise by the detector vanadium if necessary
  MatrixWorkspace_sptr detVanWS =
      this->getProperty("IntegratedDetectorVanadium");
  if (detVanWS) {
    IAlgorithm_sptr divide = this->createChildAlgorithm("Divide");
    divide->setProperty("LHSWorkspace", outputWS);
    divide->setProperty("RHSWorkspace", detVanWS);
    divide->setProperty("OutputWorkspace", outputWS);
    divide->executeAsChildAlg();
    outputWS = divide->getProperty("OutputWorkspace");
  }

  // Mask and group workspace if necessary.
  MatrixWorkspace_sptr maskWS = this->getProperty("MaskWorkspace");
  MatrixWorkspace_sptr groupWS = this->getProperty("GroupingWorkspace");
  std::string oldGroupFile("");
  std::string filePropMod = this->getProperty("AlternateGroupingTag");
  std::string fileProp = filePropMod + "OldGroupingFilename";
  if (reductionManager->existsProperty(fileProp)) {
    oldGroupFile = reductionManager->getPropertyValue(fileProp);
  }
  IAlgorithm_sptr remap = this->createChildAlgorithm("DgsRemap");
  remap->setProperty("InputWorkspace", outputWS);
  remap->setProperty("OutputWorkspace", outputWS);
  remap->setProperty("MaskWorkspace", maskWS);
  remap->setProperty("GroupingWorkspace", groupWS);
  remap->setProperty("OldGroupingFile", oldGroupFile);
  remap->executeAsChildAlg();
  outputWS = remap->getProperty("OutputWorkspace");

  if ("ISIS" == facility) {
    double scaleFactor =
        inputWS->getInstrument()->getNumberParameter("scale-factor")[0];
    const std::string scaleFactorName = "ScaleFactor";
    IAlgorithm_sptr csvw =
        this->createChildAlgorithm("CreateSingleValuedWorkspace");
    csvw->setProperty("OutputWorkspace", scaleFactorName);
    csvw->setProperty("DataValue", scaleFactor);
    csvw->executeAsChildAlg();
    MatrixWorkspace_sptr scaleFactorWS = csvw->getProperty("OutputWorkspace");

    IAlgorithm_sptr mult = this->createChildAlgorithm("Multiply");
    mult->setProperty("LHSWorkspace", outputWS);
    mult->setProperty("RHSWorkspace", scaleFactorWS);
    mult->setProperty("OutputWorkspace", outputWS);
    mult->executeAsChildAlg();
  }

  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid
} // namespace WorkflowAlgorithms
