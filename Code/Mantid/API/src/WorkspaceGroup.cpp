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

WorkspaceGroup::WorkspaceGroup() : Workspace()
{}

WorkspaceGroup::~WorkspaceGroup()
{}

/** Add the named workspace to the group
 *  @param name The name of the workspace (in the AnalysisDataService) to add
 */
void WorkspaceGroup::add(const std::string& name)
{
  m_wsNames.push_back(name);
  g_log.debug() << "workspacename added to group vector =  " << name <<std::endl;
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
  std::vector<std::string>::iterator itr;
  for (itr = m_wsNames.begin(); itr != m_wsNames.end(); itr++)
  {
    if ((*itr) == name)
    {
      m_wsNames.erase(itr);
      return;
    }
  }
  // Getting to here means we have gone through the entire loop and not
  // found a match
  g_log.warning("Workspace  " + name + "not found in workspacegroup");
}

/// Removes all members of the group from the group AND from the AnalysisDataService
void WorkspaceGroup::deepRemoveAll()
{
  // Go through the workspace entries
  std::vector<std::string>::const_iterator itr;
  for (itr = m_wsNames.begin(); itr != m_wsNames.end(); itr++)
  {
    // Remove from the ADS. Doesn't throw if the workspace isn't there.
    AnalysisDataService::Instance().remove(*itr);
  }
  // Now empty out the list of members
  removeAll();
}

/// Print the names of all the workspaces in this group to the logger (at debug level)
void WorkspaceGroup::print() const
{
  std::vector<std::string>::const_iterator itr;
  for (itr = m_wsNames.begin(); itr != m_wsNames.end(); itr++)
  {
    g_log.debug() << "workspacename in group vector=  " << *itr << std::endl;
  }
}

} // namespace API
} // namespace Mantid
