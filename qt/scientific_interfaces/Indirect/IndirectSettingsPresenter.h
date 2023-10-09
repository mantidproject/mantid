// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IndirectSettingsModel.h"
#include "IndirectSettingsView.h"

#include <memory>

#include <QObject>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IndirectSettingsPresenter : public QObject {
  Q_OBJECT

public:
  explicit IndirectSettingsPresenter(std::unique_ptr<IndirectSettingsModel> model, IIndirectSettingsView *view);

  QWidget *getView();

  void loadSettings();

  void notifyOkClicked();
  void notifyApplyClicked();
  void notifyCancelClicked();

signals:
  void closeSettings();
  void applySettings();

private:
  void saveSettings();

  void setApplyingChanges(bool applyingChanges);

  std::unique_ptr<IndirectSettingsModel> m_model;
  IIndirectSettingsView *m_view;
};

} // namespace CustomInterfaces
} // namespace MantidQt
