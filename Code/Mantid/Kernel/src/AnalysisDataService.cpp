/*  
    The Analysis data service stores instances of the Workspace objects and 
    anything that derives from them.  This is the primary data service that
    the users will interact with either through writing scripts or directly
    through the API. It is implemented as a singleton class.
    
    @author Russell Taylor, Tessella Support Services plc
    @date 01/10/2007
    
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

#include "../inc/AnalysisDataService.h"

namespace Mantid
{
Logger& AnalysisDataService::g_log = Logger::get("AnalysisDataService");

// Returns the single instance of the service
AnalysisDataService* AnalysisDataService::Instance()
{
  // Create the instance if not already created
  if (!m_instance) m_instance = new AnalysisDataService;
  return m_instance;
}

// Destructor
AnalysisDataService::~AnalysisDataService()
{
  // Delete the map of workspaces and the single instance of the service
  delete m_spaces;
  delete m_instance;
}

// Adds a named workspace to the map
StatusCode AnalysisDataService::add(std::string name, Workspace * space)
{
  // At the moment, you can't overwrite a workspace (i.e. pass in a name
  // that's already in the map with a pointer to a different workspace).
  // Also, there's nothing to stop the same workspace from being added
  // more than once with different names.
  if (m_spaces->insert(WorkspaceMap::value_type(name, space)).second)
  {
    return StatusCode::SUCCESS;
  }
  return StatusCode::FAILURE;
}

// Removes a named workspace from the map and deletes it
StatusCode AnalysisDataService::remove(std::string name)
{
  Workspace* toBeRemoved;
  // Get a pointer to the workspace
  StatusCode status = retrieve(name, toBeRemoved);
  if (status.isFailure()) return status;
  // Remove the workspace from the map
  if (m_spaces->erase(name))
  {
    // Delete the workspace itself (care required on user's part - someone could still have a pointer to it)
    delete toBeRemoved;
    return StatusCode::SUCCESS;
  }
  return StatusCode::FAILURE;
}

// Retrieve a pointer to a workspace contained in the map
StatusCode AnalysisDataService::retrieve(std::string name, Workspace *& space)
{
  WorkspaceMap::const_iterator it = m_spaces->find(name);
  if (m_spaces->end() != it)
  {
    space = it->second;
    return StatusCode::SUCCESS;
  }
  return StatusCode::FAILURE;
}

// Private constructor
AnalysisDataService::AnalysisDataService()
{
  // Create the map to store the workspaces
  m_spaces = new WorkspaceMap;
}

// Initialise the instance pointer to zero
AnalysisDataService* AnalysisDataService::m_instance = 0;

}
