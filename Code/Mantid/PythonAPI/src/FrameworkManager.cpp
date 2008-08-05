#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidPythonAPI/MantidSpectrogram.h"

#include "MantidPythonAPI/FrameworkManager.h"

using namespace Mantid;

namespace Mantid
{
namespace PythonAPI
{

FrameworkManager::FrameworkManager()
{
	API::FrameworkManager::Instance();
}
	
void FrameworkManager::clear()
{
	API::FrameworkManager::Instance().clear();
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
  * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName)
{
	return API::FrameworkManager::Instance().createAlgorithm(algName);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param version :: The version of the algorithm to use.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName, const int& version)
{
	return API::FrameworkManager::Instance().createAlgorithm(algName, version);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
  * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName, const std::string& propertiesArray)
{
	return API::FrameworkManager::Instance().createAlgorithm(algName, propertiesArray);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
 * \param version :: The version of the algorithm to use.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName, const std::string& propertiesArray,const int& version)
{
	return API::FrameworkManager::Instance().createAlgorithm(algName, propertiesArray, version);
}

/**
 * Creates and executes a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
 * \param version :: The version of the algorithm to use.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManager::execute(const std::string& algName, const std::string& propertiesArray,const int& version)
{
	return API::FrameworkManager::Instance().exec(algName, propertiesArray, version);
}

/**
 * Creates and executes a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManager::execute(const std::string& algName, const std::string& propertiesArray)
{
	return API::FrameworkManager::Instance().exec(algName, propertiesArray);
}

/**
 * Returns a specified workspace.
 * \param wsName :: The name of the workspace to retrieve.
 * \return Shared pointer to workspace.
 **/
API::Workspace* FrameworkManager::getWorkspace(const std::string& wsName)
{
	return API::FrameworkManager::Instance().getWorkspace(wsName);
}

/**
 * Deletes a specified workspace.
 * \param wsName :: The name of the workspace to delete.
 * \return Boolean result.
 **/
bool FrameworkManager::deleteWorkspace(const std::string& wsName)
{
	return API::FrameworkManager::Instance().deleteWorkspace(wsName);
}



}
}

