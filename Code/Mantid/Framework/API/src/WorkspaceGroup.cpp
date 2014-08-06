//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/IPropertyManager.h"

#include <Poco/ScopedLock.h>

namespace Mantid
{
namespace API
{
  namespace
  {
    /// initialize depth parameter
    size_t MAXIMUM_DEPTH = 100;
    /// static logger object
    Kernel::Logger g_log("WorkspaceGroup");
  }

WorkspaceGroup::WorkspaceGroup() :
  Workspace(), 
  m_deleteObserver(*this, &WorkspaceGroup::workspaceDeleteHandle),
  m_replaceObserver(*this, &WorkspaceGroup::workspaceReplaceHandle),
  m_workspaces(), m_observingADS(false)
{
}

WorkspaceGroup::~WorkspaceGroup()
{
  observeADSNotifications(false);
}

/**
 * The format is:
 *   ID
 *   -- Name1
 *   -- Name2
 * @returns A formatted human-readable string specifying the contents of the group
 */
const std::string WorkspaceGroup::toString() const
{
  std::string descr = this->id() + "\n";
  Poco::Mutex::ScopedLock _lock(m_mutex);
  for(auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it)
  {
    descr += " -- " + (*it)->name() + "\n";
  }
  return descr;
}


/**
 * Turn on/off observing delete and rename notifications to update the group accordingly
 * It can be useful to turn them off when constructing the group.
 * @param observeADS :: If true observe the ADS notifications, otherwise disable them
 */
void WorkspaceGroup::observeADSNotifications(const bool observeADS)
{
  if( observeADS )
  {
    if(!m_observingADS)
    {
      AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);
      AnalysisDataService::Instance().notificationCenter.addObserver(m_replaceObserver);
      m_observingADS = true;
    }
  }
  else
  {
    if(m_observingADS)
    {
      AnalysisDataService::Instance().notificationCenter.removeObserver(m_deleteObserver);
      AnalysisDataService::Instance().notificationCenter.removeObserver(m_replaceObserver);
      m_observingADS = false;
    }
  }
}

/**
 * @param workspace :: A workspace to check.
 * @return :: True if the workspace is found.
 */
bool WorkspaceGroup::isInChildGroup(const Workspace &workspace) const
{
    Poco::Mutex::ScopedLock _lock(m_mutex);
    for(auto ws = m_workspaces.begin(); ws != m_workspaces.end(); ++ws)
    {
        // check child groups only
        WorkspaceGroup *group = dynamic_cast<WorkspaceGroup*>( ws->get() );
        if ( group )
        {
            if ( group->isInGroup( workspace ) ) return true;
        }
    }
    return false;
}

/**
 * Adds a workspace to the group. The workspace does not have to be in the ADS
 * @param workspace :: A shared pointer to a workspace to add. If the workspace already exists give a warning.
 */
void WorkspaceGroup::addWorkspace(Workspace_sptr workspace)
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  // check it's not there already
  auto it = std::find(m_workspaces.begin(), m_workspaces.end(), workspace);
  if ( it == m_workspaces.end() )
  {
    m_workspaces.push_back( workspace );
  }
  else
  {
    g_log.warning() << "Workspace already exists in a WorkspaceGroup" << std::endl;;
  }
}

/**
 * Does this group contain the named workspace?
 * @param wsName :: A string to compare
 * @returns True if the name is part of this group, false otherwise
 */
bool WorkspaceGroup::contains(const std::string & wsName) const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  for(auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it)
  {
    if ( (**it).name() == wsName ) return true;
  }
  return false;
}

/**
 * @param workspace A pointer to a workspace
 * @returns True if the workspace exists in the group, false otherwise
 */
bool WorkspaceGroup::contains(const Workspace_sptr & workspace) const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  auto iend = m_workspaces.end();
  auto it = std::find(m_workspaces.begin(),iend, workspace);
  return (it != iend);
}

/**
 * Adds the current workspace members to the given list
 * @param memberList
 */
void WorkspaceGroup::reportMembers(std::set<Workspace_sptr> & memberList) const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  memberList.insert(m_workspaces.begin(), m_workspaces.end());
}

