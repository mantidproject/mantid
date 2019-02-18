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

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IndirectSettingsDialog : public QDialog {
  Q_OBJECT

public:
  explicit IndirectSettingsDialog(QWidget *parent,
                                  QString const &settingsGroup);

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
  void setInterfaceGroupBoxTitle(QString const &title);

  int findFacilityIndex(std::string const &text);

  QString getSelectedFacility() const;

  void setFilterInputByNameChecked(bool check);
  bool isFilterInputByNameChecked() const;

  void setApplyingChanges(bool applyingChanges);
  void setApplyText(QString const &text);
  void setApplyEnabled(bool enable);
  void setOkEnabled(bool enable);
  void setCancelEnabled(bool enable);

  QString m_settingsGroup;
  Ui::IndirectSettingsDialog m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
