// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"

#include "MantidQtWidgets/Common/FunctionMultiDomainPresenter.h"
#include "MantidQtWidgets/Common/LocalParameterItemDelegate.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"

#include <QClipboard>
#include <QMenu>
#include <QMessageBox>
#include <limits>
#include <utility>

namespace {
QString makeNumber(double d) { return QString::number(d, 'g', 16); }
const int valueColumn = 0;
const int roleColumn = 1;
} // namespace

namespace MantidQt::MantidWidgets {

/**
 * Constructor for EditLocalParameterDialog used in FunctionBrowser
 * @param parent :: [input] Parent widget of this dialog
 * @param parName :: [input] Name of parameter to edit in this dialog
 * @param datasetNames :: [input] Names of workspaces being fitted.
 * @param datasetDomainNames :: [input] Names given to the domains being fitted.
 * @param values :: [input] Parameter values.
 * @param fixes :: [input] Flags indicating if a parameter is fixed.
 * @param ties :: [input] Parameter ties.
 * @param constraints :: [input] Parameter constraints.
 */
EditLocalParameterDialog::EditLocalParameterDialog(QWidget *parent, const std::string &parName,
                                                   const std::vector<std::string> &datasetNames,
                                                   const std::vector<std::string> &datasetDomainNames,
                                                   const QList<double> &values, const QList<bool> &fixes,
                                                   const QStringList &ties, const QStringList &constraints)
    : MantidDialog(parent), m_parName(parName), m_values(values), m_fixes(fixes), m_ties(std::move(ties)),
      m_constraints(std::move(constraints)) {
  assert(static_cast<std::size_t>(values.size()) == datasetDomainNames.size());
  assert(static_cast<std::size_t>(fixes.size()) == datasetDomainNames.size());
  assert(static_cast<std::size_t>(ties.size()) == datasetDomainNames.size());
  assert(static_cast<std::size_t>(constraints.size()) == datasetDomainNames.size());
  m_uiForm.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose);
  doSetup(parName, datasetNames, datasetDomainNames);
}

/**
 * Common setup method used by both constructors
 * Prerequisite: one of the constructors must have filled m_values, m_fixes,
 * m_ties and set up the UI first
 * @param parName :: [input] Name of parameter to edit in this dialog
 * @param datasetNames :: [input] Names of workspaces being fitted.
 * @param datasetDomainNames :: [input] Names given to the domains being fitted.
 */
void EditLocalParameterDialog::doSetup(const std::string &parName, const std::vector<std::string> &datasetNames,
                                       const std::vector<std::string> &datasetDomainNames) {
  m_logFinder = std::make_unique<LogValueFinder>(stdVectorToQStringList(datasetNames));
  // Populate list of logs
  auto *logCombo = m_uiForm.logValueSelector->getLogComboBox();
  for (const auto &logName : m_logFinder->getLogNames()) {
    logCombo->addItem(QString::fromStdString(logName));
  }

  m_uiForm.logValueSelector->setCheckboxShown(true);
  connect(m_uiForm.logValueSelector, SIGNAL(logOptionsEnabled(bool)), this, SIGNAL(logOptionsChecked(bool)));
  QHeaderView *header = m_uiForm.tableWidget->horizontalHeader();
  header->setSectionResizeMode(QHeaderView::Stretch);
  connect(m_uiForm.tableWidget, SIGNAL(cellChanged(int, int)), this, SLOT(valueChanged(int, int)));
  m_uiForm.lblParameterName->setText("Parameter: " + QString::fromStdString(parName));

  for (int i = 0; i < static_cast<int>(datasetDomainNames.size()); i++) {
    m_uiForm.tableWidget->insertRow(i);
    auto cell = new QTableWidgetItem(makeNumber(m_values[i]));
    m_uiForm.tableWidget->setItem(i, valueColumn, cell);
    auto headerItem = new QTableWidgetItem(QString::fromStdString(datasetDomainNames[i]));
    m_uiForm.tableWidget->setVerticalHeaderItem(i, headerItem);
    cell = new QTableWidgetItem("");
    auto flags = cell->flags();
    flags ^= Qt::ItemIsEditable;
    flags ^= Qt::ItemIsSelectable;
    flags ^= Qt::ItemIsEnabled;
    cell->setFlags(flags);
    m_uiForm.tableWidget->setItem(i, roleColumn, cell);
    updateRoleColumn(i);
  }
  auto deleg = new LocalParameterItemDelegate(this);
  m_uiForm.tableWidget->setItemDelegateForColumn(valueColumn, deleg);
  connect(deleg, SIGNAL(setAllValues(double)), this, SLOT(setAllValues(double)));
  connect(deleg, SIGNAL(fixParameter(int, bool)), this, SLOT(fixParameter(int, bool)));
  connect(deleg, SIGNAL(setAllFixed(bool)), this, SLOT(setAllFixed(bool)));
  connect(deleg, SIGNAL(setTie(int, QString)), this, SLOT(setTie(int, QString)));
  connect(deleg, SIGNAL(setTieAll(QString)), this, SLOT(setTieAll(QString)));
  connect(deleg, SIGNAL(setConstraint(int, QString)), this, SLOT(setConstraint(int, QString)));
  connect(deleg, SIGNAL(setConstraintAll(QString)), this, SLOT(setConstraintAll(QString)));
  connect(deleg, SIGNAL(setValueToLog(int)), this, SLOT(setValueToLog(int)));
  connect(deleg, SIGNAL(setAllValuesToLog()), this, SLOT(setAllValuesToLog()));

  m_uiForm.tableWidget->installEventFilter(this);

  connect(this, SIGNAL(finished(int)), this, SLOT(emitDialogFinished(int)));
}

