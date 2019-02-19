// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSMODEL_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSMODEL_H_

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IndirectSettingsModel {
public:
  IndirectSettingsModel(std::string const &settingsGroup,
                        std::string const &availableSettings);

  std::string getSettingsGroup() const;

  bool hasInterfaceSettings() const;
  bool isSettingAvailable(std::string const &settingName) const;

  void setFacility(std::string const &facility);
  std::string getFacility() const;

private:
  std::vector<std::string> m_settingsAvailable;
  std::string m_settingsGroup;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSMODEL_H_ */
