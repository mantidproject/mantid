// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include <iterator>
#include <random>
#include <sstream>

namespace Mantid::API {

//-------------------------------------------------------------------------
// Nested class methods
//-------------------------------------------------------------------------
/**
 * Constructor.
 * @param name :: The name of a workspace group.
 */
AnalysisDataServiceImpl::GroupUpdatedNotification::GroupUpdatedNotification(const std::string &name)
    : DataServiceNotification(name, AnalysisDataService::Instance().retrieve(name)) {}
/**
 * Returns the workspace pointer cast to WorkspaceGroup
 */
std::shared_ptr<const WorkspaceGroup> AnalysisDataServiceImpl::GroupUpdatedNotification::getWorkspaceGroup() const {
  return std::dynamic_pointer_cast<const WorkspaceGroup>(this->object());
}

//-------------------------------------------------------------------------
// Public methods
//-------------------------------------------------------------------------
/**
 * Is the given name a valid name for an object in the ADS
 * @param name A string containing a possible name for an object in the ADS
 * @return An empty string if the name is valid or an error message stating the
 * problem
 * if the name is unacceptable.
 */
const std::string AnalysisDataServiceImpl::isValid(const std::string &name) const {
  std::string error;
  const std::string &illegal = illegalCharacters();
  if (illegal.empty())
    return error; // Quick route out.
  const size_t length = name.size();
  for (size_t i = 0; i < length; ++i) {
    if (illegal.find_first_of(name[i]) != std::string::npos) {
      std::ostringstream strm;
      strm << "Invalid object name '" << name << "'. Names cannot contain any of the following characters: " << illegal;
      error = strm.str();
      break;
    }
  }
  return error;
}

/**
 * Overwridden add member to attach the name to the workspace when a workspace
 * object is added to the service
 * If the name already exists then this throws a std::runtime_error. If a
 * workspace group is added adds the
 * members which are not in the ADS yet.
 * @param name The name of the object
 * @param workspace The shared pointer to the workspace to store
 */
void AnalysisDataServiceImpl::add(const std::string &name, const std::shared_ptr<API::Workspace> &workspace) {
  auto group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
  verifyName(name, group);

  // Attach the name to the workspace
  if (workspace)
    workspace->setName(name);
  Kernel::DataService<API::Workspace>::add(name, workspace);

  // if a group is added add its members as well
  if (!group)
    return;

  group->observeADSNotifications(true);
  for (size_t i = 0; i < group->size(); ++i) {
    auto ws = group->getItem(i);
    std::string wsName = ws->getName();
    // if anonymous make up a name and add
    if (wsName.empty()) {
      wsName = name + "_" + std::to_string(i + 1);
    } else if (doesExist(wsName)) { // if ws is already there do nothing
      wsName.clear();
    }
    // add member workspace if needed
    if (!wsName.empty()) {
      add(wsName, ws);
    }
  }
}

/**
 * Overwridden addOrReplace member to attach the name to the workspace when a
 * workspace object is added to the service.
 * This will overwrite one of the same name. If the workspace is group adds or
 * replaces its members.
 * @param name The name of the object
 * @param workspace The shared pointer to the workspace to store
 */
void AnalysisDataServiceImpl::addOrReplace(const std::string &name, const std::shared_ptr<API::Workspace> &workspace) {
  auto group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
  verifyName(name, group);

  // Attach the name to the workspace
  if (workspace)
    workspace->setName(name);
  Kernel::DataService<API::Workspace>::addOrReplace(name, workspace);

  if (!group)
    return;
  group->observeADSNotifications(true);
  for (size_t i = 0; i < group->size(); ++i) {
    auto ws = group->getItem(i);
    std::string wsName = ws->getName();
    // make up a name for an anonymous workspace
    if (wsName.empty()) {
      wsName = name + "_" + std::to_string(i + 1);
    } else if (doesExist(wsName)) { // if ws is already there do nothing
      wsName.clear();
    }
    // add member workspace if needed
    if (!wsName.empty()) {
      addOrReplace(wsName, ws);
    }
  }
}

/**
 * Overridden rename member to attach the new name to the workspace when a
 * workspace object is renamed
 * @param oldName The old name of the object
 * @param newName The new name of the object
 */
void AnalysisDataServiceImpl::rename(const std::string &oldName, const std::string &newName) {

  auto oldWorkspace = retrieve(oldName);
  auto group = std::dynamic_pointer_cast<WorkspaceGroup>(oldWorkspace);
  if (group && group->containsInChildren(newName)) {
    throw std::invalid_argument("Unable to rename group as the new name matches its members");
  }

  Kernel::DataService<API::Workspace>::rename(oldName, newName);
  // Attach the new name to the workspace
  auto ws = retrieve(newName);
  ws->setName(newName);
}

/**
 * Overridden remove member to delete its name held by the workspace itself.
 * It is important to do if the workspace isn't deleted after removal.
 * @param name The name of a workspace to remove.
 * @return The workspace being removed from the ADS
 */
Workspace_sptr AnalysisDataServiceImpl::remove(const std::string &name) {
  Workspace_sptr ws;
  try {
    ws = retrieve(name);
  } catch (const Kernel::Exception::NotFoundError &) {
    // do nothing - remove will do what's needed
  }
  Kernel::DataService<API::Workspace>::remove(name);
  if (ws) {
    ws->setName("");
  }
  return ws;
}

/**
 * @brief random lowercase letter used for generating workspace name in
 * unique_name and unique_hidden_name
 * @return Random Char
 */
char AnalysisDataServiceImpl::getRandomLowercaseLetter() {
  static const std::string alphabet = "abcdefghijklmnopqrstuvwxyz";
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, int(alphabet.size() - 1));
  return alphabet[dis(gen)];
}

