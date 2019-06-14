// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_ANALYSISDATASERVICEOBSERVER_H_
#define MANTID_KERNEL_ANALYSISDATASERVICEOBSERVER_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DataService.h"
#include <Poco/NObserver.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace API {

/*
 * To use the AnalysisDataServiceObserver you will need to do a few things:
 *
 * 1. Inherit from this class in the class you wish to take effect on
 *
 * 2. Make sure that the effect you are attempting to observe has been added
 * to the AnalysisDataService itself by using the public method in this
 * class, e.g. observeAll, observeAdd, observeReplace etc.
 *
 * 3. The last thing to actually have something take effect is by overriding
 * the relevant handle function e.g. when observing all override
 * anyChangeHandle and anything done in that overriden method will happen
 * every time something changes in the AnalysisDataService.
 *
 * This works in both C++ and Python, some functionality is limited in
 * python, but the handlers will all be called.
 */

class MANTID_API_DLL AnalysisDataServiceObserver {
public:
  AnalysisDataServiceObserver();
  virtual ~AnalysisDataServiceObserver();

  void observeAll(bool turnOn = true);
  void observeAdd(bool turnOn = true);
  void observeReplace(bool turnOn = true);
  void observeDelete(bool turnOn = true);
  void observeClear(bool turnOn = true);
  void observeRename(bool turnOn = true);
  void observeGroup(bool turnOn = true);
  void observeUnGroup(bool turnOn = true);
  void observeGroupUpdate(bool turnOn = true);

  virtual void anyChangeHandle();
  virtual void addHandle(const std::string &wsName, const Workspace_sptr &ws);
  virtual void replaceHandle(const std::string &wsName,
                             const Workspace_sptr &ws);
  virtual void deleteHandle(const std::string &wsName,
                            const Workspace_sptr &ws);
  virtual void clearHandle();
  virtual void renameHandle(const std::string &wsName,
                            const std::string &newName);
  virtual void groupHandle(const std::string &wsName, const Workspace_sptr &ws);
  virtual void unGroupHandle(const std::string &wsName,
                             const Workspace_sptr &ws);
  virtual void groupUpdateHandle(const std::string &wsName,
                                 const Workspace_sptr &ws);

private:
  bool m_observingAdd{false}, m_observingReplace{false},
      m_observingDelete{false}, m_observingClear{false},
      m_observingRename{false}, m_observingGroup{false},
      m_observingUnGroup{false}, m_observingGroupUpdate{false};

  void _addHandle(
      const Poco::AutoPtr<AnalysisDataServiceImpl::AddNotification> &pNf);
  void _replaceHandle(
      const Poco::AutoPtr<AnalysisDataServiceImpl::AfterReplaceNotification>
          &pNf);
  void _deleteHandle(
      const Poco::AutoPtr<AnalysisDataServiceImpl::PreDeleteNotification> &pNf);
  void _clearHandle(
      const Poco::AutoPtr<AnalysisDataServiceImpl::ClearNotification> &pNf);
  void _renameHandle(
      const Poco::AutoPtr<AnalysisDataServiceImpl::RenameNotification> &pNf);
  void _groupHandle(
      const Poco::AutoPtr<AnalysisDataServiceImpl::GroupWorkspacesNotification>
          &pNf);
  void _unGroupHandle(
      const Poco::AutoPtr<
          AnalysisDataServiceImpl::UnGroupingWorkspaceNotification> &pNf);
  void _groupUpdateHandle(
      const Poco::AutoPtr<AnalysisDataServiceImpl::GroupUpdatedNotification>
          &pNf);

  /// Poco::NObserver for AddNotification.
  Poco::NObserver<AnalysisDataServiceObserver,
                  AnalysisDataServiceImpl::AddNotification>
      m_addObserver;

  /// Poco::NObserver for ReplaceNotification.
  Poco::NObserver<AnalysisDataServiceObserver,
                  AnalysisDataServiceImpl::AfterReplaceNotification>
      m_replaceObserver;

  /// Poco::NObserver for DeleteNotification.
  Poco::NObserver<AnalysisDataServiceObserver,
                  AnalysisDataServiceImpl::PreDeleteNotification>
      m_deleteObserver;

  /// Poco::NObserver for ClearNotification
  Poco::NObserver<AnalysisDataServiceObserver,
                  AnalysisDataServiceImpl::ClearNotification>
      m_clearObserver;

  /// Poco::NObserver for RenameNotification
  Poco::NObserver<AnalysisDataServiceObserver,
                  AnalysisDataServiceImpl::RenameNotification>
      m_renameObserver;

  /// Poco::NObserver for GroupNotification
  Poco::NObserver<AnalysisDataServiceObserver,
                  AnalysisDataServiceImpl::GroupWorkspacesNotification>
      m_groupObserver;

  /// Poco::NObserver for UnGroupNotification
  Poco::NObserver<AnalysisDataServiceObserver,
                  AnalysisDataServiceImpl::UnGroupingWorkspaceNotification>
      m_unGroupObserver;

  /// Poco::NObserver for GroupUpdateNotification
  Poco::NObserver<AnalysisDataServiceObserver,
                  AnalysisDataServiceImpl::GroupUpdatedNotification>
      m_groupUpdatedObserver;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_ANALYSISDATASERVICEOBSERVER_H_*/