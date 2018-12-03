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
  
  virtual void anyChangeHandle() {}

protected:
  virtual void addHandle(const std::string &wsName,
                         const Workspace_sptr ws);
private:
  bool m_observingAdd;

  void _addHandle(const Poco::AutoPtr<AnalysisDataServiceImpl::AddNotification> &pNf){
    this->anyChangeHandle();
    this->addHandle(pNf->objectName(), pNf->object());
  }
  /// Poco::NObserver for AddNotification.
  Poco::NObserver<AnalysisDataServiceObserver, AnalysisDataServiceImpl::AddNotification>
      m_addObserver;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_ANALYSISDATASERVICEOBSERVER_H_*/