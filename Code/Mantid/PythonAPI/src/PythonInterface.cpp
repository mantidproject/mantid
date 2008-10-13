#include <iostream>

#include "boost/algorithm/string.hpp"
#include "boost/pointer_cast.hpp"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"

#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidPythonAPI/PythonInterface.h"
#include "MantidPythonAPI/SimplePythonAPI.h"

using namespace Mantid::API;

namespace Mantid
{
namespace PythonAPI
{
	
/**
 * Loads a standard ISIS raw file into Mantid, using the LoadRaw algorithm.
 * \param fileName :: The filepath of the raw file to be opened.
 * \param workspaceName :: The name under which the workspace is to be stored in Mantid.
 * \return Shared pointer to the workspace.
 **/
Workspace_sptr LoadIsisRawFile(const std::string& fileName,
		const std::string& workspaceName)
{
	//Check workspace does not exist
	if (!AnalysisDataService::Instance().doesExist(workspaceName))
	{
		IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("LoadRaw");
		alg->setPropertyValue("Filename", fileName);
		alg->setPropertyValue("OutputWorkspace", workspaceName);

		alg->execute();

		Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);

		return output;
	}
	
	Workspace_sptr empty;
	
	return empty;
}

/**
 * Returns the name of all the workspaces.
 * \return Vector of strings.
 **/
std::vector<std::string> GetWorkspaceNames()
{
	return AnalysisDataService::Instance().getObjectNames();
}

/**
 * Returns the name of all the algorithms.
 * \return Vector of strings.
 **/
std::vector<std::string> GetAlgorithmNames()
{
	return AlgorithmFactory::Instance().getKeys();
}

/**
* Create the simple Python API module
**/
void createPythonSimpleAPI()
{
  //Redirect to static helper class
  Mantid::PythonAPI::SimplePythonAPI::createModule();
}
	
}
}

