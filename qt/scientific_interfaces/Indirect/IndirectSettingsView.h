// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSVIEW_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSVIEW_H_

#include "ui_IndirectSettings.h"

#include "IIndirectSettingsView.h"

#include "DllConfig.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectSettingsView
    : public IIndirectSettingsView {
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

  void setSetting(QString const &settingsGroup, QString const &settingName,
                  bool const &value) override;
  QVariant getSetting(QString const &settingsGroup,
                      QString const &settingName) override;

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
  Ui::IndirectSettingsDialog m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSVIEW_H_ */
