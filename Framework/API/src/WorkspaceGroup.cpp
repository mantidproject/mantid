// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"

namespace Mantid::API {
namespace {
/// initialize depth parameter
size_t MAXIMUM_DEPTH = 100;
/// static logger object
Kernel::Logger g_log("WorkspaceGroup");
} // namespace

WorkspaceGroup::WorkspaceGroup()
    : Workspace(), m_deleteObserver(*this, &WorkspaceGroup::workspaceDeleteHandle),
      m_beforeReplaceObserver(*this, &WorkspaceGroup::workspaceBeforeReplaceHandle), m_workspaces(),
      m_observingADS(false) {}

WorkspaceGroup::~WorkspaceGroup() { observeADSNotifications(false); }

/**
 * The format is:
 *   ID
 *   -- Name1
 *   -- Name2
 * @returns A formatted human-readable string specifying the contents of the
 * group
 */
const std::string WorkspaceGroup::toString() const {
  const std::string firstLine = this->id() + "\n";
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  const auto descr = std::accumulate(
      m_workspaces.cbegin(), m_workspaces.cend(), firstLine,
      [](const auto &string, const auto &workspace) { return string + " -- " + workspace->getName() + '\n'; });
  return descr;
}

/**
 * Turn on/off observing delete and rename notifications to update the group
 * accordingly
 * It can be useful to turn them off when constructing the group.
 * @param observeADS :: If true observe the ADS notifications, otherwise
 * disable them
 */
void WorkspaceGroup::observeADSNotifications(const bool observeADS) {
  if (observeADS) {
    if (!m_observingADS) {
      AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);
      AnalysisDataService::Instance().notificationCenter.addObserver(m_beforeReplaceObserver);
      m_observingADS = true;
    }
  } else {
    if (m_observingADS) {
      AnalysisDataService::Instance().notificationCenter.removeObserver(m_deleteObserver);
      AnalysisDataService::Instance().notificationCenter.removeObserver(m_beforeReplaceObserver);
      m_observingADS = false;
    }
  }
}

/**
 * @param workspaceToCheck :: A workspace to check.
 * @return :: True if the workspace is found.
 */
bool WorkspaceGroup::isInChildGroup(const Workspace &workspaceToCheck) const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  for (const auto &workspace : m_workspaces) {
    // check child groups only
    auto *group = dynamic_cast<WorkspaceGroup *>(workspace.get());
    if (group) {
      if (group->isInGroup(workspaceToCheck))
        return true;
    }
  }
  return false;
}

/**
 * Sort members by Workspace name
 */
void WorkspaceGroup::sortMembersByName() {
  if (this->size() == 0) {
    return;
  }
  std::sort(m_workspaces.begin(), m_workspaces.end(),
            [](const Workspace_sptr &w1, const Workspace_sptr &w2) { return (w1->getName() < w2->getName()); });
}

/**
 * Adds a workspace to the group. The workspace does not have to be in the
 * ADS
 * @param workspace :: A shared pointer to a workspace to add. If the
 * workspace already exists give a warning.
 */
void WorkspaceGroup::addWorkspace(const Workspace_sptr &workspace) {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  if (this == workspace.get()) {
    g_log.warning("Can't add a workspace as a child of itself!\n");
    return;
  }
  const auto it = std::find(m_workspaces.begin(), m_workspaces.end(), workspace);
  if (it == m_workspaces.end()) {
    m_workspaces.emplace_back(workspace);
  } else {
    g_log.warning() << "Workspace already exists in a WorkspaceGroup\n";
  }
}

/**
 * Does this group or any of it's child groups contain the named workspace?
 * @param wsName :: A string to compare
 * @returns True if the name is part of this group, false otherwise
 */
bool WorkspaceGroup::containsInChildren(const std::string &wsName) const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  for (const auto &workspace : m_workspaces) {
    if (workspace->isGroup()) {
      // Recursive containsInChildren search
      const auto group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
      if (group->containsInChildren(wsName)) {
        return true;
      }
    } else {
      if (workspace->getName() == wsName)
        return true;
    }
  }
  return false;
}

