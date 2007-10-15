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
#include "../inc/AlgorithmFactory.h"
#include "../inc/WorkspaceFactory.h"
#include "../inc/AnalysisDataService.h"
#include "../../DataHandling/inc/LoadRaw.h"
#include "../../DataHandling/inc/SaveCSV.h"
#include "../../Algorithms/inc/SimpleIntegration.h"
#include "../../DataObjects/inc/Workspace2D.h"
#include "../../DataObjects/inc/Workspace1D.h"

#include <stdexcept> 

using namespace std;

namespace Mantid
{

//----------------------------------------------------------------------
// Public member functions
//----------------------------------------------------------------------

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
  // These lines will disappear once automatic registration is implemented
  algFactory->subscribe("LoadRaw", ConcreteAlgorithmCreator<LoadRaw>::createInstance );
  algFactory->subscribe("SaveCSV", ConcreteAlgorithmCreator<SaveCSV>::createInstance );
  algFactory->subscribe("SimpleIntegration", ConcreteAlgorithmCreator<SimpleIntegration>::createInstance );
  workFactory->registerWorkspace("Workspace2D", Workspace2D::create );
  workFactory->registerWorkspace("Workspace1D", Workspace1D::create );
}

IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName)
{
  IAlgorithm *alg;
  // Get the algorithm from the factory
  StatusCode status = algFactory->createAlgorithm(algName, alg);
  if (status.isFailure())
  {
    throw runtime_error("Unable to create algorithm " + algName);
  }
  return alg;
}

IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName, const std::string& propertiesArray)
{
  // Use the previous method to create the algorithm
  IAlgorithm *alg = createAlgorithm(algName);
  // Split up comma-separated properties into a vector
  vector<string> propPairs = SplitString(propertiesArray, ",");
  // Iterate over the properties
  for (vector<string>::const_iterator it = propPairs.begin(); it != propPairs.end(); ++it)
  {
    // Split colon-separated name-value pairs
    vector<string> property = SplitString(*it,":");
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
  // Have to initialise a newly created algorithm before executing
  StatusCode status = alg->initialize();
  if (status.isFailure())
  {
    throw runtime_error("Unable to initialise algorithm " + algName);
  }
  // Now execute the algorithm
  status = alg->execute();
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

//----------------------------------------------------------------------
// Private member functions
//----------------------------------------------------------------------

std::vector<std::string> FrameworkManager::SplitString(const std::string& input,
            const std::string& delimiter, bool includeEmpties)
{
  vector<string> results;
  int iPos = 0;
  int newPos = -1;
  int sizeS2 = (int)delimiter.size();
  int isize = (int)input.size();
  
  if( ( isize == 0 ) || ( sizeS2 == 0 ) )
  {
    results.push_back(input);
    return results;
  }

  vector<int> positions;

  newPos = input.find (delimiter, 0);
  if( newPos < 0 )
  { 
    results.push_back(input);
    return results; 
  }

  int numFound = 0;
  while( newPos >= iPos )
  {
    numFound++;
    positions.push_back(newPos);
    iPos = newPos;
    newPos = input.find (delimiter, iPos+sizeS2);
  }

  if( numFound == 0 )
  {
    results.push_back(input);
    return results;
  }
  
  for( int i=0; i <= (int)positions.size(); ++i )
  {
    string s("");
    if( i == 0 ) 
    { 
      s = input.substr( i, positions[i] ); 
    }
    int offset = positions[i-1] + sizeS2;
    if( offset < isize )
    {
      if( i == (int)positions.size() )
      {
        s = input.substr(offset);
      }
      else if( i > 0 )
      {
        s = input.substr( positions[i-1] + sizeS2, 
                          positions[i] - positions[i-1] - sizeS2 );
      }
    }
    if( includeEmpties || ( s.size() > 0 ) )
    {
      results.push_back(s);
    }
  }
  return results;
}


} // Namespace Mantid
