#ifndef MDFEDITLOCALPARAMETERDIALOG_H_
#define MDFEDITLOCALPARAMETERDIALOG_H_

#include "DllConfig.h"
#include "MDFLogValueFinder.h"
#include "MantidQtWidgets/Common/IFunctionBrowser.h"
#include "ui_EditLocalParameterDialog.h"
#include <QDialog>

namespace MantidQt {
namespace CustomInterfaces {

class MultiDatasetFit;

namespace MDF {

/**
 * A dialog for displaying and editing values of local parameters.
 * Parameters can be set individually or all to the same value.
 * They also can be fixed and unfixed.
 */
class MANTIDQT_MULTIDATASETFIT_DLL EditLocalParameterDialog : public QDialog {
  Q_OBJECT
public:
  EditLocalParameterDialog(MultiDatasetFit *parent, const QString &parName);
  EditLocalParameterDialog(QWidget *parent,
                           MantidWidgets::IFunctionBrowser *funcBrowser,
                           const QString &parName, const QStringList &wsNames,
                           const std::vector<size_t> &wsIndices);
  void doSetup(const QString &parName, const QStringList &wsNames,
               const std::vector<size_t> &wsIndices);
  QList<double> getValues() const;
  QList<bool> getFixes() const;
  QStringList getTies() const;
  double getValue(int i) const { return m_values[i]; }
  bool isFixed(int i) const { return m_fixes[i]; }
  QString getTie(int i) const { return m_ties[i]; }
  bool areOthersFixed(int i) const;
  bool areAllOthersFixed(int i) const;
  bool areOthersTied(int i) const;
  bool isLogCheckboxTicked() const;

signals:
  void logOptionsChecked(bool);

private slots:
  void valueChanged(int, int);
  void setAllValues(double);
  void fixParameter(int, bool);
  void setAllFixed(bool);
  void setTie(int, QString);
  void setTieAll(QString);
  void copy();
  void paste();
  void setValueToLog(int);
  void setAllValuesToLog();

private:
  bool eventFilter(QObject *obj, QEvent *ev) override;
  void showContextMenu();
  void redrawCells();
  void updateRoleColumn(int index);
  Ui::EditLocalParameterDialog m_uiForm;
  /// Parameter name
  QString m_parName;
  /// Cache for new values. size() == number of spectra
  QList<double> m_values;
  /// Cache for the "fixed" attribute. If changes are accepted
  /// parameters for which m_fixes[i] is true are fixed to their m_values[i]
  QList<bool> m_fixes;
  /// Cache for the ties
  QStringList m_ties;
  /// Log value finder
  std::unique_ptr<MDFLogValueFinder> m_logFinder;
};

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*MDFEDITLOCALPARAMETERDIALOG_H_*/
