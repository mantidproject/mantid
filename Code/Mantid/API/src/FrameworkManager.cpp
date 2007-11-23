//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "FrameworkManager.h"
#include "AlgorithmManager.h"
#include "WorkspaceFactory.h"
#include "AnalysisDataService.h"
#include "MantidKernel/ConfigService.h"
#include "IAlgorithm.h"
#include "MantidKernel/Exception.h"

#include <boost/tokenizer.hpp>
#include <string>

using namespace std;

namespace Mantid
{
namespace Kernel
{

Logger& FrameworkManager::g_log = Logger::get("FrameworkManager");

//----------------------------------------------------------------------
// Public member functions
//----------------------------------------------------------------------
		
/// Default constructor
FrameworkManager::FrameworkManager()
{
}

/// Destructor
FrameworkManager::~FrameworkManager()
{
}

/// Creates all of the required services
void FrameworkManager::initialize()
{
  // Required services are: the config service, the algorithm manager
  //     the analysis data service, the workspace factory
  config = ConfigService::Instance();
  algManager = AlgorithmManager::Instance();
  workFactory = WorkspaceFactory::Instance();
  data = AnalysisDataService::Instance();
  return;
}

/** At the moment clears all memory associated with AlgorithmManager.
 *  May do more in the future
 */
void FrameworkManager::clear()
{
    algManager->clear();
}

/** Creates an instance of an algorithm
 * 
 *  @param algName The name of the algorithm required
 *  @return A pointer to the created algorithm
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 */
IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName)
{
   IAlgorithm *alg = algManager->create(algName);
   return alg;
}

/** Creates an instance of an algorithm and sets the properties provided
 * 
 *  @param algName The name of the algorithm required
 *  @param propertiesArray A single string containing properties in the 
 *                         form "Property1:Value1,Property2:Value2,..."
 *  @return A pointer to the created algorithm
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 *  @throw std::invalid_argument Thrown if properties string is ill-formed
 */ 
IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName, const std::string& propertiesArray)
{
  // Use the previous method to create the algorithm
  IAlgorithm *alg = createAlgorithm(algName);
  // Split up comma-separated properties
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	
  boost::char_separator<char> sep(",");
  tokenizer propPairs(propertiesArray, sep);
  // Iterate over the properties
  for (tokenizer::iterator it = propPairs.begin(); it != propPairs.end(); ++it)
  {
    boost::char_separator<char> sep2(":");
    tokenizer properties(*it,sep2);
    vector<string> property(properties.begin(), properties.end());
    // Call the appropriate setProperty method on the algorithm
    if ( property.size() == 2)
    {
      alg->setProperty(property[0],property[1]);
    }
//    else if ( property.size() == 1)
//    {
//      // This is for a property with no value. Not clear that we will want such a thing.
//      alg->setProperty("",property[0]);
//    }
    // Throw if there's a problem with the string
    else
    {
		  throw std::invalid_argument("Misformed properties string");
    }
  }  
  return alg;
}

/** Creates an instance of an algorithm, sets the properties provided and
 *       then executes it.
 * 
 *  @param algName The name of the algorithm required
 *  @param propertiesArray A single string containing properties in the 
 *                         form "Property1:Value1,Property2:Value2,..."
 *  @return A pointer to the executed algorithm
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 *  @throw std::invalid_argument Thrown if properties string is ill-formed
 *  @throw runtime_error Thrown if algorithm cannot be executed
 */ 
IAlgorithm* FrameworkManager::exec(const std::string& algName, const std::string& propertiesArray)
{
  // Make use of the previous method for algorithm creation and property setting
  IAlgorithm *alg = createAlgorithm(algName, propertiesArray);
  
  // Now execute the algorithm
  StatusCode status = alg->execute();
  if (status.isFailure())
  {
    throw runtime_error("Unable to successfully execute algorithm " + algName);
  }  
  
  return alg;
}

/** Returns a shared pointer to the workspace requested
 * 
 *  @param wsName The name of the workspace
 *  @return A pointer to the workspace
 * 
 *  @throw NotFoundError If workspace is not registered with analysis data service
 */
Workspace* FrameworkManager::getWorkspace(const std::string& wsName)
{
  Workspace *space;
  StatusCode status = data->retrieve(wsName, space);
  if (status.isFailure())
  {
	  throw Exception::NotFoundError("Unable to retrieve workspace",wsName);
  }
  return space;
}

} // namespace Kernel
} // Namespace Mantid
