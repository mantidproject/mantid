// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_InterfaceSettings.h"

#include "ISettingsView.h"

#include "../DllConfig.h"

#include <memory>

#include <QObject>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {
class SettingsPresenter;

class MANTID_SPECTROSCOPY_DLL SettingsView final : public QWidget, public ISettingsView {
  Q_OBJECT

public:
  explicit SettingsView(QWidget *parent = nullptr);

  QWidget *getView() override;
  void subscribePresenter(SettingsPresenter *presenter) override;

  void setSelectedFacility(QString const &text) override;
  QString getSelectedFacility() const override;

  void setRestrictInputByNameChecked(bool check) override;
  bool isRestrictInputByNameChecked() const override;

  void setPlotErrorBarsChecked(bool check) override;
  bool isPlotErrorBarsChecked() const override;

  void setDeveloperFeatureFlags(QStringList const &flags) override;
  QStringList developerFeatureFlags() const override;

  void setApplyText(QString const &text) override;
  void setApplyEnabled(bool enable) override;
  void setOkEnabled(bool enable) override;
  void setCancelEnabled(bool enable) override;

private slots:
  void notifyOkClicked();
  void notifyApplyClicked();
  void notifyCancelClicked();
  void openHelp();

private:
  SettingsPresenter *m_presenter;
  std::unique_ptr<Ui::InterfaceSettings> m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt
