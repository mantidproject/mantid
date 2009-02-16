//---------------------------------------
//Includes
//---------------------------------------
#include "MantidPythonAPI/PythonInterface.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidPythonAPI/SimplePythonAPI.h"

using namespace Mantid::API;

/**
 * Returns the name of all the workspaces.
 * \return Vector of strings.
 **/
std::vector<std::string> Mantid::PythonAPI::GetWorkspaceNames()
{
	return AnalysisDataService::Instance().getObjectNames();
}

/**
 * Returns the name of all the algorithms.
 * \return Vector of strings.
 **/
std::vector<std::string> Mantid::PythonAPI::GetAlgorithmNames()
{
	return AlgorithmFactory::Instance().getKeys();
}

/**
  * Create the simple Python API module
  * @param gui Whether the module is being made for use with qtiplot or not
  **/
void Mantid::PythonAPI::createPythonSimpleAPI(bool gui)
{
  //Redirect to static helper class
  Mantid::PythonAPI::SimplePythonAPI::createModule(gui);
}
	
