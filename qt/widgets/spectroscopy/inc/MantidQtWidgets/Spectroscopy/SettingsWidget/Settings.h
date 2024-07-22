// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "SettingsPresenter.h"

#include "../DllConfig.h"

#include <map>
#include <memory>

#include <QVariant>
#include <QWidget>

class QIcon;

namespace MantidQt {
namespace API {
class UserSubWindow;
}

namespace CustomInterfaces {

class MANTID_SPECTROSCOPY_DLL ISettings {

public:
  virtual void notifyApplySettings() = 0;
  virtual void notifyCloseSettings() = 0;
};

class MANTID_SPECTROSCOPY_DLL Settings : public QWidget, public ISettings {
  Q_OBJECT

public:
  Settings(QWidget *parent = nullptr);
  ~Settings() = default;

  void connectExistingInterfaces(QList<QPointer<MantidQt::API::UserSubWindow>> const &windows);

  static QIcon icon();
  static std::map<std::string, QVariant> getSettings();

  void loadSettings();

  void notifyApplySettings() override;
  void notifyCloseSettings() override;

signals:
  void applySettings();

private:
  std::unique_ptr<SettingsPresenter> m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt
