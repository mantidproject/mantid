//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid
{
namespace API
{

Kernel::Logger& WorkspaceGroup::g_log = Kernel::Logger::get("WorkspaceGroup");

WorkspaceGroup::WorkspaceGroup(const bool observeADS) :
  Workspace(), m_deleteObserver(*this, &WorkspaceGroup::workspaceDeleteHandle),
  m_renameObserver(*this, &WorkspaceGroup::workspaceRenameHandle),
  m_workspaces(), m_observingADS(false)
{
  observeADSNotifications(observeADS);
}

WorkspaceGroup::~WorkspaceGroup()
{
  observeADSNotifications(false);
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
      AnalysisDataService::Instance().notificationCenter.addObserver(m_renameObserver);
      m_observingADS = true;
    }
  }
  else
  {
    if(m_observingADS)
    {
      AnalysisDataService::Instance().notificationCenter.removeObserver(m_deleteObserver);
      AnalysisDataService::Instance().notificationCenter.removeObserver(m_renameObserver);
      m_observingADS = false;
    }
  }
}

/** Add the named workspace to the group. The workspace must exist in the ADS
 *  @param name :: The name of the workspace (in the AnalysisDataService) to add
 */
void WorkspaceGroup::add(const std::string& name)
{
  Workspace_sptr ws = AnalysisDataService::Instance().retrieve( name );
  addWorkspace( ws );
  g_log.debug() << "workspacename added to group vector =  " << name <<std::endl;
}

/**
 * Adds a workspace to the group. The workspace does not have to be in the ADS
 * @param workspace :: A shared pointer to a workspace to add. If the workspace already exists give a warning.
 */
void WorkspaceGroup::addWorkspace(Workspace_sptr workspace)
{
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
  for(auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it)
  {
    if ( (**it).name() == wsName ) return true;
  }
  return false;
}

/**
 * Returns the names of workspaces that make up this group. 
 * Note that this returns a copy as the internal vector can mutate while the vector is being iterated over.
 */
std::vector<std::string> WorkspaceGroup::getNames() const
{
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
 *  @param name :: The name of the workspace to be removed from the group.
 */
void WorkspaceGroup::remove(const std::string& wsName)
{
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

/// Removes all members of the group from the group AND from the AnalysisDataService
void WorkspaceGroup::deepRemoveAll()
{
  if( m_observingADS ) AnalysisDataService::Instance().notificationCenter.removeObserver(m_deleteObserver);
  while (!m_workspaces.empty())
  {
    AnalysisDataService::Instance().remove(m_workspaces.back()->name());
	  m_workspaces.pop_back();
  }
  if( m_observingADS ) AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);
}

/// Print the names of all the workspaces in this group to the logger (at debug level)
void WorkspaceGroup::print() const
{
  for (auto itr = m_workspaces.begin(); itr != m_workspaces.end(); ++itr)
  {
    g_log.debug() << "Workspace name in group vector =  " << (**itr).name() << std::endl;
    //std::cerr << "Workspace name in group vector =  " << (**itr).name() << std::endl;
  }
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
  const std::string deletedName = notice->object_name();
  if( !this->contains(deletedName)) return;

  if( deletedName != this->getName() )
  {
    this->remove(deletedName);
    if(isEmpty())
    {
      AnalysisDataService::Instance().remove(this->getName());
    }
  }
}

/**
 * This method returns true if the workspace group is empty
 * @return true if workspace is empty
 */
bool WorkspaceGroup::isEmpty() const
{
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

