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
  explicit IndirectSettingsDialog(
      QWidget *parent); //, QString const &settingGroup);

  void loadProperties();
  void saveProperties();

  // void updateInstrumentConfiguration();

  void setSelectedFacility(std::string const &facility);
  // void setDisabledInstruments(QStringList const &instrumentNames);

  // QString getSelectedInstrument() const;
  // QString getSelectedAnalyser() const;
  // QString getSelectedReflection() const;

signals:
  void instrumentSetupChanged(QString const &, QString const &,
                              QString const &);

private slots:
  void okClicked();
  void applyClicked();
  void cancelClicked();

private:
  int findFacilityIndex(std::string const &text);

  // void setSelectedInstrument(QString const &instrument);
  // void setSelectedAnalyser(QString const &analyser);
  // void setSelectedReflection(QString const &reflection);

  QString getSelectedFacility() const;

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