/**
 * @brief generate a unique name that will not collide with any other workspace name
 * @param n Size of the sequence (must be a positive number)
 * @param prefix String to prefix the random name
 * @param suffix String to suffix the random name
 * @return String (prefix + n*random char + suffix)
 */
const std::string AnalysisDataServiceImpl::uniqueName(const int n, const std::string &prefix,
                                                      const std::string &suffix) {
  if (n <= 0) {
    throw std::invalid_argument("n must be a positive number");
  }
  auto randomNameGenerator = [n]() {
    std::string name;
    for (int i = 0; i < n; ++i) {
      name += getRandomLowercaseLetter();
    }
    return name;
  };

  // limit of (n * 10 * size of alphabet) to avoid infinite loop in case we can't find name that doesn't collide
  for (int i = 0; i < (n * 260); i++) {
    std::string wsName = prefix + randomNameGenerator() + suffix;
    if (!doesExist(wsName)) {
      return wsName;
    }
  }
  throw std::runtime_error("Unable to generate unique workspace of length " + std::to_string(n));
}

/**
 * @brief generate a unique hidden name to be used as a temporary workspace
 * @return String ("__" + 9*random char)
 */
const std::string AnalysisDataServiceImpl::uniqueHiddenName() { return AnalysisDataServiceImpl::uniqueName(9, "__"); }

/**
 * @brief Given a list of names retrieve the corresponding workspace handles
 * @param names A list of names of workspaces, if any does not exist then
 * a Kernel::Exception::NotFoundError is thrown.
 * @param unrollGroups If true flatten groups into the list of members.
 * @return A vector of pointers to Workspaces
 * @throws std::invalid_argument if no names are provided
 * @throws Mantid::Kernel::Exception::NotFoundError if a workspace does not
 * exist within the ADS
 */
std::vector<Workspace_sptr> AnalysisDataServiceImpl::retrieveWorkspaces(const std::vector<std::string> &names,
                                                                        bool unrollGroups) const {
  using WorkspacesVector = std::vector<Workspace_sptr>;
  WorkspacesVector workspaces;
  workspaces.reserve(names.size());
  std::transform(std::begin(names), std::end(names), std::back_inserter(workspaces),
                 [this](const std::string &name) { return this->retrieve(name); });
  assert(names.size() == workspaces.size());
  if (unrollGroups) {
    using IteratorDifference = std::iterator_traits<WorkspacesVector::iterator>::difference_type;

    bool done{workspaces.size() == 0};
    size_t i{0};
    while (!done) {
      if (auto group = std::dynamic_pointer_cast<WorkspaceGroup>(workspaces.at(i))) {
        const auto groupLength(group->size());
        workspaces.erase(std::next(std::begin(workspaces), static_cast<IteratorDifference>(i)));
        for (size_t j = 0; j < groupLength; ++j) {
          workspaces.insert(std::next(std::begin(workspaces), static_cast<IteratorDifference>(i + j)),
                            group->getItem(j));
        }
        i += groupLength;
      } else {
        ++i;
      }

      if (i == workspaces.size()) {
        done = true;
      }
    }
  }
  return workspaces;
}

/**
 * Sort members by Workspace name. The group must be in the ADS.
 * @param groupName :: A group name.
 */
void AnalysisDataServiceImpl::sortGroupByName(const std::string &groupName) {
  WorkspaceGroup_sptr group = retrieveWS<WorkspaceGroup>(groupName);
  if (!group) {
    throw std::runtime_error("Workspace " + groupName + " is not a workspace group.");
  }
  group->sortMembersByName();
  notificationCenter.postNotification(new GroupUpdatedNotification(groupName));
}

/**
 * Add a workspace to a group. The group and the workspace must be in the ADS.
 * @param groupName :: A group name.
 * @param wsName :: Name of a workspace to add to the group.
 */
