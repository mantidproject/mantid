#ifndef MANTID_API_WORKSPACEGROUP_H
#define MANTID_API_WORKSPACEGROUP_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAPI/AnalysisDataService.h"

#include <Poco/NObserver.h>
#include <iterator>
#include <mutex>

namespace Mantid {

namespace API {
//----------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------
class Algorithm;

/** Class to hold a set of workspaces.
    The workspace group can be an entry in the AnalysisDataService.
    Its constituent workspaces should also have individual ADS entries.
    Workspace groups can be used in algorithms in the same way as single
   workspaces.

    @author Sofia Antony, ISIS, RAL
    @date 12/06/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_API_DLL WorkspaceGroup : public Workspace {
public:
  /// Default constructor.
  WorkspaceGroup(
      const Parallel::StorageMode storageMode = Parallel::StorageMode::Cloned);
  /// Destructor
  ~WorkspaceGroup() override;
  /// Return a string ID of the class
  const std::string id() const override { return "WorkspaceGroup"; }
  /// Returns a formatted string detailing the contents of the group
  const std::string toString() const override;

  /// Return the memory size of all workspaces in this group and subgroups
  size_t getMemorySize() const override;
  /// Sort the internal data structure according to member name
  void sortMembersByName();
  /// Adds a workspace to the group.
  void addWorkspace(Workspace_sptr workspace);
  /// Return the number of entries within the group
  int getNumberOfEntries() const { return static_cast<int>(this->size()); }
  /// Return the size of the group, so it is more like a container
  size_t size() const { return m_workspaces.size(); }
  /// Return the ith workspace
  Workspace_sptr getItem(const size_t index) const;
  /// Return the workspace by name
  Workspace_sptr getItem(const std::string wsName) const;
  /// Return all workspaces in the group as one call for thread safety
  std::vector<Workspace_sptr> getAllItems() const;
  /// Remove a workspace from the group
  void removeItem(const size_t index);
  /// Remove all names from the group but do not touch the ADS
  void removeAll();
  /// This method returns true if the group is empty (no member workspace)
  bool isEmpty() const;
  bool areNamesSimilar() const;
  /// Inidicates that the workspace group can be treated as multiperiod.
  bool isMultiperiod() const;
  /// Check if a workspace is included in this group or any nested groups.
  bool isInGroup(const Workspace &workspaceToCheck, size_t level = 0) const;
  /// Prints the group to the screen using the logger at debug
  void print() const;

  /// Returns a non-const iterator pointing at the first element in the
  /// workspace group
  std::vector<Workspace_sptr>::iterator begin();
  /// Returns a non-const iterator pointing at the last element in the workspace
  /// group
  std::vector<Workspace_sptr>::iterator end();
  /// Returns a const iterator pointing at the first element in the workspace
  /// group
  std::vector<Workspace_sptr>::const_iterator begin() const;
  /// Returns a const iterator pointing at the last element in the workspace
  /// group
  std::vector<Workspace_sptr>::const_iterator end() const;

  /// @name Wrapped ADS calls
  //@{

  /// Invokes the ADS to sort group members by workspace name
  void sortByName() {
    AnalysisDataService::Instance().sortGroupByName(this->getName());
  }

  /// Adds a workspace to the group.
  void add(const std::string &wsName) {
    AnalysisDataService::Instance().addToGroup(this->getName(), wsName);
  }
  /// Remove a name from the group
  void remove(const std::string &wsName) {
    AnalysisDataService::Instance().removeFromGroup(this->getName(), wsName);
  }
  /// Does a workspace exist within the group
  bool contains(const std::string &wsName) const;
  /// Does a workspace exist within the group
  bool contains(const Workspace_sptr &workspace) const;
  /// Add the members of the group to the given list
  void reportMembers(std::set<Workspace_sptr> &memberList) const;
  /// Returns the names of workspaces that make up this group. Note that this
  /// returns a copy as the internal vector can mutate while the vector is being
  /// iterated over.
  std::vector<std::string> getNames() const;
  //@}

  WorkspaceGroup(const WorkspaceGroup &ref) = delete;
  WorkspaceGroup &operator=(const WorkspaceGroup &) = delete;

private:
  WorkspaceGroup *doClone() const override {
    throw std::runtime_error("Cloning of WorkspaceGroup is not implemented.");
  }
  WorkspaceGroup *doCloneEmpty() const override {
    throw std::runtime_error("Cloning of WorkspaceGroup is not implemented.");
  }
  /// ADS removes a member of this group using this method. It doesn't send
  /// notifications in contrast to remove(name).
  void removeByADS(const std::string &wsName);
  /// Turn ADS observations on/off
  void observeADSNotifications(const bool observeADS);
  /// Check if a workspace is included in any child groups and groups in them.
  bool isInChildGroup(const Workspace &workspaceToCheck) const;

  /// Callback when a delete notification is received
  void workspaceDeleteHandle(
      Mantid::API::WorkspacePostDeleteNotification_ptr notice);

  /// Callback when a before-replace notification is received
  void workspaceBeforeReplaceHandle(
      Mantid::API::WorkspaceBeforeReplaceNotification_ptr notice);

  /// Observer for workspace delete notifications
  Poco::NObserver<WorkspaceGroup, Mantid::API::WorkspacePostDeleteNotification>
      m_deleteObserver;
  /// Observer for workspace before-replace notifications
  Poco::NObserver<WorkspaceGroup,
                  Mantid::API::WorkspaceBeforeReplaceNotification>
      m_beforeReplaceObserver;
  /// The list of workspace pointers in the group
  std::vector<Workspace_sptr> m_workspaces;
  /// Flag as to whether the observers have been added to the ADS
  bool m_observingADS;
  /// Recursive mutex to avoid simultaneous access
  mutable std::recursive_mutex m_mutex;

  friend class AnalysisDataServiceImpl;
  friend class Algorithm;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACEGROUP_H*/
