// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSPRESENTER_H_

#include "IndirectSettingsModel.h"
#include "IndirectSettingsView.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IndirectSettingsPresenter : public QObject {
  Q_OBJECT

public:
  explicit IndirectSettingsPresenter(QWidget *parent,
                                     std::string const &settingsGroup,
                                     std::string const &availableSettings);

  void showDialog();
  void loadSettings();

signals:
  void applySettings();

private slots:
  void updateRestrictInputByName(std::string const &text);
  void applyAndCloseSettings();
  void applySettings();
  void closeDialog();

private:
  void initLayout();
  void loadRestrictInputSetting(std::string const &settingsGroup);
  void saveSettings();

  void setApplyingChanges(bool applyingChanges);

  std::unique_ptr<IndirectSettingsView> m_view;
  std::unique_ptr<IndirectSettingsModel> m_model;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSPRESENTER_H_ */
