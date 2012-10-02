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
      this->declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input, wsValidator),
          "An input workspace containing the detector vanadium data in TOF units.");
      this->declareProperty(new WorkspaceProperty<>("MaskWorkspace",
          "", Direction::Input, PropertyMode::Optional),
          "A mask workspace");
      this->declareProperty(new WorkspaceProperty<>("GroupingWorkspace",
          "", Direction::Input, PropertyMode::Optional), "A grouping workspace");
      this->declareProperty("AlternateGroupingTag", "",
          "Allows modification to the OldGroupingFile property name");
      this->declareProperty("NoGrouping", false, "Flag to turn off grouping. This is mainly to cover the use of old format grouping files.");
      this->declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
          "The name for the output workspace.");
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

      MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
      MatrixWorkspace_sptr outputWS = this->getProperty("OutputWorkspace");

      // Normalise result workspace to incident beam parameter
      IAlgorithm_sptr norm = this->createSubAlgorithm("DgsPreprocessData");
      norm->setProperty("InputWorkspace", inputWS);
      norm->setProperty("OutputWorkspace", outputWS);
      norm->executeAsSubAlg();
      outputWS = norm->getProperty("OutputWorkspace");

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
      rebin->setProperty("PreserveEvents", false);
      rebin->setProperty("Params", binning);
      rebin->executeAsSubAlg();
      outputWS = rebin->getProperty("OutputWorkspace");

      // Mask and group workspace if necessary.
      MatrixWorkspace_sptr maskWS = this->getProperty("MaskWorkspace");
      MatrixWorkspace_sptr groupWS = this->getProperty("GroupingWorkspace");
      std::string oldGroupFile("");
      std::string filePropMod = this->getProperty("AlternateGroupingTag");
      std::string fileProp = filePropMod + "OldGroupingFilename";
      if (reductionManager->existsProperty(fileProp))
      {
        oldGroupFile = reductionManager->getPropertyValue(fileProp);
      }
      IAlgorithm_sptr remap = this->createSubAlgorithm("DgsRemap");
      remap->setProperty("InputWorkspace", outputWS);
      remap->setProperty("OutputWorkspace", outputWS);
      remap->setProperty("MaskWorkspace", maskWS);
      remap->setProperty("GroupingWorkspace", groupWS);
      if (!this->getProperty("NoGrouping"))
      {
        remap->setProperty("OldGroupingFile", oldGroupFile);
      }
      remap->executeAsSubAlg();
      outputWS = remap->getProperty("OutputWorkspace");

      const std::string facility = ConfigService::Instance().getFacility().name();
      if ("ISIS" == facility)
      {
        // Scale results by a constant
        double wbScaleFactor = inputWS->getInstrument()->getNumberParameter("wb-scale-factor")[0];
        outputWS *= wbScaleFactor;
      }

      if (reductionManager->existsProperty("SaveProcessedDetVan"))
      {
        bool saveProc = reductionManager->getProperty("SaveProcessedDetVan");
        if (saveProc)
        {
          std::string outputFile = this->getPropertyValue("OutputWorkspace");

          // Don't save private calculation workspaces
          if (!outputFile.empty() && !boost::starts_with(outputFile, "ChildAlgOutput") &&
              !boost::starts_with(outputFile, "__"))
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
