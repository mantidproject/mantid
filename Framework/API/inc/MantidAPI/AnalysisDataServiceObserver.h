// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
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

class MANTID_API_DLL AnalysisDataServiceObserver {
public:
  AnalysisDataServiceObserver();
  virtual ~AnalysisDataServiceObserver();

  void observeAll(bool turnOn = true);
  void observeAdd(bool turnOn = true);
  void observeReplace(bool turnOn = true);
  void observeDelete(bool turnOn = true);

  virtual void anyChangeHandle() {}

protected:
  virtual void addHandle(const std::string &wsName, const Workspace_sptr ws);
  virtual void replaceHandle(const std::string &wsName,
                             const Workspace_sptr ws);
  virtual void deleteHandle(const std::string &wsName, const Workspace_sptr ws);

private:
  bool m_observingAdd, m_observingReplace, m_observingDelete;

  void _addHandle(
      const Poco::AutoPtr<AnalysisDataServiceImpl::AddNotification> &pNf);
  void _replaceHandle(
      const Poco::AutoPtr<AnalysisDataServiceImpl::AfterReplaceNotification>
          &pNf);
  void _deleteHandle(
      const Poco::AutoPtr<AnalysisDataServiceImpl::PostDeleteNotification>
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
                  AnalysisDataServiceImpl::PostDeleteNotification>
      m_deleteObserver;

  /// Poco::NObserver for ClearNotification

  /// Poco::NObserver for RenameNotification

  /// Poco::NObserver for GroupNotification

  /// Poco::NObserver for UnGroupNotification
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_ANALYSISDATASERVICEOBSERVER_H_*/