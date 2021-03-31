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
  explicit IndirectSettingsPresenter(QWidget *parent = nullptr);
  explicit IndirectSettingsPresenter(IndirectSettingsModel *model, IIndirectSettingsView *view);

  IIndirectSettingsView *getView();

  void loadSettings();

signals:
  void closeSettings();
  void applySettings();

private slots:
  void applyAndCloseSettings();
  void applyChanges();
  void closeDialog();

private:
  void setUpPresenter();
  void setDefaultRestrictData() const;
  void saveSettings();

  void setApplyingChanges(bool applyingChanges);

  std::unique_ptr<IndirectSettingsModel> m_model;
  std::unique_ptr<IIndirectSettingsView> m_view;
};

} // namespace CustomInterfaces
} // namespace MantidQt
