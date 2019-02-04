// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_CONFIGPROPERTYOBSERVER_H_
#define MANTID_KERNEL_CONFIGPROPERTYOBSERVER_H_
#include "ConfigObserver.h"
#include "ConfigService.h"
#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {
/** The ConfigObserver is used to observe changes to the value of a single
   configuration property based on notifications sent from the ConfigService.
 */
class MANTID_KERNEL_DLL ConfigPropertyObserver : ConfigObserver {
public:
  ConfigPropertyObserver(std::string propertyName);
  virtual ~ConfigPropertyObserver() = default;

protected:
  void onValueChanged(const std::string &name, const std::string &newValue,
                      const std::string &prevValue) override;

  virtual void onPropertyValueChanged(const std::string &newValue,
                                      const std::string &prevValue) = 0;

private:
  std::string m_propertyName;
};
} // namespace Kernel
} // namespace Mantid
#endif /*MANTID_KERNEL_CONFIGPROPERTYOBSERVER_H_*/