/**
 * Returns the names of workspaces that make up this group. 
 * Note that this returns a copy as the internal vector can mutate while the vector is being iterated over.
 */
std::vector<std::string> WorkspaceGroup::getNames() const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  std::vector<std::string> out;
  for(auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it)
  {
    out.push_back( (**it).name() );
  }
  return out;
}

/**
 * Return the ith workspace
 * @param index The index within the group
 * @throws an out_of_range error if the index is invalid
 */
Workspace_sptr WorkspaceGroup::getItem(const size_t index) const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  if( index >= this->size() )
  {
    std::ostringstream os;
    os << "WorkspaceGroup - index out of range. Requested=" << index << ", current size=" << this->size();
    throw std::out_of_range(os.str());
  }
  return m_workspaces[index];
}

/**
 * Return the workspace by name
 * @param wsName The name of the workspace
 * @throws an out_of_range error if the workspace'sname not contained in the group's list of workspace names
 */
Workspace_sptr WorkspaceGroup::getItem(const std::string wsName) const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  for(auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it)
  {
    if ( (**it).name() == wsName ) return *it;
  }
  throw std::out_of_range("Workspace "+wsName+" not contained in the group");
}

/// Empty all the entries out of the workspace group. Does not remove the workspaces from the ADS.
void WorkspaceGroup::removeAll()
{
  m_workspaces.clear();
}

/** Remove the named workspace from the group. Does not delete the workspace from the AnalysisDataService.
 *  @param wsName :: The name of the workspace to be removed from the group.
 */
void WorkspaceGroup::removeByADS(const std::string& wsName)
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  auto it = m_workspaces.begin();
  for(; it != m_workspaces.end(); ++it)
  {
    if ( (**it).name() == wsName )
    {
      m_workspaces.erase(it);
      break;
    }
  }
}

/// Print the names of all the workspaces in this group to the logger (at debug level)
void WorkspaceGroup::print() const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  for (auto itr = m_workspaces.begin(); itr != m_workspaces.end(); ++itr)
  {
    g_log.debug() << "Workspace name in group vector =  " << (**itr).name() << std::endl;
  }
}

/**
 * Remove a workspace pointed to by an index. The workspace remains in the ADS if it was there
 *
 * @param index :: Index of a workspace to delete.
 */
void WorkspaceGroup::removeItem(const size_t index)
{
    Poco::Mutex::ScopedLock _lock(m_mutex);
    // do not allow this way of removing for groups in the ADS
    if ( ! name().empty() )
    {
        throw std::runtime_error("AnalysisDataService must be used to remove a workspace from group.");
    }
    if( index >= this->size() )
    {
      std::ostringstream os;
      os << "WorkspaceGroup - index out of range. Requested=" << index << ", current size=" << this->size();
      throw std::out_of_range(os.str());
    }
    auto it = m_workspaces.begin() + index;
    m_workspaces.erase( it );
}

/** Callback for a workspace delete notification
 *
 * Removes any deleted entries from the group.
 * This also deletes the workspace group when the last member of it gets deteleted.
 *
 * @param notice :: A pointer to a workspace delete notificiation object
 */
void WorkspaceGroup::workspaceDeleteHandle(Mantid::API::WorkspacePostDeleteNotification_ptr notice)
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  const std::string deletedName = notice->objectName();
  if( !this->contains(deletedName)) return;

  if( deletedName != this->getName() )
  {
    this->removeByADS(deletedName);
    if(isEmpty())
    {
      //We are about to get deleted so we don't want to recieve any notifications
      observeADSNotifications(false);
      AnalysisDataService::Instance().remove(this->getName());
    }
  }
}

/**
 * Callback when a after-replace notification is received
 * Replaces a member if it was replaced in the ADS.
 * @param notice :: A pointer to a workspace after-replace notificiation object
 */
void WorkspaceGroup::workspaceReplaceHandle(Mantid::API::WorkspaceBeforeReplaceNotification_ptr notice)
{
  Poco::Mutex::ScopedLock _lock(m_mutex);

  const std::string replacedName = notice->objectName();
  for(auto citr=m_workspaces.begin(); citr!=m_workspaces.end(); ++citr)
  {
    if ( (**citr).name() == replacedName )
    {
      *citr = notice->newObject();
      break;
    }
  }
}

