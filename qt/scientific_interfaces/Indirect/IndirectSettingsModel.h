// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSMODEL_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSMODEL_H_

#include "DllConfig.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectSettingsModel {
public:
  IndirectSettingsModel(std::string const &settingsGroup = "Settings",
                        std::string const &availableSettings = "");
  virtual ~IndirectSettingsModel() = default;

  virtual std::string getSettingsGroup() const;

  virtual bool hasInterfaceSettings() const;
  virtual bool isSettingAvailable(std::string const &settingName) const;

  virtual void setFacility(std::string const &facility);
  virtual std::string getFacility() const;

private:
  std::string m_settingsGroup;
  std::vector<std::string> m_settingsAvailable;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSMODEL_H_ */
