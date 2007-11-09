//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "AnalysisDataService.h"

namespace Mantid
{
namespace Kernel
{
// Logger& AnalysisDataService::g_log = Logger::get("AnalysisDataService");

/** Static method which retrieves the single instance of the Analysis data service
  * 
  *  @returns A pointer to the service instance
  */
AnalysisDataService* AnalysisDataService::Instance()
{
  // Create the instance if not already created
  if (!m_instance) m_instance = new AnalysisDataService;
  return m_instance;
}

/** Add a pointer to a named workspace to the data service store.
	 *  Upon addition, the data service assumes ownership of the workspace.
	 * 
	 *  @param name The user-given name for the workspace
	 *  @param space A pointer to the workspace
	 *  @return A StatusCode object indicating whether the operation was successful
	 */
StatusCode AnalysisDataService::add(const std::string& name, Workspace* space)
{
  // At the moment, you can't overwrite a workspace (i.e. pass in a name
  // that's already in the map with a pointer to a different workspace).
  // Also, there's nothing to stop the same workspace from being added
  // more than once with different names.
  if (m_spaces.insert(WorkspaceMap::value_type(name, space)).second)
  {
    return StatusCode::SUCCESS;
  }
  return StatusCode::FAILURE;
}

/** Remove a workspace from the data service store.
 *  Upon removal, the workspace itself will be deleted.
 * 
 *  @param name The user-given name for the workspace
 *  @return A StatusCode object indicating whether the operation was successful
 */
StatusCode AnalysisDataService::remove(const std::string& name)
{  
  // Get a iterator to the workspace and naem
  WorkspaceMap::iterator it = m_spaces.find(name);
  if (it==m_spaces.end()) return StatusCode::FAILURE;	  
  // Delete the workspace itself (care required on user's part - someone could still have a pointer to it)
  delete it->second;
  m_spaces.erase(it);
  return StatusCode::SUCCESS;
}

/** Retrieve a pointer to a workspace by name.
 * 
 *  @param name The name of the desired workspace
 *  @param space Returns a pointer to the requested workspace
 *  @return A StatusCode object indicating whether the operation was successful
 */
StatusCode AnalysisDataService::retrieve(const std::string& name, Workspace *& space)  
{
  WorkspaceMap::const_iterator it = m_spaces.find(name);
  if (m_spaces.end() != it)
  {
    space = it->second;
    return StatusCode::SUCCESS;
  }
  return StatusCode::FAILURE;
}

//----------------------------------------------------------------------
// Private member functions
//----------------------------------------------------------------------

// Private constructor for singleton class
AnalysisDataService::AnalysisDataService() 
{ }

/** Private copy constructor
 *  Prevents singleton being copied
 */
AnalysisDataService::AnalysisDataService(const AnalysisDataService&)
{ }

/** Private destructor
 *  Prevents client from calling 'delete' on the pointer handed 
 *  out by Instance
 */
AnalysisDataService::~AnalysisDataService()
{ }

// Initialise the instance pointer to zero
AnalysisDataService* AnalysisDataService::m_instance = 0;

} // namespace Kernel
} // namespace Mantid
