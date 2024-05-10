// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/LogValueFinder.h"
#include "MantidQtWidgets/Common/MantidDialog.h"
#include "ui_EditLocalParameterDialog.h"

#include <memory>
#include <string>
#include <vector>

#include <QDialog>

namespace MantidQt {
namespace MantidWidgets {

class FunctionMultiDomainPresenter;

/**
 * A dialog for displaying and editing values of local parameters.
 * Parameters can be set individually or all to the same value.
 * They also can be fixed and unfixed.
 */
class EXPORT_OPT_MANTIDQT_COMMON EditLocalParameterDialog : public MantidQt::API::MantidDialog {
  Q_OBJECT
public:
  EditLocalParameterDialog(QWidget *parent, const std::string &parName, const std::vector<std::string> &datasetNames,
                           const std::vector<std::string> &datasetDomainNames, const QList<double> &values,
                           const QList<bool> &fixes, const QStringList &ties, const QStringList &constraints);

  std::string getParameterName() const { return m_parName; }
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
  void dialogFinished(int /*result*/, EditLocalParameterDialog * /*dialog*/);

private slots:
  void emitDialogFinished(int /*result*/);
  void valueChanged(int /*row*/, int /*col*/);
  void setAllValues(double /*value*/);
  void fixParameter(int /*index*/, bool /*fix*/);
  void setAllFixed(bool /*fix*/);
  void setTie(int /*index*/, QString /*tie*/);
  void setTieAll(const QString & /*tie*/);
  void setConstraint(int /*index*/, QString /*tie*/);
  void setConstraintAll(const QString & /*tie*/);
  void copy();
  void paste();
  void setValueToLog(int /*i*/);
  void setAllValuesToLog();

private:
  void doSetup(const std::string &parName, const std::vector<std::string> &datasetDomains,
               const std::vector<std::string> &datasetDomainNames);
  bool eventFilter(QObject *obj, QEvent *ev) override;
  void showContextMenu();
  void redrawCells();
  void updateRoleColumn(int index);
  Ui::EditLocalParameterDialog m_uiForm;
  /// Parameter name
  std::string m_parName;
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
