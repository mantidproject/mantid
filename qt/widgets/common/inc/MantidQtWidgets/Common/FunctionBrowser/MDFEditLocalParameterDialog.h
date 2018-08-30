#ifndef MDFEDITLOCALPARAMETERDIALOG_H_
#define MDFEDITLOCALPARAMETERDIALOG_H_

#include "MDFEditLocalParameterDialogSubscriber.h"

#include "MantidQtWidgets/Common/DllOption.h"
#include "ui_EditLocalParameterDialog.h"
#include <QDialog>

namespace MantidQt {
namespace CustomInterfaces {
namespace QENS {

class DLLExport EditLocalParameterDialog : public QDialog {
  Q_OBJECT
public:
  EditLocalParameterDialog();
  EditLocalParameterDialog(QWidget *parent);

  void subscribe(MDF::EditLocalParameterDialogSubscriber *subscriber);

  void setParameterNameTitle(std::string const &name);

  void addFixedParameter(std::string const &datasetName, double value);
  void addTiedParameter(std::string const &datasetName, double value,
                        std::string const &expression);
  void addFittedParameter(std::string const &datasetName, double value);

  void setParameterToFixed(int index);
  void setParameterToTied(int index);
  void setParameterToFitted(int index);

  void addLogsToMenu(std::vector<std::string> const &logNames);
  void clearLogsInMenu();

  void setParameterValues(double value);
  void setTies(std::string const &tie);
  void setParameterValue(double value, int index);
  void setTie(std::string const &tie, int index);

  void copyToClipboard(std::string const &text);

private slots:
  void cellChanged(int, int);
  void valuesChanged(double);
  void fixChanged(bool);
  void tieChanged(QString const &);
  void fixChanged(int, bool);
  void tieChanged(int, QString const &);
  void copyClicked();
  void pasteClicked(std::string const &text);
  void logValueChanged(int);
  void logValueChanged();

private:
  QString getValueAt(int row) const;
  int addRowToTable(QString const &datasetName, QString const &value);
  void addRowToTable(QString const &datasetName, QString const &value, int row);
  QTableWidgetItem *getRoleItemAt(int row);
  QTableWidgetItem *getValueItemAt(int row);
  void setRoleItemAt(QString const &value, QBrush const &foreground, int row);
  void setValueItemAt(QString const &value, int row);

  MDF::EditLocalParameterDialogSubscriber *m_subscriber;
  Ui::EditLocalParameterDialog m_uiForm;
};

} // namespace QENS
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
