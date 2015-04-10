#ifndef MDFEDITLOCALPARAMETERDIALOG_H_
#define MDFEDITLOCALPARAMETERDIALOG_H_

#include "ui_EditLocalParameterDialog.h"
#include <QDialog>

namespace MantidQt
{
namespace CustomInterfaces
{

class MultiDatasetFit;

namespace MDF
{

/**
  * A dialog for displaying and editing values of local parameters.
  * Parameters can be set individually or all to the same value.
  * They also can be fixed and unfixed.
  */
class EditLocalParameterDialog: public QDialog
{
  Q_OBJECT
public:
  EditLocalParameterDialog(MultiDatasetFit *parent, const QString &parName);
  QList<double> getValues() const;
  QList<bool> getFixes() const;
  bool isFixed(int i) const {return m_fixes[i];}
private slots:
  void valueChanged(int,int);
  void setAllValues(double);
  void fixParameter(int,bool);
  void setAllFixed(bool);
  void copy();
  void paste();
private:
  bool eventFilter(QObject * obj, QEvent * ev);
  void showContextMenu();
  Ui::EditLocalParameterDialog m_uiForm;
  /// Parameter name
  QString m_parName;
  /// Cache for new values. size() == number of spectra
  QList<double> m_values;
  /// Cache for the "fixed" attribute. If changes are accepted
  /// parameters for which m_fixes[i] is true are fixed to their m_values[i]
  QList<bool> m_fixes;
};


} // MDF
} // CustomInterfaces
} // MantidQt


#endif /*MDFEDITLOCALPARAMETERDIALOG_H_*/
