// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSVIEW_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSVIEW_H_

#include "ui_IndirectSettings.h"

#include <QDialog>
#include <QObject>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IndirectSettingsView : public QDialog {
  Q_OBJECT

public:
  explicit IndirectSettingsView(QWidget *parent);

  void setInterfaceSettingsVisible(bool visible);
  void setInterfaceGroupBoxTitle(QString const &title);

  void setRestrictInputByNameVisible(bool visible);
  void setPlotErrorBarsVisible(bool visible);

  void setSelectedFacility(QString const &text);
  QString getSelectedFacility() const;

  void setRestrictInputByNameChecked(bool check);
  bool isRestrictInputByNameChecked() const;

  void setPlotErrorBarsChecked(bool check);
  bool isPlotErrorBarsChecked() const;

  void setSetting(QString const &settingsGroup, QString const &settingName,
                  bool const &value);
  QVariant getSetting(QString const &settingsGroup, QString const &settingName);

  void setApplyText(QString const &text);
  void setApplyEnabled(bool enable);
  void setOkEnabled(bool enable);
  void setCancelEnabled(bool enable);

signals:
  void updateRestrictInputByName(std::string const &text);
  void okClicked();
  void applyClicked();
  void cancelClicked();

private slots:
  void emitUpdateRestrictInputByName(QString const &text);
  void emitOkClicked();
  void emitApplyClicked();
  void emitCancelClicked();

private:
  Ui::IndirectSettingsDialog m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSVIEW_H_ */
