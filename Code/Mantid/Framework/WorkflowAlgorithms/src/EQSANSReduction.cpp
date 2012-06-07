/*WIKI* 
EQSANS Reduction algorithm to use for Live Reduction.
*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/EQSANSReduction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidWorkflowAlgorithms/EQSANSInstrument.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "Poco/String.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSReduction)

/// Sets documentation strings for this algorithm
void EQSANSReduction::initDocs()
{
  this->setWikiSummary("Workflow to reduce EQSANS data.");
  this->setOptionalMessage("Workflow to reduce EQSANS data.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void EQSANSReduction::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::OptionalLoad, "_event.nxs"),
      "The name of the input event Nexus file to load");

  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace","",Direction::Input, PropertyMode::Optional, wsValidator),
        "Input event workspace. Assumed to be unmodified events straight from LoadEventNexus");





  declareProperty("ReductionProperties", "__eqsans_reduction_properties", Direction::Input);
  declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Workspace containing the sensitivity correction.");
  declareProperty("OutputMessage", "", Direction::Output);
}

void EQSANSReduction::exec()
{
  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  boost::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName))
  {
    reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  }
  else
  {
    g_log.notice() << "Could not find property manager" << std::endl;
    reductionManager = boost::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, reductionManager);
  }

  std::string outputMessage = "";

  // Find beam center
  if (reductionManager->existsProperty("SANSBeamFinderAlgorithm"))
  {
    IAlgorithm_sptr ctrAlg = reductionManager->getProperty("SANSBeamFinderAlgorithm");
    ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    ctrAlg->setChild(true);
    ctrAlg->execute();
    std::string outMsg = ctrAlg->getPropertyValue("OutputMessage");
    outputMessage += outMsg;
  }

  // Load data file or workspace
  // If we are processing a workspace, we assume it was simply loaded by LoadEventNexus
  const std::string outputWSName = getPropertyValue("OutputWorkspace");

  //TODO: this should be done by the new data management algorithm used for
  // live data reduction (when it's implemented...)
  const std::string fileName = getPropertyValue("Filename");
  EventWorkspace_sptr inputEventWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr inputWS;
  if (fileName.size()==0 && !inputEventWS)
  {
    g_log.error() << "EQSANSLoad input error: Either a valid file path or an input workspace must be provided" << std::endl;
    throw std::runtime_error("EQSANSLoad input error: Either a valid file path or an input workspace must be provided");
  }
  else if (fileName.size()>0 && inputEventWS)
  {
    g_log.error() << "EQSANSLoad input error: Either a valid file path or an input workspace must be provided, but not both" << std::endl;
    throw std::runtime_error("EQSANSLoad input error: Either a valid file path or an input workspace must be provided, but not both");
  }

  // Sanity check to verify that we have a loader defined
  if (!reductionManager->existsProperty("LoadAlgorithm"))
  {
    g_log.error() << "No loader found! Check your reduction options" << std::endl;
    return;
  }

  MatrixWorkspace_sptr outputWS;
  // Get load algorithm as a string so that we can create a completely
  // new proxy and ensure that we don't overwrite existing properties
  IAlgorithm_sptr loadAlg0 = reductionManager->getProperty("LoadAlgorithm");
  const std::string loadString = loadAlg0->toString();
  IAlgorithm_sptr loadAlg = Algorithm::fromString(loadString);
  loadAlg->setChild(true);

  if (inputEventWS)
    loadAlg->setProperty("InputWorkspace", inputEventWS);
  else
    loadAlg->setProperty("Filename", fileName);

  loadAlg->setPropertyValue("OutputWorkspace", outputWSName);
  loadAlg->execute();
  inputWS = loadAlg->getProperty("OutputWorkspace");
  inputEventWS = boost::dynamic_pointer_cast<EventWorkspace>(inputWS);

  outputMessage += "   |Loaded " + fileName + "\n";
  if (loadAlg->existsProperty("OutputMessage"))
  {
    std::string msg = loadAlg->getPropertyValue("OutputMessage");
    outputMessage += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
  }


  // Dark current subtraction
  g_log.notice() << "Starting dark current subtraction" << std::endl;
  if (reductionManager->existsProperty("DarkCurrentAlgorithm"))
  {
    IAlgorithm_sptr darkAlg = reductionManager->getProperty("DarkCurrentAlgorithm");
    darkAlg->setChild(true);
    darkAlg->setProperty("InputWorkspace", inputEventWS);
    darkAlg->setProperty("OutputWorkspace", inputEventWS);
    darkAlg->execute();
    if (darkAlg->existsProperty("OutputMessage"))
    {
      std::string msg = darkAlg->getPropertyValue("OutputMessage");
      outputMessage += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
    }
  }

  // Normalization

  // Mask

  // Solid angle correction
  if (reductionManager->existsProperty("SANSSolidAngleCorrection"))
  {
    IAlgorithm_sptr ctrAlg = reductionManager->getProperty("SANSSolidAngleCorrection");
    ctrAlg->setProperty("InputWorkspace", inputEventWS);
    ctrAlg->setProperty("OutputWorkspace", inputEventWS);
    ctrAlg->setChild(true);
    ctrAlg->execute();
  }

  // Sensitivity correction
  if (reductionManager->existsProperty("SensitivityAlgorithm"))
  {

    IAlgorithm_sptr effAlg = reductionManager->getProperty("SensitivityAlgorithm");
    effAlg->setProperty("InputWorkspace", inputEventWS);
    effAlg->setProperty("OutputWorkspace", inputEventWS);
    effAlg->setChild(true);
    effAlg->execute();
  }

  // Transmission correction

  // Background subtraction

  // Absolute scale

  // Geometry correction

  // Azimuthal averaging


  setProperty("OutputWorkspace", inputEventWS);

}

} // namespace WorkflowAlgorithms
} // namespace Mantid