/**
 * Does this group contain the named workspace?
 * @param wsName :: A string to compare
 * @returns True if the name is part of this group, false otherwise
 */
bool WorkspaceGroup::contains(const std::string &wsName) const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  return std::any_of(m_workspaces.cbegin(), m_workspaces.cend(),
                     [&wsName](const auto &workspace) { return workspace->getName() == wsName; });
}

/**
 * @param workspace A pointer to a workspace
 * @returns True if the workspace exists in the group, false otherwise
 */
bool WorkspaceGroup::contains(const Workspace_sptr &workspace) const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  auto iend = m_workspaces.end();
  auto it = std::find(m_workspaces.begin(), iend, workspace);
  return (it != iend);
}

/**
 * Adds the current workspace members to the given list
 * @param memberList
 */
void WorkspaceGroup::reportMembers(std::set<Workspace_sptr> &memberList) const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  memberList.insert(m_workspaces.begin(), m_workspaces.end());
}

/**
 * Returns the names of workspaces that make up this group.
 * Note that this returns a copy as the internal vector can mutate while the
 * vector is being iterated over.
 */
std::vector<std::string> WorkspaceGroup::getNames() const {
  std::vector<std::string> out;
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  out.reserve(m_workspaces.size());

  std::transform(m_workspaces.begin(), m_workspaces.end(), std::back_inserter(out),
                 [](const auto &ws) { return ws->getName(); });

  return out;
}

/**
 * Return the ith workspace
 * @param index The index within the group
 * @throws an out_of_range error if the index is invalid
 */
Workspace_sptr WorkspaceGroup::getItem(const size_t index) const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  if (index >= this->size())
    this->throwIndexOutOfRangeError(static_cast<int>(index));
  return m_workspaces[index];
}

/**
 * Return the workspace by name
 * @param wsName The name of the workspace
 * @throws an out_of_range error if the workspace's name not contained in
 * the group's list of workspace names
 */
Workspace_sptr WorkspaceGroup::getItem(const std::string &wsName) const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  const auto found = std::find_if(m_workspaces.cbegin(), m_workspaces.cend(),
                                  [&wsName](const auto &workspace) { return workspace->getName() == wsName; });
  if (found == m_workspaces.cend()) {
    throw std::out_of_range("Workspace " + wsName + " not contained in the group");
  } else {
    return *found;
  }
}

/** Return all workspaces in the group as one call for thread safety
 */
std::vector<Workspace_sptr> WorkspaceGroup::getAllItems() const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  return m_workspaces;
}

/// Empty all the entries out of the workspace group. Does not remove the
/// workspaces from the ADS.
void WorkspaceGroup::removeAll() { m_workspaces.clear(); }

/** Remove the named workspace from the group. Does not delete the workspace
 * from the AnalysisDataService.
 *  @param wsName :: The name of the workspace to be removed from the group.
 */
void WorkspaceGroup::removeByADS(const std::string &wsName) {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  auto it = m_workspaces.begin();
  for (; it != m_workspaces.end(); ++it) {
    if ((**it).getName() == wsName) {
      m_workspaces.erase(it);
      break;
    }
  }
}

/// Print the names of all the workspaces in this group to the logger (at
/// debug level)
void WorkspaceGroup::print() const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  for (const auto &workspace : m_workspaces) {
    g_log.debug() << "Workspace name in group vector =  " << workspace->getName() << '\n';
  }
}

/*
 * Throws an out of range error for an out of range index
 * @param index The out of range index to be printed
 * @throws out_of_range error
 */
void WorkspaceGroup::throwIndexOutOfRangeError(int index) const {
  std::ostringstream os;
  os << "WorkspaceGroup - index out of range. Requested=" << index << ", current size=" << this->size();
  throw std::out_of_range(os.str());
}

/**
 * Returns an iterator pointing to the first element in the group.
 *
 * @return  A non-const iterator pointing to the first workspace in this
 *          workspace group.
 */
std::vector<Workspace_sptr>::iterator WorkspaceGroup::begin() { return m_workspaces.begin(); }

/**
 * Returns a const iterator pointing to the first element in the group.
 *
 * @return  A const iterator pointing to the first workspace in this
 *          workspace group.
 */
