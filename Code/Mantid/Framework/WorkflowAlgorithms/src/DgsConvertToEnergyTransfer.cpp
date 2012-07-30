/*WIKI*

This algorithm is responsible for making the conversion from time-of-flight to
energy transfer for direct geometry spectrometers.

*WIKI*/

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
#include "MantidKernel/VisibleWhenProperty.h"

#include <boost/algorithm/string.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace WorkflowAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DgsConvertToEnergyTransfer)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DgsConvertToEnergyTransfer::DgsConvertToEnergyTransfer()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DgsConvertToEnergyTransfer::~DgsConvertToEnergyTransfer()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string DgsConvertToEnergyTransfer::name() const { return "DgsConvertToEnergyTransfer"; };
  
  /// Algorithm's version for identification. @see Algorithm::version
  int DgsConvertToEnergyTransfer::version() const { return 1; };
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string DgsConvertToEnergyTransfer::category() const { return "Workflow\\Inelastic"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void DgsConvertToEnergyTransfer::initDocs()
  {
    this->setWikiSummary("Algorithm to convert from TOF to energy transfer.");
    this->setOptionalMessage("Algorithm to convert from TOF to energy transfer.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void DgsConvertToEnergyTransfer::init()
  {
    this->declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
        "A sample data workspace.");
    this->declareProperty(new WorkspaceProperty<>("IntegratedDetectorVanadium", "",
        Direction::Input, PropertyMode::Optional), "A workspace containing the "
            "integrated detector vanadium.");
    this->declareProperty(new WorkspaceProperty<>("OutputWorkspace", "",
        Direction::Output, PropertyMode::Optional));
    this->declareProperty("ReductionProperties", "__dgs_reduction_properties", Direction::Input);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DgsConvertToEnergyTransfer::exec()
  {
    g_log.notice() << "Starting DgsConvertToEnergyTransfer" << std::endl;
    // Get the reduction property manager
    const std::string reductionManagerName = this->getProperty("ReductionProperties");
    boost::shared_ptr<PropertyManager> reductionManager;
    if (PropertyManagerDataService::Instance().doesExist(reductionManagerName))
      {
        reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
      }
    else
      {
        throw std::runtime_error("DgsConvertToEnergyTransfer cannot run without a reduction PropertyManager.");
      }

    this->enableHistoryRecordingForChild(true);

    MatrixWorkspace_const_sptr inputWS = this->getProperty("InputWorkspace");
    const std::string inWsName = inputWS->getName();
    // Make the result workspace name
    std::string outWsName = inWsName + "_et";
    // Make a monitor workspace name for SNS data
    std::string monWsName = inWsName + "_monitors";
    bool preserveEvents = false;

    // Calculate the initial energy and time zero
    const std::string facility = ConfigService::Instance().getFacility().name();
    g_log.notice() << "Processing for " << facility << std::endl;
    const double eiGuess = reductionManager->getProperty("IncidentEnergyGuess");
    const bool useEiGuess = reductionManager->getProperty("UseIncidentEnergyGuess");
    const std::vector<double> etBinning = reductionManager->getProperty("EnergyTransferRange");

    double incidentEnergy = 0.0;
    double monPeak = 0.0;
    specid_t eiMon1Spec = 0;
    specid_t eiMon2Spec = 0;

    if ("SNS" == facility)
      {
        // SNS wants to preserve events until the last
        preserveEvents = true;
        const std::string instName = inputWS->getInstrument()->getName();
        double tZero = 0.0;
        if ("CNCS" == instName || "HYSPEC" == instName)
          {
            incidentEnergy = eiGuess;
            if ("HYSPEC" == instName)
              {
                double powVal = incidentEnergy / 27.0;
                tZero = 25.0 + 85.0 / (1.0 + std::pow(powVal, 4));
              }
            // Do CNCS
            else
              {
                double powVal = 1.0 + incidentEnergy;
                tZero = (0.1982 * std::pow(powVal, -0.84098)) * 1000.0;
              }
          }
        // Do ARCS and SEQUOIA
        else
          {
            if (useEiGuess)
              {
                incidentEnergy = eiGuess;
              }
            else
              {
                g_log.notice() << "Trying to determine file name" << std::endl;
                std::string runFileName("");
                if (reductionManager->existsProperty("MonitorFilename"))
                  {
                    runFileName = reductionManager->getPropertyValue("MonitorFilename");
                    if (runFileName.empty())
                      {
                        throw std::runtime_error("Cannot find run filename, therefore cannot find the initial energy");
                      }
                  }
                else
                  {
                    throw std::runtime_error("Input workspaces are not handled, therefore cannot find the initial energy");
                  }

                std::string loadAlgName("");
                if (boost::ends_with(runFileName, "_event.nxs"))
                  {
                    g_log.notice() << "Loading NeXus monitors" << std::endl;
                    loadAlgName = "LoadNexusMonitors";
                  }

                if (boost::ends_with(runFileName, "_neutron_event.dat"))
                  {
                    g_log.notice() << "Loading PreNeXus monitors" << std::endl;
                    loadAlgName = "LoadPreNexusMonitors";
                    boost::replace_first(runFileName, "_neutron_event.dat",
                        "_runinfo.xml");
                  }

                // Load the monitors
                IAlgorithm_sptr loadmon = this->createSubAlgorithm(loadAlgName);
                loadmon->setAlwaysStoreInADS(true);
                loadmon->setProperty("Filename", runFileName);
                loadmon->setProperty("OutputWorkspace", monWsName);
                loadmon->execute();

                reductionManager->declareProperty(new PropertyWithValue<std::string>("MonitorWorkspace", monWsName));

                // Calculate Ei
                // Get the monitor spectra indices from the parameters
                // TODO: Get these from algorithm properties. No longer need to read them from parameter file.
                MatrixWorkspace_const_sptr monWS = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(monWsName);
                eiMon1Spec = static_cast<specid_t>(monWS->getInstrument()->getNumberParameter("ei-mon1-spec")[0]);
                eiMon2Spec = static_cast<specid_t>(monWS->getInstrument()->getNumberParameter("ei-mon2-spec")[0]);

                IAlgorithm_sptr getei = this->createSubAlgorithm("GetEi");
                getei->setProperty("InputWorkspace", monWsName);
                getei->setProperty("Monitor1Spec", eiMon1Spec);
                getei->setProperty("Monitor2Spec", eiMon2Spec);
                getei->setProperty("EnergyEstimate", eiGuess);
                try
                {
                    getei->execute();
                    incidentEnergy = getei->getProperty("IncidentEnergy");
                    tZero = getei->getProperty("Tzero");
                }
                catch (...)
                {
                    g_log.error() << "GetEi failed, using guess as initial energy and T0 = 0" << std::endl;
                    incidentEnergy = eiGuess;
                }
              }
          }

        g_log.notice() << "Adjusting for T0" << std::endl;
        IAlgorithm_sptr alg = this->createSubAlgorithm("ChangeBinOffset");
        alg->setAlwaysStoreInADS(true);
        alg->setChild(true);
        alg->enableHistoryRecordingForChild(true);
        alg->setProperty("InputWorkspace", inWsName);
        alg->setProperty("OutputWorkspace", outWsName);
        alg->setProperty("Offset", -tZero);
        alg->execute();
      }
    // Do ISIS
    else
      {
        eiMon1Spec = static_cast<specid_t>(inputWS->getInstrument()->getNumberParameter("ei-mon1-spec")[0]);
        eiMon2Spec = static_cast<specid_t>(inputWS->getInstrument()->getNumberParameter("ei-mon2-spec")[0]);
        IAlgorithm_sptr getei = this->createSubAlgorithm("GetEi");
        getei->setAlwaysStoreInADS(true);
        getei->setProperty("InputWorkspace", inWsName);
        getei->setProperty("Monitor1Spec", eiMon1Spec);
        getei->setProperty("Monitor2Spec", eiMon2Spec);
        getei->setProperty("EnergyEstimate", eiGuess);
        getei->execute();

        monPeak = getei->getProperty("FirstMonitorPeak");
        const specid_t monIndex = getei->getProperty("FirstMonitorIndex");
        // Why did the old way get it from the log?
        incidentEnergy = getei->getProperty("IncidentEnergy");

        IAlgorithm_sptr cbo = this->createSubAlgorithm("ChangeBinOffset");
        cbo->setAlwaysStoreInADS(true);
        cbo->enableHistoryRecordingForChild(true);
        cbo->setProperty("InputWorkspace", inWsName);
        cbo->setProperty("OutputWorkspace", outWsName);
        cbo->setProperty("Offset", -monPeak);
        cbo->execute();

        IDetector_const_sptr monDet = inputWS->getDetector(monIndex);
        V3D monPos = monDet->getPos();
        std::string srcName = inputWS->getInstrument()->getSource()->getName();

        IAlgorithm_sptr moveInstComp = this->createSubAlgorithm("MoveInstrumentComponent");
        moveInstComp->setAlwaysStoreInADS(true);
        moveInstComp->setProperty("Workspace", outWsName);
        moveInstComp->setProperty("ComponentName", srcName);
        moveInstComp->setProperty("X", monPos.X());
        moveInstComp->setProperty("Y", monPos.Y());
        moveInstComp->setProperty("Z", monPos.Z());
        moveInstComp->setProperty("RelativePosition", false);
        moveInstComp->execute();
      }

    const double binOffset = -monPeak;
    reductionManager->declareProperty(new PropertyWithValue<double>("TofRangeOffset", binOffset));

    // Subtract time-independent background if necessary
    const bool doTibSub = reductionManager->getProperty("TimeIndepBackgroundSub");
    if (doTibSub)
      {
        // Set the binning parameters for the background region
        double tibTofStart = reductionManager->getProperty("TibTofRangeStart");
        tibTofStart += binOffset;
        double tibTofEnd = reductionManager->getProperty("TibTofRangeEnd");
        tibTofEnd += binOffset;
        const double tibTofWidth = tibTofEnd - tibTofStart;
        std::vector<double> params;
        params.push_back(tibTofStart);
        params.push_back(tibTofWidth);
        params.push_back(tibTofEnd);

        if ("SNS" == facility)
          {
            // Create an original background workspace from a portion of the
            // result workspace.
            std::string origBkgWsName = "background_origin_ws";
            IAlgorithm_sptr rebin = this->createSubAlgorithm("Rebin");
            rebin->setAlwaysStoreInADS(true);
            rebin->setProperty("InputWorkspace", outWsName);
            rebin->setProperty("OutputWorkspace", origBkgWsName);
            rebin->setProperty("Params", params);
            rebin->execute();

            // Convert result workspace to DeltaE since we have Et binning
            IAlgorithm_sptr cnvun = this->createSubAlgorithm("ConvertUnits");
            cnvun->setAlwaysStoreInADS(true);
            cnvun->setProperty("InputWorkspace", outWsName);
            cnvun->setProperty("OutputWorkspace", outWsName);
            cnvun->setProperty("Target", "DeltaE");
            cnvun->setProperty("EMode", "Direct");
            cnvun->setProperty("EFixed", incidentEnergy);
            cnvun->execute();

            if (etBinning.empty())
              {
                throw std::runtime_error("Need Et rebinning parameters in order to use background subtraction");
              }

            // Rebin to Et
            rebin->setProperty("InputWorkspace", outWsName);
            rebin->setProperty("OutputWorkspace", outWsName);
            rebin->setProperty("Params", etBinning);
            rebin->setProperty("PreserveEvents", false);
            rebin->execute();

            // Convert result workspace to TOF
            cnvun->setProperty("InputWorkspace", outWsName);
            cnvun->setProperty("OutputWorkspace", outWsName);
            cnvun->setProperty("Target", "TOF");
            cnvun->setProperty("EMode", "Direct");
            cnvun->setProperty("EFixed", incidentEnergy);
            cnvun->execute();

            // Make result workspace a distribution
            IAlgorithm_sptr cnvToDist = this->createSubAlgorithm("ConvertToDistribution");
            cnvToDist->setAlwaysStoreInADS(true);
            cnvToDist->setProperty("Workspace", outWsName);
            cnvToDist->execute();

            // Calculate the background
            std::string bkgWsName = "background_ws";
            IAlgorithm_sptr flatBg = this->createSubAlgorithm("FlatBackground");
            flatBg->setAlwaysStoreInADS(true);
            flatBg->setProperty("InputWorkspace", origBkgWsName);
            flatBg->setProperty("OutputWorkspace", bkgWsName);
            flatBg->setProperty("StartX", tibTofStart);
            flatBg->setProperty("EndX", tibTofEnd);
            flatBg->setProperty("Mode", "Mean");
            flatBg->setProperty("OutputMode", "Return Background");
            flatBg->execute();

            // Remove unneeded original background workspace
            IAlgorithm_sptr delWs = this->createSubAlgorithm("DeleteWorkspace");
            delWs->setProperty("Workspace", origBkgWsName);
            delWs->execute();

            // Make background workspace a distribution
            cnvToDist->setProperty("Workspace", bkgWsName);
            cnvToDist->execute();

            // Subtrac background from result workspace
            IAlgorithm_sptr minus = this->createSubAlgorithm("Minus");
            minus->setAlwaysStoreInADS(true);
            minus->setProperty("LHSWorkspace", outWsName);
            minus->setProperty("RHSWorkspace", bkgWsName);
            minus->setProperty("OutputWorkspace", outWsName);
            minus->execute();

            // Remove unneeded background workspace
            delWs->setProperty("Workspace", bkgWsName);
            delWs->execute();
          }
        // Do ISIS
        else
          {
            // TODO: More setup work needs to be done.
            IAlgorithm_sptr flatBg = this->createSubAlgorithm("FlatBackground");
            flatBg->setAlwaysStoreInADS(true);
            flatBg->setProperty("InputWorkspace", outWsName);
            flatBg->setProperty("OutputWorkspace", outWsName);
            flatBg->setProperty("StartX", tibTofStart);
            flatBg->setProperty("EndX", tibTofEnd);
            flatBg->setProperty("Mode", "Mean");
            flatBg->execute();
          }

        // Convert result workspace back to histogram
        IAlgorithm_sptr cnvFrDist = this->createSubAlgorithm("ConvertFromDistribution");
        cnvFrDist->setAlwaysStoreInADS(true);
        cnvFrDist->setProperty("Workspace", outWsName);
        cnvFrDist->execute();
      }

    // Normalise result workspace to incident beam parameter
    IAlgorithm_sptr norm = this->createSubAlgorithm("DgsPreprocessData");
    norm->setAlwaysStoreInADS(true);
    norm->setProperty("InputWorkspace", outWsName);
    norm->setProperty("OutputWorkspace", outWsName);
    norm->execute();

    // Convert to energy transfer
    g_log.notice() << "Converting to energy transfer." << std::endl;
    IAlgorithm_sptr cnvun = this->createSubAlgorithm("ConvertUnits");
    cnvun->setAlwaysStoreInADS(true);
    cnvun->setProperty("InputWorkspace", outWsName);
    cnvun->setProperty("OutputWorkspace", outWsName);
    cnvun->setProperty("Target", "DeltaE");
    cnvun->setProperty("EMode", "Direct");
    cnvun->setProperty("EFixed", incidentEnergy);
    cnvun->execute();

    // Rebin if necessary
    if (!etBinning.empty())
      {
        g_log.notice() << "Rebinning data" << std::endl;
        IAlgorithm_sptr rebin = this->createSubAlgorithm("Rebin");
        rebin->setAlwaysStoreInADS(true);
        rebin->setProperty("InputWorkspace", outWsName);
        rebin->setProperty("OutputWorkspace", outWsName);
        rebin->setProperty("Params", etBinning);
        rebin->setProperty("PreserveEvents", preserveEvents);
        rebin->execute();
      }

    // Correct for detector efficiency
    if ("SNS" == facility)
      {
        // He3TubeEfficiency requires the workspace to be in wavelength
        cnvun->setAlwaysStoreInADS(true);
        cnvun->setProperty("InputWorkspace", outWsName);
        cnvun->setProperty("OutputWorkspace", outWsName);
        cnvun->setProperty("Target", "Wavelength");
        cnvun->execute();

        // Do the correction
        IAlgorithm_sptr alg2 = this->createSubAlgorithm("He3TubeEfficiency");
        alg2->setAlwaysStoreInADS(true);
        alg2->setProperty("InputWorkspace", outWsName);
        alg2->setProperty("OutputWorkspace", outWsName);
        alg2->execute();

        // Convert back to energy transfer
        cnvun->setAlwaysStoreInADS(true);
        cnvun->setProperty("InputWorkspace", outWsName);
        cnvun->setProperty("OutputWorkspace", outWsName);
        cnvun->setProperty("Target", "DeltaE");
        cnvun->execute();
      }
    // Do ISIS
    else
      {
        IAlgorithm_sptr alg = this->createSubAlgorithm("DetectorEfficiencyCor");
        alg->setAlwaysStoreInADS(true);
        alg->setProperty("InputWorkspace", outWsName);
        alg->setProperty("OutputWorkspace", outWsName);
        alg->execute();
      }

    // Correct for Ki/Kf
    IAlgorithm_sptr kikf = this->createSubAlgorithm("CorrectKiKf");
    kikf->setAlwaysStoreInADS(true);
    kikf->setProperty("InputWorkspace", outWsName);
    kikf->setProperty("OutputWorkspace", outWsName);
    kikf->setProperty("EMode", "Direct");
    kikf->execute();

    // Rebin to ensure consistency
    if (!etBinning.empty())
      {
        const bool sofphieIsDistribution = reductionManager->getProperty("SofPhiEIsDistribution");

        g_log.notice() << "Rebinning data" << std::endl;
        IAlgorithm_sptr rebin = this->createSubAlgorithm("Rebin");
        rebin->setAlwaysStoreInADS(true);
        rebin->setProperty("InputWorkspace", outWsName);
        rebin->setProperty("OutputWorkspace", outWsName);
        rebin->setProperty("Params", etBinning);
        if (sofphieIsDistribution)
          {
            rebin->setProperty("PreserveEvents", false);
          }
        rebin->execute();

        if (sofphieIsDistribution)
          {
            g_log.notice() << "Making distribution" << std::endl;
            IAlgorithm_sptr distrib = this->createSubAlgorithm("ConvertToDistribution");
            distrib->setProperty("Workspace", outWsName);
            distrib->execute();
          }
      }

    // Normalise by the detector vanadium if necessary
    MatrixWorkspace_const_sptr detVanWS = this->getProperty("IntegratedDetectorVanadium");
    if (detVanWS)
      {
        IAlgorithm_sptr divide = this->createSubAlgorithm("Divide");
        divide->setAlwaysStoreInADS(true);
        divide->setProperty("LHSWorkspace", outWsName);
        divide->setProperty("RHSWorkspace", detVanWS);
        divide->setProperty("OutputWorkspace", outWsName);
        divide->execute();
      }

    if ("ISIS" == facility)
      {
        double scaleFactor = inputWS->getInstrument()->getNumberParameter("scale-factor")[0];
        const std::string scaleFactorName = "ScaleFactor";
        IAlgorithm_sptr csvw = this->createSubAlgorithm("CreateSingleValuedWorkspace");
        csvw->setAlwaysStoreInADS(true);
        csvw->setProperty("OutputWorkspace", scaleFactorName);
        csvw->setProperty("DataValue", scaleFactor);
        csvw->execute();

        IAlgorithm_sptr mult = this->createSubAlgorithm("Multiply");
        mult->setAlwaysStoreInADS(true);
        mult->setProperty("LHSWorkspace", outWsName);
        mult->setProperty("RHSWorkspace", scaleFactorName);
        mult->setProperty("OutputWorkspace", outWsName);
        mult->execute();
      }

    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName);
    this->setProperty("OutputWorkspace", outputWS);
  }

} // namespace Mantid
} // namespace WorkflowAlgorithms
