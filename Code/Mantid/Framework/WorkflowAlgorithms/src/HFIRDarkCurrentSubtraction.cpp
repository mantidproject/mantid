/*WIKI* 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/HFIRDarkCurrentSubtraction.h"
#include "MantidWorkflowAlgorithms/ReductionTableHandler.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/TableRow.h"
#include "Poco/File.h"
#include "Poco/Path.h"
#include "MantidAPI/AlgorithmManager.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HFIRDarkCurrentSubtraction)

/// Sets documentation strings for this algorithm
void HFIRDarkCurrentSubtraction::initDocs()
{
  this->setWikiSummary("Perform HFIR SANS dark current subtraction.");
  this->setOptionalMessage("Perform HFIR SANS dark current subtraction.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void HFIRDarkCurrentSubtraction::init()
{
  CompositeWorkspaceValidator<> *wsValidator = new CompositeWorkspaceValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));

  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, ".xml"),
      "The name of the input event Nexus file to load as dark current.");

  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  declareProperty(new WorkspaceProperty<TableWorkspace>("ReductionTableWorkspace","", Direction::Output, true));
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputDarkCurrentWorkspace","", Direction::Output, true));
  declareProperty("OutputMessage","",Direction::Output);
}

void HFIRDarkCurrentSubtraction::exec()
{
  Progress progress(this,0.0,1.0,10);

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if ( outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    outputWS->isDistribution(inputWS->isDistribution());
  }

  const std::string fileName = getPropertyValue("Filename");

  // Get the reduction table workspace or create one
  TableWorkspace_sptr reductionTable = getProperty("ReductionTableWorkspace");
  ReductionTableHandler reductionHandler(reductionTable);
  if (!reductionTable)
  {
    const std::string reductionTableName = getPropertyValue("ReductionTableWorkspace");
    if (reductionTableName.size()>0) setProperty("ReductionTableWorkspace", reductionHandler.getTable());
  }
  if (reductionHandler.findStringEntry("DarkCurrentAlgorithm").size()==0)
    reductionHandler.addEntry("DarkCurrentAlgorithm", toString());

  progress.report("Subtracting dark current");

  // Look for an entry for the dark current in the reduction table
  Poco::Path path(fileName);
  const std::string entryName = "DarkCurrent"+path.getBaseName();
  MatrixWorkspace_sptr darkWS = reductionHandler.findWorkspaceEntry(entryName);
  std::string darkWSName = reductionHandler.findStringEntry(entryName);

  if (darkWSName.size()==0) {
    darkWSName = getPropertyValue("OutputDarkCurrentWorkspace");
    if (darkWSName.size()==0)
    {
      darkWSName = "__dark_current_"+path.getBaseName();
      setPropertyValue("OutputDarkCurrentWorkspace", darkWSName);
    }
    reductionHandler.addEntry(entryName, darkWSName);
  }

  // Load the dark current if we don't have it already
  if (!darkWS)
  {
    std::string loader = reductionHandler.findStringEntry("LoadAlgorithm");
    if (loader.size()==0)
    {
      IAlgorithm_sptr loadAlg = createSubAlgorithm("HFIRLoad", 0.1, 0.3);
      loadAlg->setProperty("Filename", fileName);
      loadAlg->executeAsSubAlg();
      darkWS = loadAlg->getProperty("OutputWorkspace");
    } else {
      IAlgorithm_sptr loadAlg = Algorithm::fromString(loader);
      loadAlg->setChild(true);
      loadAlg->setProperty("Filename", fileName);
      loadAlg->setPropertyValue("OutputWorkspace", darkWSName);
      loadAlg->execute();
      //darkWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(darkWSName));
      darkWS = loadAlg->getProperty("OutputWorkspace");
    }
    setProperty("OutputDarkCurrentWorkspace", darkWS);
  }
  progress.report(3, "Loaded dark current");

  // Perform subtraction
  double darkTimer = getCountingTime(darkWS);
  double dataTimer = getCountingTime(inputWS);
  IAlgorithm_sptr scaleAlg = createSubAlgorithm("Scale", 0.3, 0.5);
  scaleAlg->setProperty("InputWorkspace", darkWS);
  scaleAlg->setProperty("Factor", dataTimer/darkTimer);
  scaleAlg->setProperty("Operation", "Multiply");
  scaleAlg->executeAsSubAlg();
  MatrixWorkspace_sptr scaledDarkWS = scaleAlg->getProperty("OutputWorkspace");

  // Zero out timer and monitor so that we don't subtract them out
  for(size_t i=0; i<scaledDarkWS->dataY(0).size(); i++)
  {
    scaledDarkWS->dataY(DEFAULT_TIMER_ID)[i]=0.0;
    scaledDarkWS->dataE(DEFAULT_TIMER_ID)[i]=0.0;
    scaledDarkWS->dataY(DEFAULT_MONITOR_ID)[i]=0.0;
    scaledDarkWS->dataE(DEFAULT_MONITOR_ID)[i]=0.0;
  }
  IAlgorithm_sptr minusAlg = createSubAlgorithm("Minus", 0.5, 0.7);
  minusAlg->setProperty("LHSWorkspace", inputWS);
  minusAlg->setProperty("RHSWorkspace", scaledDarkWS);
  minusAlg->setProperty("OutputWorkspace", outputWS);
  minusAlg->executeAsSubAlg();

  setProperty("OutputWorkspace", outputWS);
  setProperty("OutputMessage", "Dark current subtracted: "+darkWSName);

  progress.report("Subtracted dark current");
}

/// Get the counting time from a workspace
/// @param inputWS :: workspace to read the counting time from
double HFIRDarkCurrentSubtraction::getCountingTime(MatrixWorkspace_sptr inputWS)
{
  // First, look whether we have the information in the log
  if (inputWS->run().hasProperty("timer"))
  {
    Mantid::Kernel::Property* prop = inputWS->run().getProperty("timer");
    Mantid::Kernel::PropertyWithValue<double>* dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double>* >(prop);
    return *dp;
  } else {
    // If we don't have the information in the log, use the default timer spectrum
    MantidVec& timer = inputWS->dataY(DEFAULT_TIMER_ID);
    return timer[0];
  }
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

