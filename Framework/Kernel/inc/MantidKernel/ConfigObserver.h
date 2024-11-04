// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "ConfigService.h"
#include "MantidKernel/DllConfig.h"
#include "Poco/NObserver.h"

namespace Mantid {
namespace Kernel {
/** The ConfigObserver is used to observe changes in the configuration based
  on notifications sent from the ConfigService.
 */
class MANTID_KERNEL_DLL ConfigObserver {
public:
  ConfigObserver();
  ConfigObserver(const ConfigObserver &other);
  ConfigObserver &operator=(const ConfigObserver &other);
  virtual ~ConfigObserver() noexcept;

  void notifyValueChanged(const std::string &name, const std::string &newValue, const std::string &prevValue);
  void notifyValueChanged(ConfigValChangeNotification_ptr notification);

protected:
  virtual void onValueChanged(const std::string &name, const std::string &newValue, const std::string &prevValue) = 0;

private:
  Poco::NObserver<ConfigObserver, Mantid::Kernel::ConfigValChangeNotification> m_valueChangeListener;
};
} // namespace Kernel
} // namespace Mantid
