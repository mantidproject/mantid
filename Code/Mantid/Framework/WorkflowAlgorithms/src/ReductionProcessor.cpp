#include "MantidWorkflowAlgorithms/ReductionProcessor.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/AlgorithmManager.h"
#include <stdexcept>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace WorkflowAlgorithms
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ReductionProcessor)

  /// Sets documentation strings for this algorithm
  void ReductionProcessor::initDocs()
  {
    this->setWikiSummary("Data processor algorithm.");
    this->setOptionalMessage("Data processor algorithm.");
  }

  void ReductionProcessor::init()
  {
    // Input data object (File or Workspace)
    declareProperty("InputData", "", "Input data, either as a file path or a workspace name");
    declareProperty("LoadAlgorithm", "LoadEventNexus");
    declareProperty("ProcessingAlgorithm", "");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  }

  void ReductionProcessor::exec()
  {
    // Set the data loader
    const std::string loader = getProperty("LoadAlgorithm");
    setLoadAlg(loader);

    // Load the data
    const std::string inputData = getProperty("InputData");
    Workspace_sptr inputWS = load(inputData);

    // Process the data
    g_log.information() << "Starting to process " << inputData << std::endl;

    const std::string outputWSName = getPropertyValue("OutputWorkspace");
    const std::string procAlgName = getProperty("ProcessingAlgorithm");

    IAlgorithm_sptr procAlg = createSubAlgorithm(procAlgName);
    procAlg->setPropertyValue("InputWorkspace", inputData);
    procAlg->setAlwaysStoreInADS(true);
    procAlg->setPropertyValue("OutputWorkspace", outputWSName);
    procAlg->execute();

    g_log.information() << "Done processing " << inputData << std::endl;

    Workspace_sptr outputWS = AnalysisDataService::Instance().retrieve(outputWSName);
    setProperty("OutputWorkspace", outputWS);
  }

}
}