std::vector<Workspace_sptr>::const_iterator WorkspaceGroup::begin() const { return m_workspaces.begin(); }

/**
 * Returns an iterator pointing to the past-the-end element in the group.
 *
 * @return  A non-const iterator pointing to the last workspace in this
 *          workspace group.
 */
std::vector<Workspace_sptr>::iterator WorkspaceGroup::end() { return m_workspaces.end(); }

/** Returns a const iterator pointing to the past-the-end element in the
 * group.
 *
 * @return  A const iterator pointing to the last workspace in this
 *          workspace group.
 */
std::vector<Workspace_sptr>::const_iterator WorkspaceGroup::end() const { return m_workspaces.end(); }

/**
 * Remove a workspace pointed to by an index. The workspace remains in the
 * ADS if it was there
 *
 * @param index :: Index of a workspace to delete.
 */
void WorkspaceGroup::removeItem(const size_t index) {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  // do not allow this way of removing for groups in the ADS
  if (!this->getName().empty()) {
    throw std::runtime_error("AnalysisDataService must be used to remove a "
                             "workspace from group.");
  }
  if (index >= this->size()) {
    std::ostringstream os;
    os << "WorkspaceGroup - index out of range. Requested=" << index << ", current size=" << this->size();
    throw std::out_of_range(os.str());
  }
  auto it = m_workspaces.begin() + index;
  m_workspaces.erase(it);
}

/** Callback for a workspace delete notification
 *
 * Removes any deleted entries from the group.
 * This also deletes the workspace group when the last member of it gets
 * deleted.
 *
 * @param notice :: A pointer to a workspace delete notificiation object
 */
void WorkspaceGroup::workspaceDeleteHandle(Mantid::API::WorkspacePostDeleteNotification_ptr notice) {
  std::unique_lock<std::recursive_mutex> _lock(m_mutex);
  const std::string deletedName = notice->objectName();
  if (!this->contains(deletedName))
    return;

  if (deletedName != this->getName()) {
    this->removeByADS(deletedName);
    if (isEmpty()) {
      // We are about to get deleted so we don't want to recieve any
      // notifications
      // The unique lock needs to be unlocked at this point as the workspace
      // is about to destroy itself. We have to make sure that the mutex is
      // not locked.
      _lock.unlock();
      observeADSNotifications(false);
      AnalysisDataService::Instance().remove(this->getName());
    }
  }
}

/**
 * Callback when a before-replace notification is received
 * Replaces a member if it was replaced in the ADS and checks
 * for duplicate members within the group
 * @param notice :: A pointer to a workspace before-replace notification
 * object
 */
void WorkspaceGroup::workspaceBeforeReplaceHandle(Mantid::API::WorkspaceBeforeReplaceNotification_ptr notice) {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);

  const auto oldObject = notice->oldObject();
  const auto newObject = notice->newObject();

  bool foundOld(false);
  bool foundDuplicate(false);

  auto duplicateIter = m_workspaces.end();

  for (auto it = m_workspaces.begin(); it != m_workspaces.end(); ++it) {
    auto &workspace = *it;
    if (workspace == oldObject) {
      workspace = newObject;
      foundOld = true;

    } else if (workspace == newObject) {
      duplicateIter = it;
      foundDuplicate = true;
    }

    if (foundOld && foundDuplicate) {
      break;
    }
  }

  if (foundOld && duplicateIter != m_workspaces.end()) {
    m_workspaces.erase(duplicateIter);
  }
}

/**
 * This method returns true if the workspace group is empty
 * @return true if workspace is empty
 */
bool WorkspaceGroup::isEmpty() const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  return m_workspaces.empty();
}

//------------------------------------------------------------------------------
/** Are the members of this group of similar names,
 * e.g. for a WorkspaceGroup names "groupname",
 * the members are "groupname_1", "groupname_2", etc.
 *
 * @return true if the names match this pattern.
 */
