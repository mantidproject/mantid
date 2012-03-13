/*WIKI* 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SetupHFIRReduction.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidWorkflowAlgorithms/ReductionTableHandler.h"

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
  declareProperty(new WorkspaceProperty<TableWorkspace>("ReductionTableWorkspace","", Direction::Output, PropertyMode::Optional));
}

void SetupHFIRReduction::exec()
{
  TableWorkspace_sptr reductionTable = getProperty("ReductionTableWorkspace");
  const std::string reductionTableName = getPropertyValue("ReductionTableWorkspace");

  ReductionTableHandler reductionHandler(reductionTable);
  if (!reductionTable && reductionTableName.size()>0)
    setProperty("ReductionTableWorkspace", reductionHandler.getTable());

  // Store name of the instrument
  reductionHandler.addEntry("InstrumentName", "BIOSANS", true);

  // Store dark current algorithm algorithm
  IAlgorithm_sptr darkAlg = createSubAlgorithm("HFIRDarkCurrentSubtraction");
  darkAlg->setProperty("OutputDarkCurrentWorkspace", "");
  darkAlg->setPropertyValue("ReductionTableWorkspace", reductionTableName);
  reductionHandler.addEntry("DefaultDarkCurrentAlgorithm", darkAlg->toString());

  setPropertyValue("OutputMessage", "HFIR reduction options set");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

