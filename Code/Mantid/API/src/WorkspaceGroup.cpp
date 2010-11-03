//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace API
{

Kernel::Logger& WorkspaceGroup::g_log = Kernel::Logger::get("WorkspaceGroup");

WorkspaceGroup::WorkspaceGroup() : 
  Workspace(), m_deleteObserver(*this, &WorkspaceGroup::workspaceDeleteHandle),
  m_renameObserver(*this, &WorkspaceGroup::workspaceRenameHandle)
				     
{
  // Listen for delete and rename notifications to update the group
  // accordingly
  Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);
  Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_renameObserver);
}

WorkspaceGroup::~WorkspaceGroup()
{
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_deleteObserver);
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_renameObserver);
}

/** Add the named workspace to the group
 *  @param name The name of the workspace (in the AnalysisDataService) to add
 */
void WorkspaceGroup::add(const std::string& name)
{
  m_wsNames.push_back(name);
  g_log.debug() << "workspacename added to group vector =  " << name <<std::endl;
}

/**
 * Does this group contain the named workspace?
 * @param wsName A string to compare
 * @returns True if the name is part of this group, false otherwise
 */
bool WorkspaceGroup::contains(const std::string & wsName) const
{
  std::vector<std::string>::const_iterator itr = std::find(m_wsNames.begin(), m_wsNames.end(), wsName);
  return (itr != m_wsNames.end());
}

/// Empty all the entries out of the workspace group. Does not remove the workspaces from the ADS.
void WorkspaceGroup::removeAll()
{
  m_wsNames.clear();
}

/** Remove the named workspace from the group. Does not delete the workspace from the AnalysisDataService.
 *  @param name The name of the workspace to be removed from the group.
 */
void WorkspaceGroup::remove(const std::string& name)
{
  std::vector<std::string>::iterator itr = std::find(m_wsNames.begin(), m_wsNames.end(), name);
  if( itr != m_wsNames.end() )
  {
    m_wsNames.erase(itr);
  }
}

/// Removes all members of the group from the group AND from the AnalysisDataService
void WorkspaceGroup::deepRemoveAll()
{
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_deleteObserver);
   while (!m_wsNames.empty())
   {
	   AnalysisDataService::Instance().remove(m_wsNames.back());
	   m_wsNames.pop_back();

   }
   
  Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);
}

/// Print the names of all the workspaces in this group to the logger (at debug level)
void WorkspaceGroup::print() const
{
  std::vector<std::string>::const_iterator itr;
  for (itr = m_wsNames.begin(); itr != m_wsNames.end(); itr++)
  {
    g_log.debug() << "Workspace name in group vector =  " << *itr << std::endl;
  }
}

/**M
 * Callback for a workspace delete notification
 * @param notice A pointer to a workspace delete notificiation object
 */
void WorkspaceGroup::workspaceDeleteHandle(Mantid::API::WorkspaceDeleteNotification_ptr notice)
{  
  if( notice->object_name() != this->getName() )
  {
    remove(notice->object_name());
    if(isEmpty())
    {
      Mantid::API::AnalysisDataService::Instance().remove(this->getName());
    }
  }
}

/**
 * Callback for a workspace rename notification
 * @param notice A pointer to a workspace rename notfication object
 */
void WorkspaceGroup::workspaceRenameHandle(Mantid::API::WorkspaceRenameNotification_ptr notice)
{
  std::vector<std::string>::iterator itr = 
    std::find(m_wsNames.begin(), m_wsNames.end(), notice->object_name());
  if( itr != m_wsNames.end() )
  {
    (*itr) = notice->new_objectname();
  }
}

/**
 * This method returns true if the workspace group is empty
 */
bool WorkspaceGroup::isEmpty()
{
	return m_wsNames.empty();
}

} // namespace API
} // namespace Mantid
