/*WIKI* 
Create a PropertyManager object setting the reduction options for HFIR SANS.
The property manager object is then added to the PropertyManagerDataService.

See [http://www.mantidproject.org/Reduction_for_HFIR_SANS SANS Reduction] documentation for details.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SetupHFIRReduction.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetupHFIRReduction)

/// Sets documentation strings for this algorithm
void SetupHFIRReduction::initDocs()
{
  this->setWikiSummary("Set up HFIR SANS reduction options.");
  this->setOptionalMessage("Set up HFIR SANS reduction options.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void SetupHFIRReduction::init()
{
  declareProperty("OutputMessage","",Direction::Output);
  declareProperty("ReductionProperties","__sans_reduction_properties", Direction::Input);
}

void SetupHFIRReduction::exec()
{
  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  if (reductionManagerName.size()==0)
  {
    g_log.error() << "ERROR: Reduction Property Manager name is empty" << std::endl;
    return;
  }
  boost::shared_ptr<PropertyManager> reductionManager = boost::make_shared<PropertyManager>();
  PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, reductionManager);

  // Store name of the instrument
  reductionManager->declareProperty(new PropertyWithValue<std::string>("InstrumentName", "BIOSANS") );

  // Store default dark current algorithm
  IAlgorithm_sptr darkAlg = createSubAlgorithm("HFIRDarkCurrentSubtraction");
  darkAlg->setProperty("OutputDarkCurrentWorkspace", "");
  darkAlg->setPropertyValue("ReductionProperties", reductionManagerName);
  reductionManager->declareProperty(new AlgorithmProperty("DefaultDarkCurrentAlgorithm"));
  reductionManager->setProperty("DefaultDarkCurrentAlgorithm", darkAlg);

  setPropertyValue("OutputMessage", "HFIR reduction options set");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