void AnalysisDataServiceImpl::addToGroup(const std::string &groupName, const std::string &wsName) {
  WorkspaceGroup_sptr group = retrieveWS<WorkspaceGroup>(groupName);
  if (!group) {
    throw std::runtime_error("Workspace " + groupName + " is not a workspace group.");
  }

  if (groupName == wsName) {
    throw std::runtime_error("The group name and workspace name are the same");
  }

  auto ws = retrieve(wsName);
  group->addWorkspace(ws);
  notificationCenter.postNotification(new GroupUpdatedNotification(groupName));
}

/**
 * Remove a workspace group and all its members from the ADS.
 * @param name :: A group to remove.
 */
void AnalysisDataServiceImpl::deepRemoveGroup(const std::string &name) {
  WorkspaceGroup_sptr group = retrieveWS<WorkspaceGroup>(name);
  if (!group) {
    throw std::runtime_error("Workspace " + name + " is not a workspace group.");
  }
  group->observeADSNotifications(false);
  for (size_t i = 0; i < group->size(); ++i) {
    auto ws = group->getItem(i);
    WorkspaceGroup_sptr gws = std::dynamic_pointer_cast<WorkspaceGroup>(ws);
    if (gws) {
      // if a member is a group remove its items as well
      deepRemoveGroup(gws->getName());
    } else {
      remove(ws->getName());
    }
  }
  remove(name);
}

/**
 * Remove a workspace from a group but not from the ADS.
 *
 * @param groupName :: Name of a workspace group.
 * @param wsName :: Name of a workspace to remove.
 */
void AnalysisDataServiceImpl::removeFromGroup(const std::string &groupName, const std::string &wsName) {
  WorkspaceGroup_sptr group = retrieveWS<WorkspaceGroup>(groupName);
  if (!group) {
    throw std::runtime_error("Workspace " + groupName + " is not a workspace group.");
  }
  if (!group->contains(wsName)) {
    throw std::runtime_error("WorkspaceGroup " + groupName + " does not containt workspace " + wsName);
  }
  group->removeByADS(wsName);
  notificationCenter.postNotification(new GroupUpdatedNotification(groupName));
}

/**
 * Produces a map of names to Workspaces that doesn't include
 * items that are part of a WorkspaceGroup already in the list
 * @return A lookup of name to Workspace pointer
 */
std::map<std::string, Workspace_sptr> AnalysisDataServiceImpl::topLevelItems() const {
  std::map<std::string, Workspace_sptr> topLevel;
  auto topLevelNames = this->getObjectNames();
  std::set<Workspace_sptr> groupMembers;

  for (const auto &topLevelName : topLevelNames) {
    try {
      const std::string &name = topLevelName;
      auto ws = this->retrieve(topLevelName);
      topLevel.emplace(name, ws);
      if (auto group = std::dynamic_pointer_cast<WorkspaceGroup>(ws)) {
        group->reportMembers(groupMembers);
      }
    } catch (const std::exception &) {
    }
  }

  // Prune members
  for (auto it = topLevel.begin(); it != topLevel.end();) {
    const Workspace_sptr &item = it->second;
    if (groupMembers.count(item) == 1) {
      topLevel.erase(it++);
    } else {
      ++it;
    }
  }

  return topLevel;
}

//-------------------------------------------------------------------------
// Private methods
//-------------------------------------------------------------------------

/**
 * Constructor
 */
AnalysisDataServiceImpl::AnalysisDataServiceImpl()
    : Mantid::Kernel::DataService<Mantid::API::Workspace>("AnalysisDataService"), m_illegalChars() {}

// The following is commented using /// rather than /** to stop the compiler
// complaining
// about the special characters in the comment fields.
/// Return a string containing the characters not allowed in names objects
/// within ADS
/// @returns A n array of c strings containing the following characters: "
/// +-/*\%<>&|^~=!@()[]{},:.`$?"
const std::string &AnalysisDataServiceImpl::illegalCharacters() const { return m_illegalChars; }

/**
 * Set the list of illegal characeters
 * @param illegalChars A string containing the characters, as one long string,
 * that are not to be accepted by the ADS
 * NOTE: This only affects further additions to the ADS
 */
void AnalysisDataServiceImpl::setIllegalCharacterList(const std::string &illegalChars) {
  m_illegalChars = illegalChars;
}

/**
 * Checks the name is valid
 * @param name A string containing the name to check. If the name is invalid a
 * std::invalid_argument or std::runtime_error is thrown
 * @param group A WorkspaceGroup shared ptr which will be evaluated if it is
 * valid then it will check it's container for the same name that is passed if
 * true throws std::runtime_error else nothing.
 */

void AnalysisDataServiceImpl::verifyName(const std::string &name, const std::shared_ptr<API::WorkspaceGroup> &group) {
  const std::string error = isValid(name);
  if (!error.empty()) {
    throw std::invalid_argument(error);
  }

  if (group && group->containsInChildren(name)) {
    throw std::invalid_argument("Unable to add group as name matches its members");
  }
}

} // namespace Mantid::API
