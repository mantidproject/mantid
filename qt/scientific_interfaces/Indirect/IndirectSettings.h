// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_IndirectSettings.h"

#include "IndirectSettingsPresenter.h"

#include "DllConfig.h"

#include <map>
#include <memory>

#include <QMainWindow>
#include <QVariant>

class QIcon;

namespace MantidQt {
namespace CustomInterfaces {
class IndirectInterface;

class MANTIDQT_INDIRECT_DLL IIndirectSettings {

public:
  virtual void notifyApplySettings() = 0;
  virtual void notifyCloseSettings() = 0;
};

class MANTIDQT_INDIRECT_DLL IndirectSettings : public QMainWindow, public IIndirectSettings {
  Q_OBJECT

public:
  IndirectSettings(QWidget *parent = nullptr);
  ~IndirectSettings() = default;

  void connectInterface(IndirectInterface *interface);

  static QIcon icon();
  static std::map<std::string, QVariant> getSettings();

  void loadSettings();

  void notifyApplySettings() override;
  void notifyCloseSettings() override;

signals:
  void applySettings();

private:
  // void otherUserSubWindowCreated(QPointer<UserSubWindow> window) override;
  // void otherUserSubWindowCreated(QList<QPointer<UserSubWindow>> &windows) override;

  // void connectIndirectInterface(const QPointer<UserSubWindow> &window);

  std::unique_ptr<IndirectSettingsPresenter> m_presenter;
  Ui::IndirectSettings m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt
