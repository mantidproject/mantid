//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/IPropertyManager.h"

#include <Poco/ScopedLock.h>
#include <boost/lexical_cast.hpp>

namespace Mantid
{
namespace API
{

Kernel::Logger& WorkspaceGroup::g_log = Kernel::Logger::get("WorkspaceGroup");

size_t WorkspaceGroup::g_maxNestingLevel = 5;

WorkspaceGroup::WorkspaceGroup(const bool observeADS) :
  Workspace(), 
  //m_replaceObserver(*this, &WorkspaceGroup::workspaceReplaceHandle),
  m_workspaces(), m_observingADS(false), m_nameCounter(0)
{
  observeADSNotifications(observeADS);
}

WorkspaceGroup::~WorkspaceGroup()
{
    observeADSNotifications(false);
}

/**
 * Override the base method to set names of member workspaces if they are empty.
 * Ensures that member names are unique in the ADS.
 *
 * @param name :: The new name.
 * @param force :: If true the name must be set (to realise workspace replacement in the ADS)
 */
void WorkspaceGroup::setName(const std::string &name, bool force)
{
    Workspace::setName(name, force);
    // record workspaces received new names to roll back in case of exception
    std::vector<bool> newNames(size(),false);
    size_t i = 0;
    for (auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it,++i)
    {
        std::string wsName = (**it).name();
        if ( wsName.empty() && ! name.empty() )
        {
            ++m_nameCounter;
            wsName = name + "_" + boost::lexical_cast<std::string>(m_nameCounter);
            // check that this name is unique in the ADS
            if ( AnalysisDataService::Instance().doesExist(wsName) )
            {
                auto ws = AnalysisDataService::Instance().retrieve( wsName );
                if ( ! force && ws != *it )
                {
                    // name is not unique: unset new names and throw
                    size_t n = static_cast<size_t>(it - m_workspaces.begin());
                    for(size_t j = 0; j < n; ++j)
                    {
                        if ( newNames[j] ) m_workspaces[j]->setName("",true);
                    }
                    throw std::runtime_error( "Cannot set name of workspace group member: name " + wsName + " already exists." );
                }
            }
            (**it).setName(wsName);
            newNames[i] = true;
        }
        else if ( name.empty() )
        {
            // setting empty name means workspace is being removed from the ADS.
            // this relies on ADS calling this method before removing group's pointer from the storage
            if ( AnalysisDataService::Instance().count(*it) <= 1 )
            {
                // set empty name only if this member is unique in the ADS
                (**it).setName("",true);
            }
        }
    }
}

/**
 * The size of the group is the sum of sizes of members.
 */
size_t WorkspaceGroup::getMemorySize() const
{
    size_t n = 0;
    for (auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it)
    {
        n += (**it).getMemorySize();
    }
    return n;
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
      //AnalysisDataService::Instance().notificationCenter.addObserver(m_replaceObserver);
      m_observingADS = true;
    }
  }
  else
  {
    if(m_observingADS)
    {
      //AnalysisDataService::Instance().notificationCenter.removeObserver(m_replaceObserver);
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
  AnalysisDataService::Instance().remove( name );
  ws->setName( name );
  addWorkspace( ws );
  g_log.debug() << "workspacename added to group vector =  " << name <<std::endl;
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
    if ( ! name().empty() )
    {
        // checking for non-empty name - a faster way of checking for being in the ADS
        if ( workspace->name().empty() )
        {
            ++m_nameCounter;
            std::string wsName = name() + "_" + boost::lexical_cast<std::string>(m_nameCounter);
            workspace->setName(wsName);
        }
        else
        {
            std::string wsName = workspace->name();
            bool observing = m_observingADS;
            observeADSNotifications( false );
            AnalysisDataService::Instance().removeFromTopLevel( workspace->name() );
            workspace->setName(wsName);
            observeADSNotifications( observing );
        }
    }
    updated();
  }
  else
  {
    g_log.warning() << "Workspace already exists in a WorkspaceGroup" << std::endl;;
  }
}

/**
 * Remove a workspace if it is in the group. Doesn't look into nested groups.
 * Removes the workspace from ADS (if it was there). If ADS has multiple copies
 * of the workspace only one of them is removed.
 *
 * @param workspace :: A workspace to remove.
 */
void WorkspaceGroup::removeWorkspace(Workspace_sptr workspace)
{
    Poco::Mutex::ScopedLock _lock(m_mutex);
    for(auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it)
    {
      if ( *it == workspace )
      {
          m_workspaces.erase(it);
          return;
      }
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

/**
 * Find a workspace by name. Search recursively in any nseted groups.
 *
 * @param wsName :: The name of the workspace.
 * @param convertToUpperCase :: Set true if wsName needs to be converted to upper case before search.
 * @param nesting :: Current nesting level. To detect cycles.
 * @return :: Return a shared pointer to the workspace or an empty pointer if not found.
 */
Workspace_sptr WorkspaceGroup::findItem(const std::string wsName, bool convertToUpperCase, size_t nesting) const
{
    if ( nesting >= g_maxNestingLevel )
    {
        // check for cycles
        throw std::runtime_error("Workspace group nesting is too deep. Could be a cycle.");
    }

    std::string foundName = wsName;
    if ( convertToUpperCase )
    {
        std::transform(foundName.begin(), foundName.end(), foundName.begin(),toupper);
    }
    Poco::Mutex::ScopedLock _lock(m_mutex);
    for(auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it)
    {
      Workspace *ws = it->get();
      if ( ws->getUpperCaseName() == foundName ) return *it;
      WorkspaceGroup* wsg = dynamic_cast<WorkspaceGroup*>(ws);
      if ( wsg )
      {
          // look in member groups recursively
          auto res = wsg->findItem(foundName, false, nesting + 1);
          if ( res ) return res;
      }
    }
    return Workspace_sptr();
}

/**
 * Look recursively for a workspace and count its instances.
 * @param workspace :: Workspace to look for.
 * @param nesting :: Current nesting level. To detect cycles.
 * @return :: Number of copies.
 * @throw :: std::runtime_error if nesting level exceeds maximum.
 */
size_t WorkspaceGroup::count(Workspace_const_sptr workspace, size_t nesting) const
{
    if ( nesting >= g_maxNestingLevel )
    {
        // check for cycles
        throw std::runtime_error("Workspace group nesting is too deep. Could be a cycle.");
    }

    size_t n = 0;
    for(auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it)
    {
        if ( (*it) == workspace ) n += 1;
        const Workspace *ws = it->get();
        const WorkspaceGroup* wsg = dynamic_cast<const WorkspaceGroup*>(ws);
        if ( wsg )
        {
            n += wsg->count( workspace, nesting + 1 );
        }
    }
    return n;
}

/// Empty all the entries out of the workspace group. Does not remove the workspaces from the ADS.
void WorkspaceGroup::removeAll()
{
    for(auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it)
    {
        auto ws = *it;
        if ( ! ws->name().empty() )
        {
            // leave removed workspace in the ADS
            std::string wsName = ws->name();
            ws->setName("",true);
            AnalysisDataService::Instance().add( wsName, ws );
        }
    }
    m_workspaces.clear();
}

/** Remove the named workspace from the group. Does not delete the workspace from the AnalysisDataService.
 *  @param wsName :: The name of the workspace to be removed from the group.
 */
void WorkspaceGroup::remove(const std::string& wsName)
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  auto it = m_workspaces.begin();
  for(; it != m_workspaces.end(); ++it)
  {
    if ( (**it).name() == wsName )
    {
      // leave removed workspace in the ADS
      auto ws = *it;
      m_workspaces.erase(it);
      AnalysisDataService::Instance().add( ws->name(), ws );
      break;
    }
  }
  updated();
}

/**
 *  Search in nested groups for a name and remove the workspace if found.
 * @param name :: Name of a workspace to remove.
 * @param convertToUpperCase  :: Set true if wsName needs to be converted to upper case before search.
 * @param nesting :: Current nesting level. To detect cycles.
 * @return :: True in success.
 */
void WorkspaceGroup::deepRemove(const std::string &name, bool convertToUpperCase, size_t nesting)
{
    if ( nesting >= g_maxNestingLevel )
    {
        // check for cycles
        throw std::runtime_error("Workspace group nesting is too deep. Could be a cycle.");
    }

    std::string upperName = name;
    if ( convertToUpperCase )
    {
        std::transform(upperName.begin(), upperName.end(), upperName.begin(),toupper);
    }

    Poco::Mutex::ScopedLock _lock(m_mutex);

    for(auto it = m_workspaces.begin(); it != m_workspaces.end();)
    {
      if ( (**it).getUpperCaseName() == upperName )
      {
        (**it).setName("",true);
        it = m_workspaces.erase(it);
      }
      else
      {
          WorkspaceGroup* wsg = dynamic_cast<WorkspaceGroup*>( it->get() );
          if ( wsg )
          {
              wsg->deepRemove( upperName, false, nesting + 1);
          }
          ++it;
      }

    }

    // empty top-level groups must be removed from the ADS
    if ( nesting == 0 && m_workspaces.empty() && ! getName().empty() )
    {
        AnalysisDataService::Instance().remove( getName() );
    }
}

/// Removes all members of the group from the group AND from the AnalysisDataService
void WorkspaceGroup::deepRemoveAll()
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  while (!m_workspaces.empty())
  {
    AnalysisDataService::Instance().remove(m_workspaces.back()->name());
    m_workspaces.pop_back();
  }
}

/**
 * Print the names of all the workspaces in this group to the logger (at debug level)
 *
 * @param padding :: A string to put in front of each output line. Used to create
 * different identation for nested groups.
 */
void WorkspaceGroup::print(const std::string &padding) const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  for (auto itr = m_workspaces.begin(); itr != m_workspaces.end(); ++itr)
  {
    g_log.debug() << padding << (**itr).name() << std::endl;
    std::cerr << padding << (**itr).name() << std::endl;
    const WorkspaceGroup *grp = dynamic_cast<const WorkspaceGroup*>( itr->get() );
    if ( grp )
    {
        grp->print( padding + "  " );
    }
  }
}

