// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_IndirectInterfaceSettings.h"

#include "IIndirectSettingsView.h"

#include "DllConfig.h"

#include <memory>

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IndirectSettingsView : public IIndirectSettingsView {
  Q_OBJECT

public:
  explicit IndirectSettingsView(QWidget *parent = nullptr);
  virtual ~IndirectSettingsView() override = default;

  void setSelectedFacility(QString const &text) override;
  QString getSelectedFacility() const override;

  void setRestrictInputByNameChecked(bool check) override;
  bool isRestrictInputByNameChecked() const override;

  void setPlotErrorBarsChecked(bool check) override;
  bool isPlotErrorBarsChecked() const override;

  void setApplyText(QString const &text) override;
  void setApplyEnabled(bool enable) override;
  void setOkEnabled(bool enable) override;
  void setCancelEnabled(bool enable) override;

private slots:
  void emitOkClicked();
  void emitApplyClicked();
  void emitCancelClicked();
  void openHelp();

private:
  std::unique_ptr<Ui::IndirectInterfaceSettings> m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt
