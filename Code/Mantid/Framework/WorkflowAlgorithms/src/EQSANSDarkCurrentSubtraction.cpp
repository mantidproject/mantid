/*WIKI* 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/EQSANSDarkCurrentSubtraction.h"
#include "MantidWorkflowAlgorithms/ReductionTableHandler.h"
#include "MantidDataObjects/EventWorkspace.h"
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
DECLARE_ALGORITHM(EQSANSDarkCurrentSubtraction)

/// Sets documentation strings for this algorithm
void EQSANSDarkCurrentSubtraction::initDocs()
{
  this->setWikiSummary("Perform EQSANS dark current subtraction.");
  this->setOptionalMessage("Perform EQSANS dark current subtraction.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void EQSANSDarkCurrentSubtraction::init()
{
  CompositeWorkspaceValidator<> *wsValidator = new CompositeWorkspaceValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));

  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, ".nxs"),
      "The name of the input event Nexus file to load as dark current.");

  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  declareProperty(new WorkspaceProperty<TableWorkspace>("ReductionTableWorkspace","", Direction::Output, true));
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputDarkCurrentWorkspace","", Direction::Output, true));
  declareProperty("OutputMessage","",Direction::Output);

}

void EQSANSDarkCurrentSubtraction::exec()
{
  Progress progress(this,0.0,1.0,10);

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if ( outputWS != inputWS )
  {
    EventWorkspace_sptr eventInputWS = boost::dynamic_pointer_cast<EventWorkspace>(inputWS);
    if (eventInputWS)
    {
      //Make a brand new EventWorkspace
      EventWorkspace_sptr eventOutputWS = boost::dynamic_pointer_cast<EventWorkspace>(
          API::WorkspaceFactory::Instance().create("EventWorkspace", eventInputWS->getNumberHistograms(), 2, 1));
      //Copy geometry over.
      API::WorkspaceFactory::Instance().initializeFromParent(eventInputWS, eventOutputWS, false);
      //You need to copy over the data as well.
      eventOutputWS->copyDataFrom( (*eventInputWS) );

      //Cast to the matrixOutputWS and save it
      outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(eventOutputWS);
    } else {
      outputWS = WorkspaceFactory::Instance().create(inputWS);
      outputWS->isDistribution(inputWS->isDistribution());
    }
  }

  const std::string fileName = getPropertyValue("Filename");
  MatrixWorkspace_sptr darkWS = getProperty("OutputDarkCurrentWorkspace");
  std::string darkWSName = getPropertyValue("OutputDarkCurrentWorkspace");

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
  darkWS = reductionHandler.findWorkspaceEntry(entryName);
  darkWSName = reductionHandler.findStringEntry(entryName);

  if (darkWSName.size()==0) {
    darkWSName = getPropertyValue("OutputDarkCurrentWorkspace");
    if (darkWSName.size()==0)
      darkWSName = "__dark_current_"+path.getBaseName();
    setPropertyValue("OutputDarkCurrentWorkspace", darkWSName);
    reductionHandler.addEntry(entryName, darkWSName);
  }

  // Load the dark current if we don't have it already
  if (!darkWS)
  {
    std::string loader = reductionHandler.findStringEntry("LoadAlgorithm");
    if (loader.size()==0)
    {
      IAlgorithm_sptr loadAlg = createSubAlgorithm("EQSANSLoad", 0.1, 0.3);
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

  // Normalize the dark current and data to counting time
  Mantid::Kernel::Property* prop = inputWS->run().getProperty("proton_charge");
  Mantid::Kernel::TimeSeriesProperty<double>* dp = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double>* >(prop);
  double duration = dp->getStatistics().duration;

  prop = darkWS->run().getProperty("proton_charge");
  dp = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double>* >(prop);
  double dark_duration = dp->getStatistics().duration;
  double scaling_factor = duration/dark_duration;

  progress.report("Scaling dark current");

  // Scale the stored dark current by the counting time
  IAlgorithm_sptr rebinAlg = createSubAlgorithm("RebinToWorkspace", 0.4, 0.5);
  rebinAlg->setProperty("WorkspaceToRebin", darkWS);
  rebinAlg->setProperty("WorkspaceToMatch", inputWS);
  rebinAlg->setProperty("OutputWorkspace", darkWS);
  rebinAlg->executeAsSubAlg();
  MatrixWorkspace_sptr scaledDarkWS = rebinAlg->getProperty("OutputWorkspace");

  // Perform subtraction
  IAlgorithm_sptr scaleAlg = createSubAlgorithm("Scale", 0.5, 0.6);
  scaleAlg->setProperty("InputWorkspace", scaledDarkWS);
  scaleAlg->setProperty("Factor", scaling_factor);
  scaleAlg->setProperty("OutputWorkspace", scaledDarkWS);
  scaleAlg->setProperty("Operation", "Multiply");
  scaleAlg->executeAsSubAlg();

  IAlgorithm_sptr minusAlg = createSubAlgorithm("Minus", 0.6, 0.7);
  minusAlg->setProperty("LHSWorkspace", inputWS);
  minusAlg->setProperty("RHSWorkspace", scaledDarkWS);
  minusAlg->setProperty("OutputWorkspace", outputWS);
  minusAlg->executeAsSubAlg();

  setProperty("OutputWorkspace", outputWS);
  setProperty("OutputMessage", "Dark current subtracted: "+darkWSName);

  progress.report("Subtracted dark current");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

