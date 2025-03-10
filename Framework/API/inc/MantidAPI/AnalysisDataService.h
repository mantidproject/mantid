// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/DataService.h"
#include "MantidKernel/SingletonHolder.h"

#include <Poco/AutoPtr.h>

namespace Mantid {

namespace API {

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

class WorkspaceGroup;

/** The Analysis data service stores instances of the Workspace objects and
    anything that derives from template class
    DynamicFactory<Mantid::Kernel::IAlgorithm>. This is the primary data service
    that the users will interact with either through writing scripts or directly
    through the API. It is implemented as a singleton class.

    This is the manager/owner of Workspace* when registered.

    @author Russell Taylor, Tessella Support Services plc
    @date 01/10/2007
    @author L C Chapon, ISIS, Rutherford Appleton Laboratory

    Modified to inherit from DataService
*/
class MANTID_API_DLL AnalysisDataServiceImpl final : public Kernel::DataService<API::Workspace> {
public:
  /** @name Extra notifications only applicable to the ADS */
  //@{
  /// GroupWorkspaces notification is send from GroupWorkspaces algorithm
  class GroupWorkspacesNotification : public DataServiceNotification {
  public:
    /// Constructor
    GroupWorkspacesNotification(const std::vector<std::string> &wsnames)
        : DataServiceNotification("", std::shared_ptr<API::Workspace>()), m_wsnames(wsnames) {}
    /// returns the workspace names
    const std::vector<std::string> &inputworkspacenames() const { return m_wsnames; }

  private:
    std::vector<std::string> m_wsnames; ///< cache of ws names
  };

  /// UnGroupingWorkspace notification is sent from UnGroupWorkspace algorithm
  /// before the WorkspaceGroup is removed from the DataService
  class UnGroupingWorkspaceNotification : public DataServiceNotification {
  public:
    /// Constructor
    UnGroupingWorkspaceNotification(const std::string &name, const std::shared_ptr<Workspace> &obj)
        : DataServiceNotification(name, obj) {}
  };

  /// GroupWorkspaces notification is send when a group is updated by adding
  /// or removing members.
  /// Disable observing the ADS by a group
  /// (WorkspaceGroup::observeADSNotifications(false))
  /// to prevent sending this notification.
  class GroupUpdatedNotification : public DataServiceNotification {
  public:
    /// Constructor
    GroupUpdatedNotification(const std::string &name);
    /// Returns the workspace pointer cast to WorkspaceGroup
    std::shared_ptr<const WorkspaceGroup> getWorkspaceGroup() const;
  };

  //@}

public:
  /// Return the list of illegal characters as one string
  const std::string &illegalCharacters() const;
  /// Set the list of illegal characters
  void setIllegalCharacterList(const std::string &);
  /// Is the given name a valid name for an object in the ADS
  const std::string isValid(const std::string &name) const;
  /// Overridden add member to attach the name to the workspace when a
  /// workspace object is added to the service
  void add(const std::string &name, const std::shared_ptr<API::Workspace> &workspace) override;
  /// Overridden addOrReplace member to attach the name to the workspace when
  /// a workspace object is added to the service
  void addOrReplace(const std::string &name, const std::shared_ptr<API::Workspace> &workspace) override;
  /// Overridden rename member to attach the new name to the workspace when a
  /// workspace object is renamed
  virtual void rename(const std::string &oldName, const std::string &newName);
  /// Overridden remove member to delete its name held by the workspace itself
  virtual Workspace_sptr remove(const std::string &name);
  /// Random generated unique workspace name
  const std::string uniqueName(const int n = 5, const std::string &prefix = "", const std::string &suffix = "");
  /// Random generated unique hidden workspace name
  const std::string uniqueHiddenName();

  /** Retrieve a workspace and cast it to the given WSTYPE
   *
   * @param name :: name of the workspace
   * @tparam WSTYPE :: type of workspace to cast to. Should sub-class
   * Workspace
   * @return a shared pointer of WSTYPE
   */
  template <typename WSTYPE> std::shared_ptr<WSTYPE> retrieveWS(const std::string &name) const {
    // Get as a bare workspace
    try {
      // Cast to the desired type and return that.
      return std::dynamic_pointer_cast<WSTYPE>(Kernel::DataService<API::Workspace>::retrieve(name));

    } catch (Kernel::Exception::NotFoundError &) {
      throw;
    }
  }

  std::vector<Workspace_sptr> retrieveWorkspaces(const std::vector<std::string> &names,
                                                 bool unrollGroups = false) const;