/**
 * This method returns true if the workspace group is empty
 * @return true if workspace is empty
 */
bool WorkspaceGroup::isEmpty() const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
	return m_workspaces.empty();
}

//------------------------------------------------------------------------------
/** Are the members of this group of similar names,
 * e.g. for a WorkspaceGroup names "groupname",
 * the members are "groupname_1", "groupname_2", etc.
 *
 * @return true if the names match this pattern.
 */
bool WorkspaceGroup::areNamesSimilar() const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  if(m_workspaces.empty()) return false;

  //Check all the members are of similar names
  for(auto citr=m_workspaces.begin(); citr!=m_workspaces.end(); ++citr)
  {
    const std::string wsName = (**citr).name();
    // Find the last underscore _
    std::size_t pos = wsName.find_last_of("_");
    // No underscore = not similar
    if(pos==std::string::npos)
      return false;
    // The part before the underscore has to be the same
    // as the group name to be similar
    std::string commonpart(wsName.substr(0,pos));
    if (this->name() != commonpart)
      return false;
  }
  return true;
}

//------------------------------------------------------------------------------
/**
Determine in the WorkspaceGroup is multiperiod.
* @return True if the WorkspaceGroup instance is multiperiod.
*/
bool WorkspaceGroup::isMultiperiod() const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  if(m_workspaces.size() < 1)
  {
    g_log.debug("Not a multiperiod-group with < 1 nested workspace.");
    return false;
  }
  std::vector<Workspace_sptr>::const_iterator iterator = m_workspaces.begin();
  // Loop through all inner workspaces, checking each one in turn.
  while(iterator != m_workspaces.end())
  {
    if(MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(*iterator))
    {
      try
      {
      Kernel::Property* nPeriodsProp = ws->run().getLogData("nperiods");
      int num = -1;
      Kernel::Strings::convert(nPeriodsProp->value(), num);
      if(num < 1)
      {
        g_log.debug("Not a multiperiod-group with nperiods log < 1.");
        return false;
      }
      }
      catch(Kernel::Exception::NotFoundError&)
      {
        g_log.debug("Not a multiperiod-group without nperiods log on all nested workspaces.");
        return false;
      }
    }
    else
    {
      g_log.debug("Not a multiperiod-group unless all inner workspaces are Matrix Workspaces.");
      return false;
    }
    ++iterator;
  }
  return true;
}

/**
 * @param workspace :: A workspace to check.
 * @param level :: The current nesting level. Intended for internal use only by WorkspaceGroup.
 * @return :: True if the worspace is found in any of the nested groups in this group.
 */
bool WorkspaceGroup::isInGroup(const Workspace &workspace, size_t level) const
{
    // Check for a cycle.
    if ( level > MAXIMUM_DEPTH )
    {
        throw std::runtime_error("WorkspaceGroup nesting level is too deep.");
    }
    Poco::Mutex::ScopedLock _lock(m_mutex);
    for(auto ws = m_workspaces.begin(); ws != m_workspaces.end(); ++ws)
    {
        if ( ws->get() == &workspace ) return true;
        WorkspaceGroup *group = dynamic_cast<WorkspaceGroup*>( ws->get() );
        if ( group )
        {
            if ( group->isInGroup( workspace, level + 1 ) ) return true;
        }
    }
    return false;
}

} // namespace API
} // namespace Mantid

/// @cond TEMPLATE

namespace Mantid
{
  namespace Kernel
  {

    template<> MANTID_API_DLL
      Mantid::API::WorkspaceGroup_sptr IPropertyManager::getValue<Mantid::API::WorkspaceGroup_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::API::WorkspaceGroup_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::API::WorkspaceGroup_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected WorkspaceGroup.";
        throw std::runtime_error(message);
      }
    }

    template<> MANTID_API_DLL
      Mantid::API::WorkspaceGroup_const_sptr IPropertyManager::getValue<Mantid::API::WorkspaceGroup_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::API::WorkspaceGroup_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::API::WorkspaceGroup_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const WorkspaceGroup.";
        throw std::runtime_error(message);
      }
    }

  } // namespace Kernel
} // namespace Mantid

/// @endcond TEMPLATE

