#ifndef MANTID_KERNEL_ANALYSISDATASERVICE_H_
#define MANTID_KERNEL_ANALYSISDATASERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DataService.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidAPI/Workspace.h"

#include <Poco/AutoPtr.h>

namespace Mantid {

namespace API {

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

class WorkspaceGroup;

/** The Analysis data service stores instances of the Workspace objects and
    anything that derives from template class
   DynamicFactory<Mantid::Kernel::IAlgorithm>.
    This is the primary data service that
    the users will interact with either through writing scripts or directly
    through the API. It is implemented as a singleton class.

    This is the manager/owner of Workspace* when registered.

    @author Russell Taylor, Tessella Support Services plc
    @date 01/10/2007
    @author L C Chapon, ISIS, Rutherford Appleton Laboratory

    Modified to inherit from DataService
    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport AnalysisDataServiceImpl
    : public Kernel::DataService<API::Workspace> {
public:
  /** @name Extra notifications only applicable to the ADS */
  //@{
  /// GroupWorkspaces notification is send from GroupWorkspaces algorithm
  class GroupWorkspacesNotification : public DataServiceNotification {
  public:
    /// Constructor
    GroupWorkspacesNotification(const std::vector<std::string> &wsnames)
        : DataServiceNotification("", boost::shared_ptr<API::Workspace>()),
          m_wsnames(wsnames) {}
    /// returns the workspace names
    const std::vector<std::string> &inputworkspacenames() const {
      return m_wsnames;
    }

  private:
    std::vector<std::string> m_wsnames; ///< cache of ws names
  };

  /// UnGroupingWorkspace notification is sent from UnGroupWorkspace algorithm
  /// before the WorkspaceGroup is removed from the
  /// DataService
  class UnGroupingWorkspaceNotification : public DataServiceNotification {
  public:
    /// Constructor
    UnGroupingWorkspaceNotification(const std::string &name,
                                    const boost::shared_ptr<Workspace> &obj)
        : DataServiceNotification(name, obj) {}
  };

  /// GroupWorkspaces notification is send when a group is updated by adding or
  /// removing members.
  /// Disable observing the ADS by a group
  /// (WorkspaceGroup::observeADSNotifications(false))
  /// to prevent sending this notification.
  class GroupUpdatedNotification : public DataServiceNotification {
  public:
    /// Constructor
    GroupUpdatedNotification(const std::string &name);
    /// Returns the workspace pointer cast to WorkspaceGroup
    boost::shared_ptr<const WorkspaceGroup> getWorkspaceGroup() const;
  };

  //@}

public:
  /// Return the list of illegal characters as one string
  const std::string &illegalCharacters() const;
  /// Set the list of illegal characeters
  void setIllegalCharacterList(const std::string &);
  /// Is the given name a valid name for an object in the ADS
  const std::string isValid(const std::string &name) const;
  /// Overridden add member to attach the name to the workspace when a workspace
  /// object is added to the service
  virtual void add(const std::string &name,
                   const boost::shared_ptr<API::Workspace> &workspace);
  /// Overridden addOrReplace member to attach the name to the workspace when a
  /// workspace object is added to the service
  virtual void addOrReplace(const std::string &name,
                            const boost::shared_ptr<API::Workspace> &workspace);
  /// Overridden rename member to attach the new name to the workspace when a
  /// workspace object is renamed
  virtual void rename(const std::string &oldName, const std::string &newName);
  /// Overridden remove member to delete its name held by the workspace itself
  virtual void remove(const std::string &name);

  /** Retrieve a workspace and cast it to the given WSTYPE
   *
   * @param name :: name of the workspace
   * @tparam WSTYPE :: type of workspace to cast to. Should sub-class Workspace
   * @return a shared pointer of WSTYPE
   */
  template <typename WSTYPE>
  boost::shared_ptr<WSTYPE> retrieveWS(const std::string &name) const {
    // Get as a bare workspace
    boost::shared_ptr<Mantid::API::Workspace> workspace =
        Kernel::DataService<API::Workspace>::retrieve(name);
    // Cast to the desired type and return that.
    return boost::dynamic_pointer_cast<WSTYPE>(workspace);
  }

  /** @name Methods to work with workspace groups */
  //@{
  void addToGroup(const std::string &groupName, const std::string &wsName);
  void deepRemoveGroup(const std::string &name);
  void removeFromGroup(const std::string &groupName, const std::string &wsName);
  //@}

