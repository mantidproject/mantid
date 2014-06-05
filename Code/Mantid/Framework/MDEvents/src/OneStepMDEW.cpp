#include "MantidMDEvents/OneStepMDEW.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
//#include "MantidNexus/LoadEventNexus.h"
//#include "MantidMDEvents/ConvertToDiffractionMDWorkspace.h"

namespace Mantid
{
  namespace MDEvents
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(OneStepMDEW)

    using namespace Mantid::Kernel;
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;


    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    OneStepMDEW::OneStepMDEW()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    OneStepMDEW::~OneStepMDEW()
    {
    }


    //----------------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
    */
    void OneStepMDEW::init()
    {
      this->declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
        "The name (including its full or relative path) of the Nexus file to\n"
        "attempt to load. The file extension must either be .nxs or .NXS" );

      this->declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
    */
    void OneStepMDEW::exec()
    {
      std::string tempWsName = getPropertyValue("OutputWorkspace") + "_nxs";

      Algorithm_sptr childAlg;
      // -------- First we load the event nexus file -------------
      childAlg = AlgorithmFactory::Instance().create("LoadEventNexus", 1);
      childAlg->initialize();
      childAlg->setPropertyValue("Filename", getPropertyValue("Filename"));
      childAlg->setPropertyValue("OutputWorkspace", tempWsName);
      childAlg->executeAsChildAlg();

      // --------- Now Convert -------------------------------
      childAlg = createChildAlgorithm("ConvertToDiffractionMDWorkspace");
      childAlg->setPropertyValue("InputWorkspace", tempWsName);
      childAlg->setProperty<bool>("ClearInputWorkspace", false);
      childAlg->setProperty<bool>("LorentzCorrection", true);
      childAlg->executeAsChildAlg();

      IMDEventWorkspace_sptr outWS = childAlg->getProperty("OutputWorkspace");
      setProperty<Workspace_sptr>("OutputWorkspace", outWS);
    }



  } // namespace Mantid
} // namespace MDEvents