/**
 * Callback when a after-replace notification is received
 * Replaces a member if it was replaced in the ADS.
 * @param notice :: A pointer to a workspace after-replace notificiation object
 */
//void WorkspaceGroup::workspaceReplaceHandle(Mantid::API::WorkspaceBeforeReplaceNotification_ptr notice)
//{
//  Poco::Mutex::ScopedLock _lock(m_mutex);
//  bool isObserving = m_observingADS;
//  if ( isObserving )
//    observeADSNotifications( false );
//  const std::string replacedName = notice->object_name();
//  for(auto citr=m_workspaces.begin(); citr!=m_workspaces.end(); ++citr)
//  {
//    if ( (**citr).name() == replacedName )
//    {
//      *citr = notice->new_object();
//      break;
//    }
//  }
//  if ( isObserving )
//    observeADSNotifications( true );
//}

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
 * If the group observes the ADS it will send the GroupUpdatedNotification.
 */
void WorkspaceGroup::updated() const
{
  Poco::Mutex::ScopedLock _lock(m_mutex);
  if ( m_observingADS )
  {
    try
    {
      AnalysisDataService::Instance().notificationCenter.postNotification(
        new GroupUpdatedNotification( name() ));
    }
    catch( ... )
    {
      // if this workspace is not in the ADS do nothing
    }
  }
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