bool WorkspaceGroup::areNamesSimilar() const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  if (m_workspaces.empty())
    return false;

  // Check all the members are of similar names
  for (const auto &workspace : m_workspaces) {
    const std::string &wsName = workspace->getName();
    // Find the last underscore _
    std::size_t pos = wsName.find_last_of('_');
    // No underscore = not similar
    if (pos == std::string::npos)
      return false;
    // The part before the underscore has to be the same
    // as the group name to be similar
    std::string commonpart(wsName.substr(0, pos));
    if (this->getName() != commonpart)
      return false;
  }
  return true;
}

//------------------------------------------------------------------------------
/**
Determine in the WorkspaceGroup is multiperiod.
* @return True if the WorkspaceGroup instance is multiperiod.
*/
bool WorkspaceGroup::isMultiperiod() const {
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  if (m_workspaces.empty()) {
    g_log.debug("Not a multiperiod-group with < 1 nested workspace.");
    return false;
  }
  // Loop through all inner workspaces, checking each one in turn.
  for (const auto &workspace : m_workspaces) {
    if (MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(workspace)) {
      try {
        Kernel::Property *nPeriodsProp = ws->run().getLogData("nperiods");
        int num = -1;
        Kernel::Strings::convert(nPeriodsProp->value(), num);
        if (num < 1) {
          g_log.debug("Not a multiperiod-group with nperiods log < 1.");
          return false;
        }
      } catch (Kernel::Exception::NotFoundError &) {
        g_log.debug("Not a multiperiod-group without nperiods log on all "
                    "nested workspaces.");
        return false;
      }
    } else {
      g_log.debug("Not a multiperiod-group unless all inner workspaces are "
                  "Matrix Workspaces.");
      return false;
    }
  }
  return true;
}

/**
 * @return :: True if all of the workspaces in the group are peak workspaces
 */
bool WorkspaceGroup::isGroupPeaksWorkspaces() const {
  return std::all_of(m_workspaces.begin(), m_workspaces.end(),
                     [](auto ws) { return dynamic_cast<IPeaksWorkspace *>(ws.get()) != nullptr; });
}

/**
 * @param workspaceToCheck :: A workspace to check.
 * @param level :: The current nesting level. Intended for internal use only
 * by WorkspaceGroup.
 * @return :: True if the worspace is found in any of the nested groups in
 * this group.
 */
bool WorkspaceGroup::isInGroup(const Workspace &workspaceToCheck, size_t level) const {
  // Check for a cycle.
  if (level > MAXIMUM_DEPTH) {
    throw std::runtime_error("WorkspaceGroup nesting level is too deep.");
  }
  std::lock_guard<std::recursive_mutex> _lock(m_mutex);
  for (const auto &workspace : m_workspaces) {
    if (workspace.get() == &workspaceToCheck)
      return true;
    const auto *group = dynamic_cast<WorkspaceGroup *>(workspace.get());
    if (group) {
      if (group->isInGroup(workspaceToCheck, level + 1))
        return true;
    }
  }
  return false;
}

size_t WorkspaceGroup::getMemorySize() const {
  auto total = std::size_t(0);
  // Go through each workspace
  for (auto workspace : m_workspaces) {
    // If the workspace is a group
    if (workspace->getMemorySize() == 0) {
      total = total + std::dynamic_pointer_cast<WorkspaceGroup>(workspace)->getMemorySize();
      continue;
    }
    total = total + workspace->getMemorySize();
  }
  return total;
}

} // namespace Mantid::API

/// @cond TEMPLATE

namespace Mantid::Kernel {

template <>
MANTID_API_DLL Mantid::API::WorkspaceGroup_sptr
IPropertyManager::getValue<Mantid::API::WorkspaceGroup_sptr>(const std::string &name) const {
  const auto *prop = dynamic_cast<PropertyWithValue<Mantid::API::WorkspaceGroup_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected shared_ptr<WorkspaceGroup>.";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_API_DLL Mantid::API::WorkspaceGroup_const_sptr
IPropertyManager::getValue<Mantid::API::WorkspaceGroup_const_sptr>(const std::string &name) const {
  const auto *prop = dynamic_cast<PropertyWithValue<Mantid::API::WorkspaceGroup_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected const shared_ptr<WorkspaceGroup>.";
    throw std::runtime_error(message);
  }
}

} // namespace Mantid::Kernel

/// @endcond TEMPLATE
