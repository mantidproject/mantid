/*  The main public API via which users interact with the Mantid framework.

    @author Russell Taylor, Tessella Support Services plc
    @date 05/10/2007
    
    Copyright &copy; 2007 ???RAL???

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "../inc/FrameworkManager.h"
#include "../inc/AlgorithmManager.h"
#include "../inc/WorkspaceFactory.h"
#include "../inc/AnalysisDataService.h"
#include "../inc/ConfigSvc.h"
#include "../inc/IAlgorithm.h"

#include <stdexcept>
#include <boost/tokenizer.hpp>
#include <string>

using namespace std;

namespace Mantid
{
Logger& FrameworkManager::g_log = Logger::get("FrameworkManager");
//----------------------------------------------------------------------
// Public member functions
//----------------------------------------------------------------------
		
FrameworkManager::FrameworkManager()
{
}

FrameworkManager::~FrameworkManager()
{
}

std::string FrameworkManager::initialize()
{
  // Required services are: the config service, the algorithm manager
  //     the analysis data service, the workspace factory
  config = ConfigSvc::Instance();
  algManager = AlgorithmManager::Instance();
  workFactory = WorkspaceFactory::Instance();
  data = AnalysisDataService::Instance();
  return "Framework Manager initialised!";
}

void FrameworkManager::clear()
{
	algManager->clear();
}

IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName)
{
   IAlgorithm *alg = algManager->createAlgorithm(algName);
	return alg;
}

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
    else if ( property.size() == 1)
    {
      alg->setProperty(property[0]);
    }
    // Throw if there's a problem with the string
    else
    {
      throw runtime_error("Misformed properties string");
    }
  }  
  return alg;
}

IAlgorithm* FrameworkManager::exec(const std::string& algName, const std::string& propertiesArray)
{
  // Make use of the previous method for algorithm creation and property setting
  IAlgorithm *alg = createAlgorithm(algName, propertiesArray);
  
  // this is now performed by the algorithm manager
  // Have to initialise a newly created algorithm before executing
 /* StatusCode status = alg->initialize();
  if (status.isFailure())
  {
    throw runtime_error("Unable to initialise algorithm " + algName);
  } */
  // Now execute the algorithm
  StatusCode status = alg->execute();
  if (status.isFailure())
  {
    throw runtime_error("Unable to successfully execute algorithm " + algName);
  }  
  
  return alg;
}

Workspace* FrameworkManager::getWorkspace(const std::string& wsName)
{
  Workspace *space;
  StatusCode status = data->retrieve(wsName, space);
  if (status.isFailure())
  {
    throw runtime_error("Unable to retrieve workspace " + wsName);
  }
  return space;
}

} // Namespace Mantid
