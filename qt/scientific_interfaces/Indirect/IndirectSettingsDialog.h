// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSDIALOG_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSDIALOG_H_

#include "ui_IndirectSettings.h"

#include <QDialog>
#include <QObject>
#include <QSettings>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IndirectSettingsDialog : public QDialog {
  Q_OBJECT

public:
  explicit IndirectSettingsDialog(QWidget *parent, QString const &settingsGroup,
                                  QString const &settings);

  void loadSettings();
  void saveSettings();

  void setSelectedFacility(std::string const &facility);

signals:
  void updateSettings();

private slots:
  void updateFilterInputByName(QString const &);

  void okClicked();
  void applyClicked();
  void cancelClicked();

private:
  void initLayout();
  void setInterfaceSettingsVisible(bool visible);
  void setInterfaceGroupBoxTitle(QString const &title);

  void loadFilterInputByNameSetting(QSettings const &settings);
  template <typename T>
  void saveSetting(QSettings &settings, QString const &settingName,
                   T const &value);

  int findFacilityIndex(std::string const &text);

  QString getSelectedFacility() const;

  void setFilterInputByNameVisible(bool visible);
  void setFilterInputByNameChecked(bool check);
  bool isFilterInputByNameChecked() const;

  void setApplyingChanges(bool applyingChanges);
  void setApplyText(QString const &text);
  void setApplyEnabled(bool enable);
  void setOkEnabled(bool enable);
  void setCancelEnabled(bool enable);

  QStringList m_settings;
  QString m_settingsGroup;
  Ui::IndirectSettingsDialog m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
