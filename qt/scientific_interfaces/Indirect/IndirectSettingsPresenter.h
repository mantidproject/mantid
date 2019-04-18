// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSPRESENTER_H_

#include "DllConfig.h"
#include "IndirectSettingsModel.h"
#include "IndirectSettingsView.h"

#include <memory>

#include <QObject>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectSettingsPresenter : public QObject {
  Q_OBJECT

public:
  explicit IndirectSettingsPresenter(QWidget *parent);
  explicit IndirectSettingsPresenter(IndirectSettingsModel *model,
                                     IIndirectSettingsView *view);

  void showDialog();
  void loadSettings();

  QVariant getSetting(std::string const &settingName);

signals:
  void applySettings();

private slots:
  void applyAndCloseSettings();
  void applyChanges();
  void closeDialog();

private:
  void setUpPresenter();
  void saveSettings();

  void setApplyingChanges(bool applyingChanges);

  std::unique_ptr<IndirectSettingsModel> m_model;
  std::unique_ptr<IIndirectSettingsView> m_view;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSPRESENTER_H_ */
