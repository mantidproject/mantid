/*WIKI*

This algorithm is responsible for processing the detector vanadium in the form
required for the sample data normalisation in the convert to energy transfer
process.

*WIKI*/

#include "MantidWorkflowAlgorithms/DgsProcessDetectorVanadium.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

#include <boost/algorithm/string/predicate.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
  namespace WorkflowAlgorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(DgsProcessDetectorVanadium)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    DgsProcessDetectorVanadium::DgsProcessDetectorVanadium()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    DgsProcessDetectorVanadium::~DgsProcessDetectorVanadium()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string DgsProcessDetectorVanadium::name() const { return "DgsProcessDetectorVanadium"; }

    /// Algorithm's version for identification. @see Algorithm::version
    int DgsProcessDetectorVanadium::version() const { return 1; }

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string DgsProcessDetectorVanadium::category() const { return "Workflow\\Inelastic"; }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void DgsProcessDetectorVanadium::initDocs()
    {
      this->setWikiSummary("Algorithm to process detector vanadium.");
      this->setOptionalMessage("Algorithm to process detector vanadium.");
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void DgsProcessDetectorVanadium::init()
    {
      auto wsValidator = boost::make_shared<CompositeValidator>();
      wsValidator->add<WorkspaceUnitValidator>("TOF");
      this->declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input, wsValidator),
          "An input workspace containing the detector vanadium data in TOF units.");
      this->declareProperty(new WorkspaceProperty<MatrixWorkspace>("DiagMaskWorkspace",
          "", Direction::Input, PropertyMode::Optional),
          "Workspace containing a mask determined by a diagnostic procedure");
      this->declareProperty(new WorkspaceProperty<MatrixWorkspace>("GroupingWorkspace",
          "", Direction::Input, PropertyMode::Optional), "A grouping workspace");
      this->declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
          "The workspace containing the processed results.");
      this->declareProperty("ReductionProperties", "__dgs_reduction_properties",
          Direction::Output);
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void DgsProcessDetectorVanadium::exec()
    {
      g_log.notice() << "Starting DgsProcessDetectorVanadium" << std::endl;
      // Get the reduction property manager
      const std::string reductionManagerName = this->getProperty("ReductionProperties");
      boost::shared_ptr<PropertyManager> reductionManager;
      if (PropertyManagerDataService::Instance().doesExist(reductionManagerName))
      {
        reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
      }
      else
      {
        throw std::runtime_error("DgsProcessDetectorVanadium cannot run without a reduction PropertyManager.");
      }

      this->enableHistoryRecordingForChild(true);

      MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
      const std::string outWsName = inputWS->getName() + "_idetvan";
      MatrixWorkspace_sptr outputWS;

      // Normalise result workspace to incident beam parameter
      IAlgorithm_sptr norm = this->createSubAlgorithm("DgsPreprocessData");
      norm->setProperty("InputWorkspace", inputWS);
      norm->executeAsSubAlg();
      outputWS = norm->getProperty("OutputWorkspace");

      // Apply masking from either DgsDiagnose or a given hard mask. If the
      // DgsDiagnose and hard mask are present, only do DgsDiagnose since the
      // hard mask will already be incorporated.
      const std::string hardMaskWsName = reductionManager->getProperty("HardMaskWorkspace");
      MatrixWorkspace_sptr diagMaskWs = this->getProperty("DiagMaskWorkspace");
      if (!hardMaskWsName.empty() || diagMaskWs)
      {
        IAlgorithm_sptr mask = this->createSubAlgorithm("MaskDetectors");
        mask->setProperty("Workspace", outputWS);
        if (diagMaskWs)
        {
          mask->setProperty("MaskedWorkspace", diagMaskWs);
        }
        else
        {
          mask->setPropertyValue("MaskedWorkspace", hardMaskWsName);
        }
        mask->execute();
        outputWS = mask->getProperty("Workspace");
      }

      double detVanIntRangeLow = reductionManager->getProperty("DetVanIntRangeLow");
      if (EMPTY_DBL() == detVanIntRangeLow)
      {
        detVanIntRangeLow = inputWS->getInstrument()->getNumberParameter("wb-integr-min")[0];
      }
      double detVanIntRangeHigh = reductionManager->getProperty("DetVanIntRangeHigh");
      if (EMPTY_DBL() == detVanIntRangeHigh)
      {
        detVanIntRangeHigh = inputWS->getInstrument()->getNumberParameter("wb-integr-max")[0];
      }
      const std::string detVanIntRangeUnits = reductionManager->getProperty("DetVanIntRangeUnits");

      if ("TOF" != detVanIntRangeUnits)
      {
        // Convert the data to the appropriate units
        IAlgorithm_sptr cnvun = this->createSubAlgorithm("ConvertUnits");
        cnvun->setProperty("InputWorkspace", outputWS);
        cnvun->setProperty("OutputWorkspace", outputWS);
        cnvun->setProperty("Target", detVanIntRangeUnits);
        cnvun->setProperty("EMode", "Elastic");
        cnvun->executeAsSubAlg();
        outputWS = cnvun->getProperty("OutputWorkspace");
      }

      // Rebin the data (not Integration !?!?!?)
      std::vector<double> binning;
      binning.push_back(detVanIntRangeLow);
      binning.push_back(detVanIntRangeHigh - detVanIntRangeLow);
      binning.push_back(detVanIntRangeHigh);

      IAlgorithm_sptr rebin = this->createSubAlgorithm("Rebin");
      rebin->setProperty("InputWorkspace", outputWS);
      rebin->setProperty("OutputWorkspace", outputWS);
      rebin->setProperty("Params", binning);
      rebin->executeAsSubAlg();
      outputWS = rebin->getProperty("OutputWorkspace");

      // Mask and group workspace if necessary.
      MatrixWorkspace_sptr groupWS = this->getProperty("GroupingWorkspace");
      if (groupWS)
      {
        IAlgorithm_sptr remap = this->createSubAlgorithm("DgsRemap");
        remap->setProperty("InputWorkspace", outputWS);
        remap->setProperty("OutputWorkspace", outputWS);
        if (groupWS)
        {
          remap->setProperty("GroupingWorkspace", groupWS);
        }
        remap->executeAsSubAlg();
        outputWS = remap->getProperty("OutputWorkspace");
      }

      const std::string facility = ConfigService::Instance().getFacility().name();
      if ("ISIS" == facility)
      {
        // Scale results by a constant
        double wbScaleFactor = inputWS->getInstrument()->getNumberParameter("wb-scale-factor")[0];
        outputWS *= wbScaleFactor;
      }

      outputWS->setName(outWsName);
      if (reductionManager->existsProperty("SaveProcessedDetVan"))
      {
        bool saveProc = reductionManager->getProperty("SaveProcessedDetVan");
        if (saveProc)
        {
          std::string outputFile = outputWS->name();
          g_log.warning() << "DetVan WS: " << outputFile << std::endl;
          // Don't save private calculation workspaces
          if (!outputFile.empty() && !boost::starts_with(outputFile, "_"))
          {
            IAlgorithm_sptr save = this->createSubAlgorithm("SaveNexus");
            save->setProperty("InputWorkspace", outputWS);
            outputFile += ".nxs";
            save->setProperty("FileName", outputFile);
            save->execute();
          }
        }
      }

      this->setProperty("OutputWorkspace", outputWS);
    }

  } // namespace WorkflowAlgorithms
} // namespace Mantid
