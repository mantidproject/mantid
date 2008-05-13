//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <boost/tokenizer.hpp>
#include <string>
#include <iostream>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/LibraryManager.h"

using namespace std;

namespace Mantid
{
namespace API
{
/// Default constructor
FrameworkManagerImpl::FrameworkManagerImpl() : g_log(Kernel::Logger::get("FrameworkManager"))
{
	std::cerr << "Framework Manager created." << std::endl;
	g_log.debug() << "FrameworkManager created." << std::endl;
}

/// Destructor
FrameworkManagerImpl::~FrameworkManagerImpl()
{
//	std::cerr << "FrameworkManager destroyed." << std::endl;
//	g_log.debug() << "FrameworkManager destroyed." << std::endl;
}

/// Creates all of the required services
void FrameworkManagerImpl::initialize()
{ 
  std::string pluginDir = Kernel::ConfigService::Instance().getString("plugins.directory");
  if (pluginDir.length() > 0)
  {
    Mantid::Kernel::LibraryManager::Instance().OpenAllLibraries(pluginDir, false);
  }
  return;
}

/** At the moment clears all memory associated with AlgorithmManager.
 *  May do more in the future
 */
void FrameworkManagerImpl::clear()
{
  AlgorithmManager::Instance().clear();
  AnalysisDataService::Instance().clear();
}

/** Creates an instance of an algorithm
 * 
 *  @param algName The name of the algorithm required
 *  @param version The version of the algorithm
 *  @return A pointer to the created algorithm
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 */
IAlgorithm* FrameworkManagerImpl::createAlgorithm(const std::string& algName, const int& version)
{ 
   IAlgorithm* alg = AlgorithmManager::Instance().create(algName,version).get();
   return alg;
}

/** Creates an instance of an algorithm and sets the properties provided
 * 
 *  @param algName The name of the algorithm required
 *  @param propertiesArray A single string containing properties in the 
 *                         form "Property1=Value1;Property2=Value2;..."
 *  @param version The version of the algorithm
 *  @return A pointer to the created algorithm
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 *  @throw std::invalid_argument Thrown if properties string is ill-formed
 */ 
IAlgorithm* FrameworkManagerImpl::createAlgorithm(const std::string& algName,const std::string& propertiesArray, const int& version)
{
  // Use the previous method to create the algorithm
  IAlgorithm *alg = AlgorithmManager::Instance().create(algName,version).get();//createAlgorithm(algName);
  // Split up comma-separated properties
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	
  boost::char_separator<char> sep(";");
  tokenizer propPairs(propertiesArray, sep);
  int index=0;
  // Iterate over the properties
  for (tokenizer::iterator it = propPairs.begin(); it != propPairs.end(); ++it)
  {
    boost::char_separator<char> sep2("=");
    tokenizer properties(*it,sep2);
    vector<string> property(properties.begin(), properties.end());
    // Call the appropriate setProperty method on the algorithm
    if ( property.size() == 2)
    {
      alg->setPropertyValue(property[0],property[1]);
    }
    else if ( property.size() == 1)
    {
      // This is for a property with no value. Not clear that we will want such a thing.
      alg->setPropertyOrdinal(index,property[0]);
    }
    // Throw if there's a problem with the string
    else
    {
      throw std::invalid_argument("Misformed properties string");
    }
	index++;
  }  
  return alg;
}

/** Creates an instance of an algorithm, sets the properties provided and
 *       then executes it.
 * 
 *  @param algName The name of the algorithm required
 *  @param propertiesArray A single string containing properties in the 
 *                         form "Property1=Value1;Property2=Value2;..."
 *  @param version The version of the algorithm
 *  @return A pointer to the executed algorithm
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 *  @throw std::invalid_argument Thrown if properties string is ill-formed
 *  @throw runtime_error Thrown if algorithm cannot be executed
 */ 
IAlgorithm* FrameworkManagerImpl::exec(const std::string& algName, const std::string& propertiesArray, const int& version)
{
  // Make use of the previous method for algorithm creation and property setting
  IAlgorithm *alg = createAlgorithm(algName, propertiesArray,version);
  
  // Now execute the algorithm
  alg->execute();
  
  return alg;
}

/** Returns a shared pointer to the workspace requested
 * 
 *  @param wsName The name of the workspace
 *  @return A pointer to the workspace
 * 
 *  @throw NotFoundError If workspace is not registered with analysis data service
 */
Workspace* FrameworkManagerImpl::getWorkspace(const std::string& wsName)
{
  Workspace *space;
  try
  {
    space = AnalysisDataService::Instance().retrieve(wsName).get();
  }
  catch (Kernel::Exception::NotFoundError& ex)
  {
    throw Kernel::Exception::NotFoundError("Unable to retrieve workspace",wsName);
  }
  return space;
}

/** Removes and deletes a workspace from the data service store.
 * 
 *  @param wsName The user-given name for the workspace 
 *  @return true if the workspace was found and deleted
 * 
 *  @throw NotFoundError Thrown if workspace cannot be found
 */
bool FrameworkManagerImpl::deleteWorkspace(const std::string& wsName)
{
  bool retVal = false;
  try
  {
    AnalysisDataService::Instance().remove(wsName);
    retVal = true;
  }
  catch (Kernel::Exception::NotFoundError& ex)
  {
    //workspace was not found
    g_log.error()<<"Workspace "<<wsName<<" could not be found."<<std::endl;
    retVal = false;
  }
  return retVal;
}

} // namespace API
} // Namespace Mantid