  /** @name Methods to work with workspace groups */
  //@{
  void sortGroupByName(const std::string &groupName);
  void addToGroup(const std::string &groupName, const std::string &wsName);
  void deepRemoveGroup(const std::string &name);
  void removeFromGroup(const std::string &groupName, const std::string &wsName);
  //@}

  /// Return a lookup of the top level items
  std::map<std::string, Workspace_sptr> topLevelItems() const;

private:
  /// Checks the name is valid, throwing if not
  void verifyName(const std::string &name, const std::shared_ptr<API::WorkspaceGroup> &workspace);
  static char getRandomLowercaseLetter();

  friend struct Mantid::Kernel::CreateUsingNew<AnalysisDataServiceImpl>;
  /// Constructor
  AnalysisDataServiceImpl();
  /// Private, unimplemented copy constructor
  AnalysisDataServiceImpl(const AnalysisDataServiceImpl &) = delete;
  /// Private, unimplemented copy assignment operator
  AnalysisDataServiceImpl &operator=(const AnalysisDataServiceImpl &) = delete;
  /// Private destructor
  ~AnalysisDataServiceImpl() override = default;

  /// The string of illegal characters
  std::string m_illegalChars;
};

using AnalysisDataService = Mantid::Kernel::SingletonHolder<AnalysisDataServiceImpl>;

using WorkspaceAddNotification = Mantid::Kernel::DataService<Mantid::API::Workspace>::AddNotification;
using WorkspaceAddNotification_ptr =
    const Poco::AutoPtr<Mantid::Kernel::DataService<Mantid::API::Workspace>::AddNotification> &;

using WorkspaceBeforeReplaceNotification =
    Mantid::Kernel::DataService<Mantid::API::Workspace>::BeforeReplaceNotification;
using WorkspaceBeforeReplaceNotification_ptr =
    const Poco::AutoPtr<Mantid::Kernel::DataService<Mantid::API::Workspace>::BeforeReplaceNotification> &;

using WorkspaceAfterReplaceNotification = Mantid::Kernel::DataService<Mantid::API::Workspace>::AfterReplaceNotification;
using WorkspaceAfterReplaceNotification_ptr =
    const Poco::AutoPtr<Mantid::Kernel::DataService<Mantid::API::Workspace>::AfterReplaceNotification> &;

using WorkspacePreDeleteNotification = Mantid::Kernel::DataService<Mantid::API::Workspace>::PreDeleteNotification;
using WorkspacePreDeleteNotification_ptr =
    const Poco::AutoPtr<Mantid::Kernel::DataService<Mantid::API::Workspace>::PreDeleteNotification> &;

using WorkspacePostDeleteNotification = Mantid::Kernel::DataService<Mantid::API::Workspace>::PostDeleteNotification;
using WorkspacePostDeleteNotification_ptr =
    const Poco::AutoPtr<Mantid::Kernel::DataService<Mantid::API::Workspace>::PostDeleteNotification> &;

using ClearADSNotification = Mantid::Kernel::DataService<Mantid::API::Workspace>::ClearNotification;
using ClearADSNotification_ptr =
    const Poco::AutoPtr<Mantid::Kernel::DataService<Mantid::API::Workspace>::ClearNotification> &;

using WorkspaceRenameNotification = Mantid::Kernel::DataService<Mantid::API::Workspace>::RenameNotification;
using WorkspaceRenameNotification_ptr =
    const Poco::AutoPtr<Mantid::Kernel::DataService<Mantid::API::Workspace>::RenameNotification> &;

using WorkspacesGroupedNotification = AnalysisDataServiceImpl::GroupWorkspacesNotification;
using WorkspacesGroupedNotification_ptr = const Poco::AutoPtr<AnalysisDataServiceImpl::GroupWorkspacesNotification> &;

using WorkspaceUnGroupingNotification = AnalysisDataServiceImpl::UnGroupingWorkspaceNotification;
using WorkspaceUnGroupingNotification_ptr =
    const Poco::AutoPtr<AnalysisDataServiceImpl::UnGroupingWorkspaceNotification> &;

using GroupUpdatedNotification = AnalysisDataServiceImpl::GroupUpdatedNotification;
using GroupUpdatedNotification_ptr = const Poco::AutoPtr<AnalysisDataServiceImpl::GroupUpdatedNotification> &;

} // Namespace API
} // Namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::AnalysisDataServiceImpl>;
}
} // namespace Mantid
