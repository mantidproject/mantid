// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSETTINGS_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSETTINGS_H_

#include "ui_IndirectSettings.h"

#include "IndirectSettingsPresenter.h"

#include "DllConfig.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

#include <map>
#include <memory>

#include <QVariant>

class QIcon;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IndirectSettings
    : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  IndirectSettings(QWidget *parent = nullptr);
  ~IndirectSettings() = default;

  static std::string name() { return "Settings"; }
  static QString categoryInfo() { return "Indirect"; }
  static QIcon icon();

  void initLayout() override;
  void loadSettings();

  std::map<std::string, QVariant> getSettings() const;

signals:
  void applySettings();

private slots:
  void closeSettings();

private:
  void otherUserSubWindowCreated(QPointer<UserSubWindow> window) override;
  void
  otherUserSubWindowCreated(QList<QPointer<UserSubWindow>> &windows) override;

  void connectIndirectInterface(QPointer<UserSubWindow> window);

  std::unique_ptr<IndirectSettingsPresenter> m_presenter;
  Ui::IndirectSettings m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTSETTINGS_H_ */
