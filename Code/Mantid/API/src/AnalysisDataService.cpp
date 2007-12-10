//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AnalysisDataService.h"

namespace Mantid
{
namespace API
{
	Kernel::Logger& AnalysisDataService::g_log = Kernel::Logger::get("AnalysisDataService");

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
	 *  @throw runtime_error Thrown if problems adding workspace
	 */
void AnalysisDataService::add(const std::string& name, Workspace* space)
{
  // Don't permit an empty name for the workspace
  if (name.empty())
  {
	  g_log.error("Workspace does not have a name");
	  throw std::runtime_error("Workspace does not have a name");
  }
  
  // At the moment, you can't overwrite a workspace (i.e. pass in a name
  // that's already in the map with a pointer to a different workspace).
  // Also, there's nothing to stop the same workspace from being added
  // more than once with different names.
  if (m_spaces.insert(WorkspaceMap::value_type(name, space)).second)
  {
	  return;
  }
	g_log.error("Unable to insert workspace" + name);
	throw std::runtime_error("Unable to insert workspace");
}

/** Add or replaces a pointer to a named workspace to the data service store.
	 *  Upon addition, the data service assumes ownership of the workspace.
	 *	if an existing workspace is already there it is replaced.
	 *	if the two workspaces are not pointers to the same workspace then the one that is replaced is deleted.
	 * 
	 *  @param name The user-given name for the workspace
	 *  @param space A pointer to the workspace
     *  @throw runtime_error Thrown if unable to add or replace workspace
	 */
void AnalysisDataService::addOrReplace(const std::string& name, Workspace* space)
{
	//find if the workspace already exists
	WorkspaceMap::const_iterator it = m_spaces.find(name);
	if (m_spaces.end() != it)
	{
		//Yes it does
		Workspace *existingSpace = it->second;
		//replace it.
		m_spaces[name] = space;

		if(existingSpace != space)
		{
			// Delete the workspace itself (care required on user's part - someone could still have a pointer to it)
			delete existingSpace;
		}
		return;
	}
	else
	{
		//no it doesn't we need to add it.	
		try
		{
			add(name,space);
		}
		catch(std::runtime_error& ex)
		{
			g_log.error("ADS: error adding workspace");
			throw;
		}
		return;
	}
	// code should NEVER get here
	throw std::runtime_error("ADS: UNKNOWN error");	
}

/** Remove a workspace from the data service store.
 *  Upon removal, the workspace itself will be deleted.
 * 
 *  @param name The user-given name for the workspace 
 *  @throw runtime_error Thrown if workspace cannot be found
 */
void AnalysisDataService::remove(const std::string& name)
{  
  // Get a iterator to the workspace and name
  WorkspaceMap::iterator it = m_spaces.find(name);
  if (it==m_spaces.end())
  {
	  g_log.error("ADS: Workspace " + name + " cannot be found");
	  throw std::runtime_error("ADS: Workspace cannot be found");
  }
  // Delete the workspace itself (care required on user's part - someone could still have a pointer to it)
  delete it->second;
  m_spaces.erase(it);
  return;
}

/** Retrieve a pointer to a workspace by name.
 * 
 *  @param name The name of the desired workspace
 *  @param space Returns a pointer to the requested workspace
 *  @throw runtime_error Thrown if workspace cannot be found
 */
void AnalysisDataService::retrieve(const std::string& name, Workspace *& space)  
{
  WorkspaceMap::const_iterator it = m_spaces.find(name);
  if (m_spaces.end() != it)
  {
    space = it->second;
	return;
  }
  g_log.error("ADS:Workspace " + name + " not found");
  throw std::runtime_error("ADS:Workspace not found");
}

//----------------------------------------------------------------------
// Private member functions
//----------------------------------------------------------------------

/// Private constructor for singleton class
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
{ 
  for (WorkspaceMap::iterator it = m_spaces.begin(); it != m_spaces.end(); ++it )
  {
    delete it->second;
  }
}

// Initialise the instance pointer to zero
AnalysisDataService* AnalysisDataService::m_instance = 0;

} // namespace Kernel
} // namespace Mantid
