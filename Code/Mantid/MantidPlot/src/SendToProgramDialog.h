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
  SendToProgramDialog(QWidget* parent, QString& programName, std::map<std::string, std::string> programKeysAndDetails, Qt::WFlags fl = 0 );
  std::pair<std::string,std::map<std::string,std::string> > getSettings() const;

private slots:
  void browse();
  void validateAll();
  void validateName();
  void validateTarget();
  void validateSaveUsing();
  void save();

private:
  bool validName, validTarget, validSaveUsing;
  Ui::SendToProgramDialog m_uiform;
  //MantidUI * const m_mantidUI;
  std::pair<std::string,std::map<std::string,std::string> > m_settings;
};


#endif // SendToProgram_H
