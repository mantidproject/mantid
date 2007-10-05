#include "../inc/FrameworkManager.h"

#include "../inc/AlgorithmFactory.h"
#include "../inc/WorkspaceFactory.h"
#include "../inc/AnalysisDataService.h"
#include "../../DataHandling/inc/LoadRaw.h"
#include "../../Algorithms/inc/SimpleIntegration.h"
#include "../../DataObjects/inc/Workspace2D.h"
#include "../../DataObjects/inc/Workspace1D.h"

#include <stdexcept> 

namespace Mantid
{

FrameworkManager::FrameworkManager()
{
}

FrameworkManager::~FrameworkManager()
{
}

void FrameworkManager::initialize()
{
  // Required services are: the algorithm factory
  //     the analysis data service, the workspace factory
  algFactory = AlgorithmFactory::Instance();
  workFactory = WorkspaceFactory::Instance();
  data = AnalysisDataService::Instance();
  
  // Register all our algorithms and workspaces
  algFactory->subscribe("LoadRaw", ConcreteAlgorithmCreator<LoadRaw>::createInstance );
  algFactory->subscribe("SimpleIntegration", ConcreteAlgorithmCreator<SimpleIntegration>::createInstance );
  workFactory->registerWorkspace("Workspace2D", Workspace2D::create );
  workFactory->registerWorkspace("Workspace1D", Workspace1D::create );
}

IAlgorithm* FrameworkManager::createAlgorithm(std::string algName)
{
  IAlgorithm *alg;
  StatusCode status = algFactory->createAlgorithm(algName, alg);
  if (status.isFailure())
  {
    throw std::runtime_error("Unable to create algorithm " + algName);
  }
  return alg;
}

// Not implemented yet
IAlgorithm* FrameworkManager::createAlgorithm(std::string algName, std::string propertiesArray)
{
  return 0;
}

// Not implemented yet
IAlgorithm* FrameworkManager::exec(std::string algName, std::string propertiesArray)
{
  return 0;
}

// returns a shared pointer to the workspace requested
Workspace* FrameworkManager::getWorkspace(std::string wsName)
{
  Workspace *space;
  StatusCode status = data->retrieve(wsName, space);
  if (status.isFailure())
  {
    throw std::runtime_error("Unable to retrieve workspace " + wsName);
  }
  return space;
}

}
