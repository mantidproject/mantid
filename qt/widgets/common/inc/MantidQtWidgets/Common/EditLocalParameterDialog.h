// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_EDITLOCALPARAMETERDIALOG_H_
#define MANTIDWIDGETS_EDITLOCALPARAMETERDIALOG_H_

#include "DllOption.h"
#include "MantidQtWidgets/Common/LogValueFinder.h"
#include "MantidQtWidgets/Common/MantidDialog.h"
#include "ui_EditLocalParameterDialog.h"
#include <QDialog>
#include <memory>

namespace MantidQt {
namespace MantidWidgets {

class FunctionMultiDomainPresenter;

/**
 * A dialog for displaying and editing values of local parameters.
 * Parameters can be set individually or all to the same value.
 * They also can be fixed and unfixed.
 */
class EXPORT_OPT_MANTIDQT_COMMON EditLocalParameterDialog
    : public MantidQt::API::MantidDialog {
  Q_OBJECT
public:
  EditLocalParameterDialog(QWidget *parent, const QString &parName,
                           const QStringList &wsNames, QList<double> values,
                           QList<bool> fixes, QStringList ties,
                           QStringList constraints);
  void doSetup(const QString &parName, const QStringList &wsNames);
  QString getParameterName() const { return m_parName; }
  QList<double> getValues() const;
  QList<bool> getFixes() const;
  QStringList getTies() const;
  QStringList getConstraints() const;
  double getValue(int i) const { return m_values[i]; }
  bool isFixed(int i) const { return m_fixes[i]; }
  QString getTie(int i) const { return m_ties[i]; }
  QString getConstraint(int i) const { return m_constraints[i]; }
  bool areOthersFixed(int i) const;
  bool areAllOthersFixed(int i) const;
  bool areOthersTied(int i) const;
  bool isLogCheckboxTicked() const;

signals:
  void logOptionsChecked(bool /*_t1*/);

private slots:
  void valueChanged(int /*row*/, int /*col*/);
  void setAllValues(double /*value*/);
  void fixParameter(int /*index*/, bool /*fix*/);
  void setAllFixed(bool /*fix*/);
  void setTie(int /*index*/, QString /*tie*/);
  void setTieAll(QString /*tie*/);
  void setConstraint(int /*index*/, QString /*tie*/);
  void setConstraintAll(QString /*tie*/);
  void copy();
  void paste();
  void setValueToLog(int /*i*/);
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
  /// Cache for the constraints
  QStringList m_constraints;
  /// Log value finder
  std::unique_ptr<LogValueFinder> m_logFinder;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*MANTIDWIDGETS_EDITLOCALPARAMETERDIALOG_H_*/
