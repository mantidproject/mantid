/*WIKI* 
Perform EQSANS reduction. This algorithm is used for live reduction
and can handle MPI.
*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/EQSANSReduce.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSReduce)

/// Sets documentation strings for this algorithm
void EQSANSReduce::initDocs()
{
  this->setWikiSummary("Workflow to perform EQSANS reduction.");
  this->setOptionalMessage("Workflow to perform EQSANS reduction.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;

void EQSANSReduce::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::OptionalLoad, "_event.nxs"),
      "File containing the data to reduce");
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Workspace to be reduced");
  declareProperty("ReductionProcess", true,
      "If true, both the reduction and the post-processing will be run");

  setPropertySettings("Filename", new EnabledWhenProperty("ReductionProcess", IS_EQUAL_TO, "1") );

  declareProperty("PostProcess", false,
      "If true, I(q) will be computed from the input workspace");
  declareProperty(new API::FileProperty("LogDataFile", "", API::FileProperty::OptionalLoad, ".nxs"),
      "For testing: optional file containing the sample logs");
  setPropertySettings("LogDataFile", new EnabledWhenProperty("ReductionProcess", IS_EQUAL_TO, "1") );

  declareProperty("ReductionProperties", "__eqsans_reduction_properties", Direction::Input);

  declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Workspace containing the sensitivity correction.");
  declareProperty(new FileProperty("OutputFile", "", FileProperty::OptionalSave, ".nxs"),
      "File path for the output nexus file");
}

/**
 * Determine whether the input data is a file or a workspace and load it.
 */
Workspace_sptr EQSANSReduce::loadInputData()
{
  setLoadAlg("LoadEventNexus");
  Workspace_sptr inputWS;

  std::string inputData = getPropertyValue("Filename");
  const std::string inputWSName = getPropertyValue("InputWorkspace");
  if (inputWSName.size() > 0 && inputData.size() > 0)
    throw std::runtime_error("EQSANSReduce: Either the Filename property or InputWorkspace property must be provided, NOT BOTH");
  else if (inputWSName.size() > 0)
    inputWS = load(inputWSName);
  else if (inputData.size() > 0)
    inputWS = load(inputData);
  else
    throw std::runtime_error("EQSANSReduce: Either the Filename property or InputWorkspace property must be provided");

  return inputWS;
}

/**
 * Perform the reduction process on the given workspace.
 * @param workspace :: name of the workspace to reduce
 */
void EQSANSReduce::performReduction(Workspace_sptr workspace)
{
  if (!workspace)
    throw std::runtime_error("EQSANSReduce.performReduction was passed a pointer to no workspace");

  // For testing the live reduction, we may need to load some
  // logs from another file
  const std::string logFile = getPropertyValue("LogDataFile");
  if (logFile.size()>0)
  {
    IAlgorithm_sptr alg = this->createSubAlgorithm("LoadNexusLogs");
    alg->setLogging(false);
    alg->setProperty("Workspace", workspace);
    alg->setPropertyValue("Filename", logFile);
    alg->setProperty("OverwriteLogs", true);
    alg->execute();
  }

  // Write the Reducer python script to be executed
  std::string script = "import reduction.instruments.sans.sns_command_interface as cmd\n";
  script += "cmd.AppendDataFile([\"" + workspace->name() + "\"])\n";
  script += "cmd.Reduce1D()\n";

  // Run a snippet of python
  IAlgorithm_sptr alg = this->createSubAlgorithm("RunPythonScript");
  alg->setLogging(true);
  alg->setPropertyValue("Code", script);
  alg->execute();
}

/**
 * Perform post-processing (I(q) calculation) on the reduced workspace.
 * In the case of MPI jobs, the post-processing is done on the assemble workspace.
 * @param workspace :: name of the workspace to process
 */
Workspace_sptr EQSANSReduce::postProcess(Workspace_sptr workspace)
{
  // Construct the script's output workspace name
  const std::string outputIq = workspace->name() + "_Iq";
  const std::string outputWSName = getPropertyValue("OutputWorkspace");

  // Write the Reducer python script to be executed
  std::string script = "import reduction.instruments.sans.sns_command_interface as cmd\n";
  script += "from reduction.instruments.sans.sns_reduction_steps import AzimuthalAverageByFrame\n";
  script += "averager = AzimuthalAverageByFrame()\n";
  script += "output = \"" + outputIq + "\"\n";
  script += "averager.execute(cmd.ReductionSingleton(),\"" + workspace->name() + "\")\n";

  // Run a snippet of python
  IAlgorithm_sptr scriptAlg = this->createSubAlgorithm("RunPythonScript");
  scriptAlg->setLogging(true);
  scriptAlg->setPropertyValue("Code", script);
  scriptAlg->setPropertyValue("OutputWorkspace", outputIq);
  scriptAlg->execute();

  Workspace_sptr outputWS = AnalysisDataService::Instance().retrieve(outputIq);

  MatrixWorkspace_sptr matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
  matrixWS *= getNThreads();

  return outputWS;
}

void EQSANSReduce::exec()
{
  // Check the validity of the input data and load as appropriate
  Workspace_sptr workspace = loadInputData();

  // Reduce the data
  const bool doReduction = getProperty("ReductionProcess");
  const bool doPostProcessing = getProperty("PostProcess");
  const std::string outputFile = getPropertyValue("OutputFile");

  if (doReduction) performReduction(workspace);

  // Assemble parts (MPI jobs only)
  std::string outputWSName = workspace->name();
  Workspace_sptr assembledWS = assemble(outputWSName, outputWSName);

  if (doPostProcessing)
  {
    if (isMainThread())
    {
      workspace = postProcess(assembledWS);
      saveNexus(workspace->name(), outputFile);
    }
    setProperty("OutputWorkspace", workspace);
  }
  else if (doReduction)
  {
    setProperty("OutputWorkspace", workspace);
  }
  else
    g_log.error() << "EQSANSReduce: The ReductionProcess and PostProcess properties are set to false: nothing to do" << std::endl;

}

} // namespace WorkflowAlgorithms
} // namespace Mantid