void EditLocalParameterDialog::emitDialogFinished(int result) { emit dialogFinished(result, this); }

/// Slot. Called when a value changes.
/// @param row :: Row index of the changed cell.
/// @param col :: Column index of the changed cell.
void EditLocalParameterDialog::valueChanged(int row, int col) {
  if (col == valueColumn) {
    QString text = m_uiForm.tableWidget->item(row, col)->text();
    try {
      bool ok = false;
      double value = text.toDouble(&ok);
      if (ok) {
        m_values[row] = value;
      } else {
        m_ties[row] = text;
      }
    } catch (std::exception &) {
      // restore old value
      m_uiForm.tableWidget->item(row, col)->setText(makeNumber(m_values[row]));
    }
  }
}

/// Set all parameters to the same value.
/// @param value :: A new value.
void EditLocalParameterDialog::setAllValues(double value) {
  int n = m_values.size();
  for (int i = 0; i < n; ++i) {
    m_values[i] = value;
    m_uiForm.tableWidget->item(i, valueColumn)->setText(makeNumber(value));
    updateRoleColumn(i);
  }
}

/// Get the list of new parameter values.
QList<double> EditLocalParameterDialog::getValues() const { return m_values; }

/// Get a list with the "fixed" attribute.
QList<bool> EditLocalParameterDialog::getFixes() const { return m_fixes; }

/// Get a list of the ties.
QStringList EditLocalParameterDialog::getTies() const { return m_ties; }

/// Get a list of the constraints
QStringList EditLocalParameterDialog::getConstraints() const { return m_constraints; }

/// Fix/unfix a single parameter.
/// @param index :: Index of a paramter to fix or unfix.
/// @param fix :: Fix (true) or unfix (false).
void EditLocalParameterDialog::fixParameter(int index, bool fix) {
  m_fixes[index] = fix;
  m_ties[index] = "";
  updateRoleColumn(index);
}

/// Set a new tie for a parameter
/// @param index :: Index of a paramter to tie.
/// @param tie :: A tie string.
void EditLocalParameterDialog::setTie(int index, QString tie) {
  m_ties[index] = std::move(tie);
  m_fixes[index] = false;
  updateRoleColumn(index);
}

/// Set the same tie to all parameters.
/// @param tie :: A tie string.
void EditLocalParameterDialog::setTieAll(const QString &tie) {
  for (int i = 0; i < m_ties.size(); ++i) {
    m_ties[i] = tie;
    m_fixes[i] = false;
    updateRoleColumn(i);
  }
  redrawCells();
}

void EditLocalParameterDialog::setConstraint(int index, QString constraint) {
  m_constraints[index] = std::move(constraint);
  updateRoleColumn(index);
}

void EditLocalParameterDialog::setConstraintAll(const QString &constraint) {
  for (int i = 0; i < m_constraints.size(); ++i) {
    m_constraints[i] = constraint;
    updateRoleColumn(i);
  }
  redrawCells();
}

/// Fix/unfix all parameters.
/// @param fix :: Fix (true) or unfix (false).
void EditLocalParameterDialog::setAllFixed(bool fix) {
  if (m_fixes.empty())
    return;
  for (int i = 0; i < m_fixes.size(); ++i) {
    m_fixes[i] = fix;
    m_ties[i] = "";
    updateRoleColumn(i);
  }
  redrawCells();
}

/// Event filter for managing the context menu.
bool EditLocalParameterDialog::eventFilter(QObject *obj, QEvent *ev) {
  if (obj == m_uiForm.tableWidget && ev->type() == QEvent::ContextMenu) {
    showContextMenu();
  }
  return QDialog::eventFilter(obj, ev);
}

