#ifndef SendToProgramDialog_H
#define SendToProgramDialog_H

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

//SendToProgramDialog

class SendToProgramDialog : public QDialog
{
  Q_OBJECT
  
public:
  SendToProgramDialog(QWidget* parent, Qt::WFlags fl = 0 );
  SendToProgramDialog(QWidget* parent, QString programName, std::map<std::string, std::string> programKeysAndDetails, Qt::WFlags fl = 0 );
  std::pair<std::string,std::map<std::string,std::string> > getSettings() const;

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
  std::pair<std::string,std::map<std::string,std::string> > m_settings;
};


#endif // SendToProgram_H
