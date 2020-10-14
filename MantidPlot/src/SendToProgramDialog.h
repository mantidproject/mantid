// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_SendToProgramDialog.h"
#include <QDialog>

class QLineEdit;
class QGroupBox;
class QPushButton;
class QStackedWidget;
class QWidget;
class QComboBox;
class QLabel;
class QListWidget;
class QMouseEvent;
class QStringList;

// SendToProgramDialog

class SendToProgramDialog : public QDialog {
  Q_OBJECT

public:
  SendToProgramDialog(QWidget *parent, const Qt::WFlags &fl = nullptr);
  SendToProgramDialog(QWidget *parent, const QString &programName,
                      std::map<std::string, std::string> programKeysAndDetails,
                      const Qt::WFlags &fl = nullptr);
  std::pair<std::string, std::map<std::string, std::string>>
  getSettings() const;

private slots:
  /// Open up a new file browsing window.
  void browse();

  /// Validate all user entered fields to enable/disable the save button.
  void validateAll();

  /// See if user has entered a name for the program.
  void validateName();

  /// Validate user specified target.
  void validateTarget();

  /// Validate user specified save algorithm.
  void validateSaveUsing();

  /// Save the new/edited program.
  void save();

private:
  bool validName, validTarget, validSaveUsing;
  Ui::SendToProgramDialog m_uiform;
  std::pair<std::string, std::map<std::string, std::string>> m_settings;
};