  /// Return a lookup of the top level items
  std::map<std::string, Workspace_sptr> topLevelItems() const;

private:
  /// Checks the name is valid, throwing if not
  void verifyName(const std::string &name);

  friend struct Mantid::Kernel::CreateUsingNew<AnalysisDataServiceImpl>;
  /// Constructor
  AnalysisDataServiceImpl();
  /// Private, unimplemented copy constructor
  AnalysisDataServiceImpl(const AnalysisDataServiceImpl &);
  /// Private, unimplemented copy assignment operator
  AnalysisDataServiceImpl &operator=(const AnalysisDataServiceImpl &);
  /// Private destructor
  virtual ~AnalysisDataServiceImpl();

  /// The string of illegal characters
  std::string m_illegalChars;
};

/// Forward declaration of a specialisation of SingletonHolder for
/// AnalysisDataServiceImpl (needed for dllexport/dllimport) and a typedef for
/// it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<AnalysisDataServiceImpl>;
#endif /* _WIN32 */
typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<AnalysisDataServiceImpl>
    AnalysisDataService;

typedef Mantid::Kernel::DataService<Mantid::API::Workspace>::AddNotification
    WorkspaceAddNotification;
typedef const Poco::AutoPtr<Mantid::Kernel::DataService<
    Mantid::API::Workspace>::AddNotification> &WorkspaceAddNotification_ptr;

typedef Mantid::Kernel::DataService<
    Mantid::API::Workspace>::BeforeReplaceNotification
    WorkspaceBeforeReplaceNotification;
typedef const Poco::AutoPtr<Mantid::Kernel::DataService<
    Mantid::API::Workspace>::BeforeReplaceNotification> &
    WorkspaceBeforeReplaceNotification_ptr;

typedef Mantid::Kernel::DataService<
    Mantid::API::Workspace>::AfterReplaceNotification
    WorkspaceAfterReplaceNotification;
typedef const Poco::AutoPtr<Mantid::Kernel::DataService<
    Mantid::API::Workspace>::AfterReplaceNotification> &
    WorkspaceAfterReplaceNotification_ptr;

typedef Mantid::Kernel::DataService<
    Mantid::API::Workspace>::PreDeleteNotification
    WorkspacePreDeleteNotification;
typedef const Poco::AutoPtr<Mantid::Kernel::DataService<
    Mantid::API::Workspace>::PreDeleteNotification> &
    WorkspacePreDeleteNotification_ptr;

typedef Mantid::Kernel::DataService<
    Mantid::API::Workspace>::PostDeleteNotification
    WorkspacePostDeleteNotification;
typedef const Poco::AutoPtr<Mantid::Kernel::DataService<
    Mantid::API::Workspace>::PostDeleteNotification> &
    WorkspacePostDeleteNotification_ptr;

typedef Mantid::Kernel::DataService<Mantid::API::Workspace>::ClearNotification
    ClearADSNotification;
typedef const Poco::AutoPtr<Mantid::Kernel::DataService<
    Mantid::API::Workspace>::ClearNotification> &ClearADSNotification_ptr;

typedef Mantid::Kernel::DataService<Mantid::API::Workspace>::RenameNotification
    WorkspaceRenameNotification;
typedef const Poco::AutoPtr<
    Mantid::Kernel::DataService<Mantid::API::Workspace>::RenameNotification> &
    WorkspaceRenameNotification_ptr;

typedef AnalysisDataServiceImpl::GroupWorkspacesNotification
    WorkspacesGroupedNotification;
typedef const Poco::AutoPtr<
    AnalysisDataServiceImpl::GroupWorkspacesNotification> &
    WorkspacesGroupedNotification_ptr;

typedef AnalysisDataServiceImpl::UnGroupingWorkspaceNotification
    WorkspaceUnGroupingNotification;
typedef const Poco::AutoPtr<
    AnalysisDataServiceImpl::UnGroupingWorkspaceNotification> &
    WorkspaceUnGroupingNotification_ptr;

typedef AnalysisDataServiceImpl::GroupUpdatedNotification
    GroupUpdatedNotification;
typedef const Poco::AutoPtr<AnalysisDataServiceImpl::GroupUpdatedNotification> &
    GroupUpdatedNotification_ptr;

} // Namespace API
} // Namespace Mantid

#endif /*MANTID_KERNEL_ANALYSISDATASERVICE_H_*/
