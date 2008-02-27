//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AnalysisDataService.h"

namespace Mantid
{
namespace API
{

/** Add a pointer to a named workspace to the data service store.
 *  Upon addition, the data service assumes ownership of the workspace.
 * 
 *  @param name The user-given name for the workspace
 *  @param space A pointer to the workspace
 *  @throw runtime_error Thrown if problems adding workspace
 */
void AnalysisDataServiceImpl::add(const std::string& name, Workspace_sptr space)
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
  if ( ! m_spaces.insert(WorkspaceMap::value_type(name, space)).second)
  {
    g_log.error("Unable to insert workspace" + name);
    throw std::runtime_error("Unable to insert workspace");
  }
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
void AnalysisDataServiceImpl::addOrReplace(const std::string& name, Workspace_sptr space)
{
  //find if the workspace already exists
  WorkspaceMap::const_iterator it = m_spaces.find(name);
  if (m_spaces.end() != it)
  {
    //Yes it does
    Workspace_sptr existingSpace = it->second;
    //replace it.
    m_spaces[name] = space;

    //no need to delete it is a shared pointer
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
      g_log.error("Error adding workspace" + name);
      throw;
    }
    return;
  }
  // code should NEVER get here
  throw std::runtime_error("UNKNOWN error");	
}

/** Remove a workspace from the data service store.
 *  Upon removal, the workspace itself will be deleted.
 * 
 *  @param name The user-given name for the workspace 
 *  @throw runtime_error Thrown if workspace cannot be found
 */
void AnalysisDataServiceImpl::remove(const std::string& name)
{  
  // Get a iterator to the workspace and name
  WorkspaceMap::iterator it = m_spaces.find(name);
  if (it==m_spaces.end())
  {
    g_log.error("Workspace " + name + " cannot be found");
    throw Kernel::Exception::NotFoundError("Workspace cannot be found",name);
  }
  //no need to delete it is a shared pointer
  m_spaces.erase(it);
  return;
}

/// Clears all workspaces from the data service store
void AnalysisDataServiceImpl::clear()
{
  m_spaces.clear();
  return;
}

/** Retrieve a pointer to a workspace by name.
 * 
 *  @param name The name of the desired workspace
 *  @return A pointer to the requested workspace
 *  @throw runtime_error Thrown if workspace cannot be found
 */
Workspace_sptr AnalysisDataServiceImpl::retrieve(const std::string& name)  
{
  WorkspaceMap::const_iterator it = m_spaces.find(name);
  if (m_spaces.end() != it)
  {
    return it->second;
  }
  g_log.error("Workspace " + name + " not found");
  throw Kernel::Exception::NotFoundError("Workspace not found",name);
}

//----------------------------------------------------------------------
// Private member functions
//----------------------------------------------------------------------

/// Private constructor for singleton class
AnalysisDataServiceImpl::AnalysisDataServiceImpl() :
  g_log(Kernel::Logger::get("AnalysisDataService"))
{ 
	g_log.debug() << "Analysis Data Service created." << std::endl;
}

/** Private copy constructor
 *  Prevents singleton being copied
 */
AnalysisDataServiceImpl::AnalysisDataServiceImpl(const AnalysisDataServiceImpl&) :
  g_log(Kernel::Logger::get("AnalysisDataService"))
{ 
}

/** Private destructor
 *  Prevents client from calling 'delete' on the pointer handed 
 *  out by Instance
 */
AnalysisDataServiceImpl::~AnalysisDataServiceImpl()
{ 
	g_log.debug() << "Analysis Data Service destroyed." << std::endl;
}


} // namespace Kernel
} // namespace Mantid