/// Show the context menu.
void EditLocalParameterDialog::showContextMenu() {
  auto selection = m_uiForm.tableWidget->selectionModel()->selectedColumns();

  bool hasSelection = false;

  for (auto &index : selection) {
    if (index.column() == valueColumn)
      hasSelection = true;
  }

  if (!hasSelection)
    return;

  QMenu *menu = new QMenu(this);
  {
    QAction *action = new QAction("Copy", this);
    action->setToolTip("Copy data to clipboard.");
    connect(action, SIGNAL(triggered()), this, SLOT(copy()));
    menu->addAction(action);
  }
  {
    QAction *action = new QAction("Paste", this);
    action->setToolTip("Paste data from clipboard.");
    connect(action, SIGNAL(triggered()), this, SLOT(paste()));
    auto text = QApplication::clipboard()->text();
    action->setEnabled(!text.isEmpty());
    menu->addAction(action);
  }

  menu->exec(QCursor::pos());
}

/// Copy all parameter values to the clipboard.
/// Values will be separated by '\n'
void EditLocalParameterDialog::copy() {
  QStringList text;
  auto n = m_values.size();
  for (int i = 0; i < n; ++i) {
    text << makeNumber(m_values[i]);
  }
  QApplication::clipboard()->setText(text.join("\n"));
}

/// Paste a list of values from the clipboard.
void EditLocalParameterDialog::paste() {
  auto text = QApplication::clipboard()->text();
  auto vec = text.split(QRegExp("\\s|,"), Qt::SkipEmptyParts);
  auto n = qMin(vec.size(), m_uiForm.tableWidget->rowCount());
  // prepare for pasting data
  auto deleg = static_cast<LocalParameterItemDelegate *>(m_uiForm.tableWidget->itemDelegateForColumn(valueColumn));
  deleg->prepareForPastedData();
  // insert data into table
  for (int i = 0; i < n; ++i) {
    auto str = vec[i];
    bool ok;
    m_values[i] = str.toDouble(&ok);
    if (!ok)
      str = "0";
    m_uiForm.tableWidget->item(i, valueColumn)->setText(str);
  }
}

/// Force the table to redraw its cells.
void EditLocalParameterDialog::redrawCells() {
  for (int i = 0; i < m_values.size(); ++i) {
    // it's the only way I am able to make the table to repaint itself
    auto text = makeNumber(m_values[i]);
    m_uiForm.tableWidget->item(i, valueColumn)->setText(text + " ");
    m_uiForm.tableWidget->item(i, valueColumn)->setText(text);
  }
}

/// Update the text in the role column
void EditLocalParameterDialog::updateRoleColumn(int index) {
  auto cell = m_uiForm.tableWidget->item(index, roleColumn);
  QString text;
  if (m_fixes[index]) {
    text = "fixed";
    cell->setForeground(QBrush(Qt::red));
  } else if (!m_ties[index].isEmpty()) {
    text = "tied";
    cell->setForeground(QBrush(Qt::blue));
  } else {
    text = "fitted";
    cell->setForeground(QBrush(Qt::darkGreen));
  }
  if (!m_constraints[index].isEmpty()) {
    text += ", " + m_constraints[index];
  }
  cell->setText(text);
}

/// Check if there are any other fixed parameters
bool EditLocalParameterDialog::areOthersFixed(int i) const {
  for (int j = 0; j < m_fixes.size(); ++j) {
    if (j != i && m_fixes[j])
      return true;
  }
  return false;
}

/// Check if all other parameters are fixed
bool EditLocalParameterDialog::areAllOthersFixed(int i) const {
  for (int j = 0; j < m_fixes.size(); ++j) {
    if (j != i && !m_fixes[j])
      return false;
  }
  return true;
}

/// Check if there are any other tied parameters
bool EditLocalParameterDialog::areOthersTied(int i) const {
  for (int j = 0; j < m_fixes.size(); ++j) {
    if (j != i && !m_ties[j].isEmpty())
      return true;
  }
  return false;
}

/// Set value to log value
/// @param i :: [input] Index of parameter to set
void EditLocalParameterDialog::setValueToLog(int i) {
  assert(i < m_values.size());

  const auto &logName = m_uiForm.logValueSelector->getLog();
  const auto &function = m_uiForm.logValueSelector->getFunction();

  double value = std::numeric_limits<double>::quiet_NaN();
  try {
    value = m_logFinder->getLogValue(logName, function, i);
  } catch (const std::invalid_argument &err) {
    const auto &message = QString("Failed to get log value:\n\n %1").arg(err.what());
    QMessageBox::critical(this, "Mantid - Error", message);
  }
  m_values[i] = value;
  m_uiForm.tableWidget->item(i, valueColumn)->setText(makeNumber(value));
  updateRoleColumn(i);
}

/// Set value of each parameter to log value from respective workspace
void EditLocalParameterDialog::setAllValuesToLog() {
  const int nValues = m_values.size();
  for (int i = 0; i < nValues; ++i) {
    setValueToLog(i);
  }
}

/// Returns whether log checkbox is ticked or not
/// @returns True if log options are enabled
bool EditLocalParameterDialog::isLogCheckboxTicked() const { return m_uiForm.logValueSelector->isCheckboxTicked(); }

} // namespace MantidQt::MantidWidgets
